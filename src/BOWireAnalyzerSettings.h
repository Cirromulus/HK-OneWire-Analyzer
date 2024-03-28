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
	// Channel mOptionalBusyChannel;
	U64 mTimeBase_us;

	enum DecodeLevel : uint8_t
	{
		wordlevel = 0,
		commandlevel
	} mDecodeLevel;

protected:
	std::unique_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::unique_ptr< AnalyzerSettingInterfaceInteger >	mTimeBaseInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList > mPacketLevelDecodeInterface;
};

#endif //BOWIRE_ANALYZER_SETTINGS
