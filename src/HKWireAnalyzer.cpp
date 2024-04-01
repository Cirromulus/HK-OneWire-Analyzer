#include "HKWireAnalyzer.h"
#include "HKWireAnalyzerSettings.h"
#include "HKWire.h"
#include <AnalyzerChannelData.h>

#include <arpa/inet.h> // E.T., are you there?
#include <iostream>

using namespace HKWire;
using namespace std;


HKWireAnalyzer::HKWireAnalyzer()
:	Analyzer2(),
	mSettings( new HKWireAnalyzerSettings() )
{
	SetAnalyzerSettings( mSettings.get() );
	UseFrameV2();
}

HKWireAnalyzer::~HKWireAnalyzer()
{
	KillThread();
}

void HKWireAnalyzer::SetupResults()
{
	mResults.reset( new HKWireAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mDataChannel );
}

void HKWireAnalyzer::WorkerThread()
{
	const auto sampleRateHz = GetSampleRate();
	const auto samplesPerTick = getSamplesPerTick(mSettings->mTimeBase_us, sampleRateHz);

	HKWireState state;

	mChannelData = GetAnalyzerChannelData( mSettings->mDataChannel );

	for( ; ; )
	{
		mChannelData->AdvanceToNextEdge();
		if( mChannelData->GetBitState() != BIT_LOW )
		{
			// We look for transitions from HIGH to LOW, so skip this one
			continue;
		}
		const auto fallingEdge = mChannelData->GetSampleNumber();
		mChannelData->AdvanceToNextEdge();
		const auto risingEdge = mChannelData->GetSampleNumber();

		const auto lowPulseLength = risingEdge - fallingEdge;
		const auto centerOfLowPulse = fallingEdge + lowPulseLength / 2;
		const auto expectedNextLowEdge = risingEdge + samplesPerTick * Waveform::high;

		const auto waveform = Waveform{matchSamplesToTicks(samplesPerTick, lowPulseLength)};
		auto maybeBitType = getBitFromWaveform(waveform);
		if (!maybeBitType.has_value())
		{
			// Üeh
			mResults->AddMarker(centerOfLowPulse,
								AnalyzerResults::ErrorDot,
								mSettings->mDataChannel );

			// nothing good will come from this.
			mResults->CancelPacketAndStartNewPacket();
			state.reset();
			continue;
		}
		auto& bitType = *maybeBitType;

		// // this does not work, because it advances the mResults time pointer too far
		// for(Ticks i=0; i < waveform.low; i++ )
		// {
		// 	//let's put a dot for every tick counter (just cosmetics)
		// 	mResults->AddMarker(fallingEdge + i * samplesPerTick,
		// 					    AnalyzerResults::Dot,
		// 						mSettings->mDataChannel);
		// }

		// check duration of stop (high) pulse
		// end byte only differs by high-duration.
		// This is observed to be about 8ms, or just one Tick if busy.
		// Could be checked by "busy" line,
		// but this is not necessary (just greater than 2 ticks or equal to one tick)
		// or implied through the number of observed bits (TODO)
		const auto samplingOffset = samplesPerTick / 2;
		const auto oneTick  = samplesPerTick;
		const auto twoTicks = samplesPerTick * 2;
		const auto isShortHigh = mChannelData->WouldAdvancingCauseTransition(oneTick + samplingOffset);
		const auto isLongHigh = !mChannelData->WouldAdvancingCauseTransition(twoTicks + samplingOffset);
		if (isShortHigh || isLongHigh)
		{
			// this is an end bit
			bitType = BitType::end;
		}

		if (state.currentNumberOfBitsReceived == 0)
		{
			// could have been estimated in the transition, but this is cleaner.
			state.startOfCurrentWord = fallingEdge;
		}

		// now let's reason about this bit.
		AnalyzerResults::MarkerType markerType;
		switch (bitType)
		{
			case BitType::start:
				// start state.
				state = HKWireState(fallingEdge);
				markerType = AnalyzerResults::Start;
				mResults->CommitPacketAndStartNewPacket(); // probably useless anyway
				break;
			case BitType::data1:
				state.setCurrentBit(1);
				markerType = AnalyzerResults::One;
				break;
			case BitType::data0:
				state.setCurrentBit(0);
				markerType = AnalyzerResults::Zero;
				break;
			case BitType::end:
				markerType = isShortHigh ? AnalyzerResults::Dot : AnalyzerResults::Stop;
				// this could indicate the actual state, but
				// knowledge about whether we had data is
				// easier to keep with the state than an extra bool
				// state.consumeBit(bitType);
				break;
			default:
				cerr << "Üeh, unknown bit type! (" << to_underlying(bitType) <<
						")" << endl;
				markerType = AnalyzerResults::ErrorX;
		}
		state.currentNumberOfBitsReceived++;
		mResults->AddMarker(centerOfLowPulse, markerType, mSettings->mDataChannel);

		// check for transition
		const auto canAdvanceState = state.canAdvanceState(bitType);
		if (! canAdvanceState.has_value())
		{
			cerr << "We over-stated our state (" << to_underlying(state.wordState) <<
						")" << endl;
			mResults->AddMarker(centerOfLowPulse,
					AnalyzerResults::ErrorSquare,
					mSettings->mDataChannel);
			// PS.: It is ok that we already wrote into something,
			// we have a buffer of one byte (because of ::_num)
			mResults->CancelPacketAndStartNewPacket();
			continue;
		}
		if (canAdvanceState.value())
		{
			// in word level, we don't care about the actual state
			const bool wordlevelProduceFrame =
					(mSettings->mDecodeLevel == HKWireAnalyzerSettings::wordlevel) &&
					// don't want to produce end frame info here
					hasWordStateData(state.wordState) && bitType != BitType::end;

			const bool commandLevelProduceFrame =
					(mSettings->mDecodeLevel == HKWireAnalyzerSettings::commandlevel) &&
					(bitType == BitType::end);

			if (wordlevelProduceFrame || commandLevelProduceFrame)
			{
				//print out a Frame
				const auto& endOfFrame = risingEdge + oneTick;
				addFrame(state, endOfFrame);
				ReportProgress( endOfFrame );
			}

			// advance
			state.advanceState();

			// commit markers and maybe frame
			mResults->CommitResults();
		}

	}
}

void
HKWireAnalyzer::addFrame(const HKWire::HKWireState& state, const U32& endOfTransmission)
{
	if (mSettings->mDecodeLevel == HKWireAnalyzerSettings::wordlevel)
	{
		addWordFrame(state, endOfTransmission);
	}
	else if (mSettings->mDecodeLevel == HKWireAnalyzerSettings::commandlevel)
	{
		addCommandFrame(state, endOfTransmission);
	}
	// no commit, because this is done somewhere else
}

void
HKWireAnalyzer::addWordFrame(const HKWire::HKWireState& state, const U32& endOfTransmission)
{
	// inspired by one-wire: just generate both v1 and v2 frames, lolo

	Frame frame;		// needed for bubble text
	frame.mStartingSampleInclusive = state.startOfCurrentWord;
	frame.mEndingSampleInclusive = endOfTransmission;
	frame.mData1 = state.payload.getSerialized();
	//frame.mData2 = some_more_data_we_collected;
	frame.mType = to_underlying(state.wordState);
	frame.mFlags = mSettings->mDecodeLevel;
	mResults->AddFrame( frame );

	FrameV2 frame_v2;	// nice for table
	const char* type = getNameOfWordState(state.wordState);
	const auto wordWidth = getBitsPerWord(state.wordState).value_or(8);
	if (wordWidth <= 8)
	{
		frame_v2.AddByte(type, state.payload.getWord(state.wordState));
	}
	else
	{
		// probably dead code with data now split in two phases
		const auto hoData = state.payload.getDataInHostOrder(); // ??
		frame_v2.AddByteArray(type, reinterpret_cast<const U8*>(&hoData), sizeof(decltype(hoData)));
	}
	mResults->AddFrameV2( frame_v2, type, state.startOfCurrentWord, endOfTransmission );
	// no commit, because this is done somewhere else
}

void
HKWireAnalyzer::addCommandFrame(const HKWire::HKWireState& state, const U32& endOfTransmission)
{
	// inspired by one-wire: just generate both v1 and v2 frames, lolo
	Frame frame;		// needed for bubble text
	frame.mStartingSampleInclusive = state.startOfTransmission;
	frame.mEndingSampleInclusive = endOfTransmission;
	frame.mData1 = state.payload.getSerialized();
	//frame.mData2 = some_more_data_we_collected;
	frame.mType = to_underlying(state.wordState);
	frame.mFlags = mSettings->mDecodeLevel;
	mResults->AddFrame( frame );

	static bool tryDecode = true;	// TODO: Make setting

	FrameV2 frame_v2;	// nice for table
	const char* type = "command";

	const ID src = state.payload.getWord(WordState::source);
	const ID dst = state.payload.getWord(WordState::dest);
	const Command cmd = state.payload.getWord(WordState::command);

	const auto maybeSrcName = knownIDs.find(src);
	const auto maybeDstName = knownIDs.find(dst);
	const auto maybeCommandDescriptionMap = knownCommands.find(dst);
	std::optional<const char*> maybeCommandName;
	if (maybeCommandDescriptionMap != knownCommands.cend())
	{
		if (maybeCommandDescriptionMap->second.find(cmd) != maybeCommandDescriptionMap->second.cend())
		{
			// Uffuff
			maybeCommandName = maybeCommandDescriptionMap->second.find(cmd)->second;
		}
	}

	// TODO: Make this less redundant
	if (tryDecode && maybeSrcName != knownIDs.cend())
	{
		frame_v2.AddString(getNameOfWordState(WordState::source), maybeSrcName->second);
	}
	else
	{
		frame_v2.AddByte(getNameOfWordState(WordState::source), state.payload.getWord(WordState::source));
	}
	if (tryDecode && maybeDstName != knownIDs.cend())
	{
		frame_v2.AddString(getNameOfWordState(WordState::dest), maybeDstName->second);
	}
	else
	{
		frame_v2.AddByte(getNameOfWordState(WordState::dest), state.payload.getWord(WordState::dest));
	}
	if (tryDecode && maybeCommandName.has_value())
	{
		frame_v2.AddString(getNameOfWordState(WordState::command), *maybeCommandName);
	}
	else
	{
		frame_v2.AddByte(getNameOfWordState(WordState::command), state.payload.getWord(WordState::command));
	}

	// "reverse" test
	if (state.payload.data2.has_value())
	{
		type = "command with 16 bit data";
		U8 arrayData[2];
		arrayData[0] = *state.payload.data1;	// zero is leftmost
		arrayData[1] = *state.payload.data2;
		frame_v2.AddByteArray("data", arrayData, sizeof(arrayData));
	}
	else if (state.payload.data1.has_value())
	{
		type = "command with 8 bit data";
		frame_v2.AddByte("data", state.payload.getDataInHostOrder());
	}
	mResults->AddFrameV2( frame_v2, type, state.startOfTransmission, endOfTransmission );

	// debugging
	if (state.startOfTransmission == 0)
	{
		cerr << "Is something fishy?" << endl;
	}

	// no commit, because this is done somewhere else
}

bool HKWireAnalyzer::NeedsRerun()
{
	// hm
	return false;
}

U32 HKWireAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	return 0;
}

U32 HKWireAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mTimeBase_us * 4;
}

const char* HKWireAnalyzer::GetAnalyzerName() const
{
	return "HK Onewire";
}

const char* GetAnalyzerName()
{
	return "HK Onewire";
}

Analyzer* CreateAnalyzer()
{
	return new HKWireAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}