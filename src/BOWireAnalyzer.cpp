#include "BOWireAnalyzer.h"
#include "BOWireAnalyzerSettings.h"
#include "BOWire.h"
#include <AnalyzerChannelData.h>

#include <iostream>

BOWireAnalyzer::BOWireAnalyzer()
:	Analyzer2(),
	mSettings( new BOWireAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
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
	using namespace BOWire;

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
		const auto risingEdge = mChannelData->GetSampleNumber(); //rising edge

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
		const Ticks safetyMargin = 1;
		// end byte only differs by high-duration.
		// This is observed to be about 8ms.
		// Could be checked by "busy" line,
		// but this is not necessary (just greater than 2 ticks or equal to one tick)
		// or implied through the number of observed bits (TODO)
		const auto samplingPointStopBit = expectedNextLowEdge + samplesPerTick * safetyMargin;
		if ( !mChannelData->WouldAdvancingToAbsPositionCauseTransition(samplingPointStopBit))
		{
			// override bit type
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
				mResults->CommitPacketAndStartNewPacket();
				break;
			case BitType::data1:
				state.data[std::to_underlying(state.wordState)] |= 1 << state.currentNumberOfBitsReceived;
				markerType = AnalyzerResults::One;
				break;
			case BitType::data0:
				// nothing, as we shift in a zero...
				markerType = AnalyzerResults::Zero;
				break;
			case BitType::end:
				markerType = AnalyzerResults::Stop;
				// mResults->CommitPacketAndStartNewPacket not needed here, as this produces no frames
				break;
			default:
				std::cerr << "Üeh, unknown bit type! (" << std::to_underlying(bitType) <<
						")" << std::endl;
				markerType = AnalyzerResults::ErrorX;
		}
		state.currentNumberOfBitsReceived++;
		mResults->AddMarker(centerOfLowPulse, markerType, mSettings->mInputChannel);

		// check for transition
		const auto expectedBitsInThisState = getBitsPerWord(state.wordState);
		if (! expectedBitsInThisState.has_value())
		{
			std::cerr << "We over-stated our state (" << std::to_underlying(state.wordState) <<
						")" << std::endl;
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
			if (hasWordStateData(state.wordState))
			{
				//print out a Frame
				const auto& endOfTransmission = risingEdge;
				Frame frame;
				frame.mStartingSampleInclusive = state.startOfCurrentWord;
				frame.mEndingSampleInclusive = endOfTransmission;
				frame.mData1 = state.data[std::to_underlying(state.wordState)];
				//frame.mData2 = some_more_data_we_collected;
				frame.mType = std::to_underlying(state.wordState);
				frame.mFlags = 0;
				// if( such_and_such_error == true )
				// frame.mFlags |= SUCH_AND_SUCH_ERROR_FLAG | DISPLAY_AS_ERROR_FLAG;
				// if( such_and_such_warning == true )
				// frame.mFlags |= SUCH_AND_SUCH_WARNING_FLAG | DISPLAY_AS_WARNING_FLAG;
				mResults->AddFrame( frame );
				ReportProgress( endOfTransmission );
			}

			// advance
			state.wordState = static_cast<WordState>(std::to_underlying(state.wordState) + 1);
			state.currentNumberOfBitsReceived = 0;
			// commit markers and maybe frame
			mResults->CommitResults();
		}

	}
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