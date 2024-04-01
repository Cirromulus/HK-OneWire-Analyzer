#ifndef HKWire_ANALYZER_SETTINGS
#define HKWire_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class HKWireAnalyzerSettings : public AnalyzerSettings
{
public:
	HKWireAnalyzerSettings();
	virtual ~HKWireAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	static const char* const dataChannelName;
	static const char* const busyChannelName;


	Channel mDataChannel;
	// Channel mOptionalBusyChannel;
	U64 mTimeBase_us;

	enum DecodeLevel : uint8_t
	{
		wordlevel = 0,
		commandlevel,
		textlevel,
	} mDecodeLevel;

	inline bool
	isCommandLevel() const
	{
		return mDecodeLevel == commandlevel || mDecodeLevel == textlevel;
	}

protected:
	std::unique_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::unique_ptr< AnalyzerSettingInterfaceInteger >	mTimeBaseInterface;
	std::unique_ptr< AnalyzerSettingInterfaceNumberList > mPacketLevelDecodeInterface;
};

#endif //HKWire_ANALYZER_SETTINGS
