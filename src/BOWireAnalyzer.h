#ifndef BOWIRE_ANALYZER_H
#define BOWIRE_ANALYZER_H

#include <Analyzer.h>
#include "BOWire.h"
#include "BOWireAnalyzerResults.h"
#include "BOWireSimulationDataGenerator.h"

class BOWireAnalyzerSettings;
class ANALYZER_EXPORT BOWireAnalyzer : public Analyzer2
{


public:
	BOWireAnalyzer();
	virtual ~BOWireAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::unique_ptr< BOWireAnalyzerSettings > mSettings;
	std::unique_ptr< BOWireAnalyzerResults > mResults;
	AnalyzerChannelData* mChannelData;

	BOWireSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

private:
	// simple dispatcher to Word or command frame functions
	void
	addFrame(const BOWire::BOWireState& state, const U32& endOfTransmission);
	void
	addWordFrame(const BOWire::BOWireState& state, const U32& endOfTransmission);
	void
	addCommandFrame(const BOWire::BOWireState& state, const U32& endOfTransmission);

};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //BOWIRE_ANALYZER_H
