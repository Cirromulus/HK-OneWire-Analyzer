#include "BOWireAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


BOWireAnalyzerSettings::BOWireAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mTimeBase_us( 560 )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Data", "Standard B&O Onewire Data" );
	mInputChannelInterface->SetChannel( mInputChannel );

	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Time Base of one tick (microseconds)",
										   "Specify the microseconds per tick." );
	mBitRateInterface->SetMax( 6000000 );
	mBitRateInterface->SetMin( 1 );
	mBitRateInterface->SetInteger( mTimeBase_us );

	AddInterface( mInputChannelInterface.get() );
	AddInterface( mBitRateInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "B&O Onewire", false );
}

BOWireAnalyzerSettings::~BOWireAnalyzerSettings()
{
}

bool BOWireAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mTimeBase_us = mBitRateInterface->GetInteger();

	ClearChannels();
	AddChannel( mInputChannel, "B&O Onewire DATA", true );
	// AddChannel( mOptionalBusyChannel, "B&O Onewire Busy", false );

	return true;
}

void BOWireAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mBitRateInterface->SetInteger( mTimeBase_us );
}

void BOWireAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mTimeBase_us;

	ClearChannels();
	AddChannel( mInputChannel, "B&O Onewire", true );

	UpdateInterfacesFromSettings();
}

const char* BOWireAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mTimeBase_us;

	return SetReturnString( text_archive.GetString() );
}
