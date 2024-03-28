#ifndef BOWIRE_ANALYZER_SETTINGS
#define BOWIRE_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class BOWireAnalyzerSettings : public AnalyzerSettings
{
public:
	BOWireAnalyzerSettings();
	virtual ~BOWireAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();


	Channel mInputChannel;
	U64 mTimeBase_us;

protected:
	std::unique_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::unique_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
};

#endif //BOWIRE_ANALYZER_SETTINGS
