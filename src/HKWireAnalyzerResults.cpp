#include "HKWireAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "HKWireAnalyzer.h"
#include "HKWireAnalyzerSettings.h"
#include "HKWire.h"

#include <iostream>
#include <fstream>

using namespace HKWire;

HKWireAnalyzerResults::HKWireAnalyzerResults( HKWireAnalyzer* analyzer, HKWireAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

HKWireAnalyzerResults::~HKWireAnalyzerResults()
{
}

void HKWireAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );
	const auto& state = static_cast<WordState>(frame.mType);
	const char* type = getNameOfWordState(state);
	const auto decodeLevel = static_cast<HKWireAnalyzerSettings::DecodeLevel>(frame.mFlags);

	if (decodeLevel == HKWireAnalyzerSettings::wordlevel)
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

		AnalyzerHelpers::GetNumberString( payload.source, display_base, *getBitsPerWord(WordState::source), src, 16 );
		AnalyzerHelpers::GetNumberString( payload.dest, display_base, *getBitsPerWord(WordState::dest), dst, 16 );
		AnalyzerHelpers::GetNumberString( payload.command, display_base, *getBitsPerWord(WordState::command), cmd, 16 );
		if (payload.data1.has_value())
		{
			// ugly as fuck, for separation
			data[0] = ' ';
			const auto length = payload.getDataLength();	// might also have data2
			AnalyzerHelpers::GetNumberString( payload.getDataInHostOrder(), display_base, length, data + 1, 16 );
		}
		// TODO: Decode source, dest, and cmd
		AddResultString(src, " -> ", dst, " : ", cmd, data);
	}

}

void HKWireAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	// just assume from the first frame. Ugly AF
	const auto decodeLevel = static_cast<HKWireAnalyzerSettings::DecodeLevel>(GetFrame( 0 ).mFlags);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Type,";
	// soo ugly, so much redundancy
	if (decodeLevel != HKWireAnalyzerSettings::wordlevel)
	{
		file_stream << "Src,Dst,Cmd,";
	}
	file_stream << "Dat" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		const auto& state = static_cast<WordState>(frame.mType);
		const auto thisFramesDecodeLevel = static_cast<HKWireAnalyzerSettings::DecodeLevel>(frame.mFlags);
		if (thisFramesDecodeLevel != decodeLevel)
			// skip this one. It is different.
			continue;

		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
		file_stream << time_str;

		if (decodeLevel == HKWireAnalyzerSettings::wordlevel)
		{
			file_stream << "," << getNameOfWordState(state);
			if (hasWordStateData(state))
			{
				char number_str[128];
				const auto numBits = getBitsPerWord(state).value_or(8);
				const auto payload = Payload(frame.mData1);
				const auto data = payload.getWord(state);
				AnalyzerHelpers::GetNumberString( data, display_base, numBits, number_str, 128 );
				file_stream << "," << number_str;
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
			const bool withData = payload.data1.has_value();

			file_stream << "," << "command";
			if (withData)
				file_stream << " with data";

			AnalyzerHelpers::GetNumberString( payload.source, display_base, *getBitsPerWord(WordState::source), src, 16 );
			AnalyzerHelpers::GetNumberString( payload.dest, display_base, *getBitsPerWord(WordState::dest), dst, 16 );
			AnalyzerHelpers::GetNumberString( payload.command, display_base, *getBitsPerWord(WordState::command), cmd, 16 );
			file_stream << "," << src << "," << dst << "," << cmd;
			if (withData)
			{
				AnalyzerHelpers::GetNumberString( payload.getDataInHostOrder(), display_base, payload.getDataLength(), data, 16 );
				file_stream << "," << data;
			}
		}

		file_stream << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			break;
		}
	}

	file_stream.close();
}

void HKWireAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	// we use V2 frames

	// using namespace HKWire;
	// Frame frame = GetFrame( frame_index );
	// ClearTabularText();
	// const auto& state = static_cast<WordState>(frame.mType);
	// const char* type = getNameOfWordState(state);
	// if (hasWordStateData(state))
	// {
	// 	char number_str[128];
	// 	const auto numBits = getBitsPerWord(state).value_or(4);
	// 	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, numBits, number_str, 128 );
	// 	AddTabularText( type, " ", number_str );
	// }
	// else
	// {
	// 	AddTabularText( type );
	// }
#endif
}

void HKWireAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not yet? supported

}

void HKWireAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}