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

	if (hasWordStateData(state))
	{
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		AddResultString( type, " ", number_str );
	}
	else
	{
		AddResultString( type );
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
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
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
	//not supported

}

void BOWireAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}