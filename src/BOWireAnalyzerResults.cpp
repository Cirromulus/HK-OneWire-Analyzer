#include "BOWireAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "BOWireAnalyzer.h"
#include "BOWireAnalyzerSettings.h"
#include "BOWire.h"

#include <iostream>
#include <fstream>

BOWireAnalyzerResults::BOWireAnalyzerResults( BOWireAnalyzer* analyzer, BOWireAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

BOWireAnalyzerResults::~BOWireAnalyzerResults()
{
}

void BOWireAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	using namespace BOWire;

	ClearResultStrings();
	Frame frame = GetFrame( frame_index );
	const auto& state = static_cast<WordState>(frame.mType);
	const char* type = getNameOfWordState(state);
	const auto decodeLevel = static_cast<BOWireAnalyzerSettings::DecodeLevel>(frame.mFlags);

	if (decodeLevel == BOWireAnalyzerSettings::wordlevel)
	{
		if (hasWordStateData(state))
		{
			char number_str[128];
			const auto numBits = getBitsPerWord(state).value_or(8);
			const auto payload = Payload(frame.mData1);
			const auto data = payload.getWord(state);
			AnalyzerHelpers::GetNumberString( data, display_base, numBits, number_str, 128 );
			AddResultString( number_str );
			AddResultString( type, " ", number_str );
		}
		else
		{
			AddResultString( type );
		}
	}
	else
	{
		// whole command
		char src[16];
		char dst[16];
		char cmd[16];
		char data[16] = {0};	// default: none
		const auto payload = Payload(frame.mData1);

		const bool withData = state == WordState::end; // it finished data, so is in end bit.

		AnalyzerHelpers::GetNumberString( payload.source, display_base, *getBitsPerWord(WordState::source), src, 16 );
		AnalyzerHelpers::GetNumberString( payload.dest, display_base, *getBitsPerWord(WordState::dest), dst, 16 );
		AnalyzerHelpers::GetNumberString( payload.command, display_base, *getBitsPerWord(WordState::command), cmd, 16 );
		if (withData)
			AnalyzerHelpers::GetNumberString( payload.data, display_base, *getBitsPerWord(WordState::data), data, 16 );

		AddResultString(src, " -> ", dst, " : ", cmd, data);
	}

}

void BOWireAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );

		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		file_stream << time_str << "," << number_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void BOWireAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	using namespace BOWire;
	Frame frame = GetFrame( frame_index );
	ClearTabularText();
	const auto& state = static_cast<WordState>(frame.mType);
	const char* type = getNameOfWordState(state);
	if (hasWordStateData(state))
	{
		char number_str[128];
		const auto numBits = getBitsPerWord(state).value_or(4);
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, numBits, number_str, 128 );
		AddTabularText( type, " ", number_str );
	}
	else
	{
		AddTabularText( type );
	}
#endif
}

void BOWireAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not yet? supported

}

void BOWireAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}