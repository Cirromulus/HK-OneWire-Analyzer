#ifndef BOWIRE_SIMULATION_DATA_GENERATOR
#define BOWIRE_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class BOWireAnalyzerSettings;

class BOWireSimulationDataGenerator
{
public:
	BOWireSimulationDataGenerator();
	~BOWireSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, BOWireAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	BOWireAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //BOWIRE_SIMULATION_DATA_GENERATOR