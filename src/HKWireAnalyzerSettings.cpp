#include "HKWireAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


HKWireAnalyzerSettings::HKWireAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mTimeBase_us( 560 )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Data", "Standard B&O Onewire Data" );
	mInputChannelInterface->SetChannel( mInputChannel );

	mTimeBaseInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mTimeBaseInterface->SetTitleAndTooltip( "Time Base of one tick (microseconds)",
										   "Specify the microseconds per tick." );
	mTimeBaseInterface->SetMax( 6000000 );
	mTimeBaseInterface->SetMin( 1 );
	mTimeBaseInterface->SetInteger( mTimeBase_us );

	mPacketLevelDecodeInterface.reset(new AnalyzerSettingInterfaceNumberList());
	mPacketLevelDecodeInterface->SetTitleAndTooltip( "Decode level",
										   "Define word or command level decoding" );
	mPacketLevelDecodeInterface->AddNumber(wordlevel, "Word level", "Decode individual words");
	mPacketLevelDecodeInterface->AddNumber(commandlevel, "Command level", "Decode complete commands");

	AddInterface( mInputChannelInterface.get() );
	AddInterface( mTimeBaseInterface.get() );
	AddInterface( mPacketLevelDecodeInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "csv", "csv" ); // this might be interesting some day

	ClearChannels();
	AddChannel( mInputChannel, "B&O Onewire", false );
}

HKWireAnalyzerSettings::~HKWireAnalyzerSettings()
{
}

bool HKWireAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mTimeBase_us = mTimeBaseInterface->GetInteger();
	mDecodeLevel = static_cast<DecodeLevel>(mPacketLevelDecodeInterface->GetNumber());

	ClearChannels();
	AddChannel( mInputChannel, "B&O Onewire DATA", true );
	// AddChannel( mOptionalBusyChannel, "B&O Onewire Busy", false );

	return true;
}

void HKWireAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mTimeBaseInterface->SetInteger( mTimeBase_us );
	mPacketLevelDecodeInterface->SetNumber( mDecodeLevel );
}

void HKWireAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mTimeBase_us;
	double intermediate;
	text_archive >> intermediate;
	mPacketLevelDecodeInterface->SetNumber(intermediate);

	ClearChannels();
	AddChannel( mInputChannel, "B&O Onewire", true );

	UpdateInterfacesFromSettings();
}

const char* HKWireAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mTimeBase_us;
	text_archive << mPacketLevelDecodeInterface->GetNumber();

	return SetReturnString( text_archive.GetString() );
}
