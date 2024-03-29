#include "BOWireAnalyzer.h"
#include "BOWireAnalyzerSettings.h"
#include "BOWire.h"
#include <AnalyzerChannelData.h>

#include <iostream>

using namespace BOWire;
using namespace std;


BOWireAnalyzer::BOWireAnalyzer()
:	Analyzer2(),
	mSettings( new BOWireAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
	UseFrameV2();
}

BOWireAnalyzer::~BOWireAnalyzer()
{
	KillThread();
}

void BOWireAnalyzer::SetupResults()
{
	mResults.reset( new BOWireAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

void BOWireAnalyzer::WorkerThread()
{
	const auto sampleRateHz = GetSampleRate();
	const auto samplesPerTick = getSamplesPerTick(mSettings->mTimeBase_us, sampleRateHz);

	BOWireState state;

	mChannelData = GetAnalyzerChannelData( mSettings->mInputChannel );

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
								mSettings->mInputChannel );

			// nothing good will come from this.
			mResults->CancelPacketAndStartNewPacket();
			state.reset();
			continue;
		}
		auto& bitType = *maybeBitType;

		// for(Ticks i=0; i < maxLowTicks; i++ )
		// {
		// 	//let's put a dot for every tick counter (just cosmetics)
		// 	mResults->AddMarker(fallingEdge + i * samplesPerTick,
		// 					    AnalyzerResults::Dot,
		// 						mSettings->mInputChannel);
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
				state = BOWireState(fallingEdge);
				markerType = AnalyzerResults::Start;
				mResults->CommitPacketAndStartNewPacket(); // probably useless anyway
				break;
			case BitType::data1:
				state.setCurrentBit(1);
				markerType = AnalyzerResults::One;
				break;
			case BitType::data0:
				// nothing, as we shift in a zero...
				markerType = AnalyzerResults::Zero;
				break;
			case BitType::end:
				markerType = AnalyzerResults::Stop;
				break;
			default:
				cerr << "Üeh, unknown bit type! (" << to_underlying(bitType) <<
						")" << endl;
				markerType = AnalyzerResults::ErrorX;
		}
		state.currentNumberOfBitsReceived++;
		mResults->AddMarker(centerOfLowPulse, markerType, mSettings->mInputChannel);

		// check for transition
		const auto expectedBitsInThisState = getBitsPerWord(state.wordState);
		if (! expectedBitsInThisState.has_value())
		{
			cerr << "We over-stated our state (" << to_underlying(state.wordState) <<
						")" << endl;
			mResults->AddMarker(centerOfLowPulse,
					AnalyzerResults::ErrorSquare,
					mSettings->mInputChannel);
			// PS.: It is ok that we already wrote into something,
			// we have a buffer of one byte (because of ::_num)
			mResults->CancelPacketAndStartNewPacket();
			continue;
		}
		if (state.currentNumberOfBitsReceived == *expectedBitsInThisState)
		{
			// in word level, we don't care about the actual state
			const bool wordlevelProduceFrame =
					(mSettings->mDecodeLevel == BOWireAnalyzerSettings::wordlevel) &&
					hasWordStateData(state.wordState);
			const bool commandLevelProduceFrame =
					(mSettings->mDecodeLevel == BOWireAnalyzerSettings::commandlevel) &&
					(bitType == BitType::end);
			if (wordlevelProduceFrame || commandLevelProduceFrame)
			{
				//print out a Frame
				const auto& endOfFrame = risingEdge;
				addFrame(state, endOfFrame);
				ReportProgress( endOfFrame );
			}

			// advance
			state.wordState = static_cast<WordState>(to_underlying(state.wordState) + 1);
			state.currentNumberOfBitsReceived = 0;

			// commit markers and maybe frame
			mResults->CommitResults();
		}

	}
}

void
BOWireAnalyzer::addFrame(const BOWire::BOWireState& state, const U32& now)
{
	// inspired by one-wire: just generate both v1 and v2 frames, lolo
	const auto& endOfTransmission = now;

	Frame frame;	// needed for bubble text
	frame.mStartingSampleInclusive = state.startOfCurrentWord;
	frame.mEndingSampleInclusive = endOfTransmission;
	frame.mData1 = state.payload.getSerialized();
	//frame.mData2 = some_more_data_we_collected;
	frame.mType = to_underlying(state.wordState);
	frame.mFlags = mSettings->mDecodeLevel;
	mResults->AddFrame( frame );

	FrameV2 frame_v2;
	if (mSettings->mDecodeLevel == BOWireAnalyzerSettings::wordlevel)
	{
		const char* type = getNameOfWordState(state.wordState);
		const auto wordWidth = getBitsPerWord(state.wordState).value_or(8);
		if (wordWidth <= 8)
		{
			frame_v2.AddByte(type, state.payload.getWord(state.wordState));
		}
		else
		{
			frame_v2.AddByteArray(type, reinterpret_cast<const U8*>(&state.payload.data), 2);
		}
		mResults->AddFrameV2( frame_v2, type, state.startOfCurrentWord, endOfTransmission );
	}
	else
	{
		//  commandlevel
		const char* type = "command";
		frame_v2.AddByte(getNameOfWordState(WordState::source), state.payload.getWord(WordState::source));
		frame_v2.AddByte(getNameOfWordState(WordState::dest), state.payload.getWord(WordState::dest));
		frame_v2.AddByte(getNameOfWordState(WordState::command), state.payload.getWord(WordState::command));
		if (state.wordState == WordState::data || state.wordState == WordState::endBit)
		{
			type = "command with data";
			frame_v2.AddByteArray(getNameOfWordState(WordState::data), reinterpret_cast<const U8*>(&state.payload.data), 2);
		}
		mResults->AddFrameV2( frame_v2, type, state.startOfTransmission, endOfTransmission );
	}

	// no commit, because this is done somewhere else
}

bool BOWireAnalyzer::NeedsRerun()
{
	// hm
	return false;
}

U32 BOWireAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 BOWireAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mTimeBase_us * 4;
}

const char* BOWireAnalyzer::GetAnalyzerName() const
{
	return "B&O Onewire";
}

const char* GetAnalyzerName()
{
	return "B&O Onewire";
}

Analyzer* CreateAnalyzer()
{
	return new BOWireAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}