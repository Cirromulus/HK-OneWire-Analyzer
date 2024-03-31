#ifndef HKWire_ANALYZER_H
#define HKWire_ANALYZER_H

#include <Analyzer.h>
#include "HKWire.h"
#include "HKWireAnalyzerResults.h"

class HKWireAnalyzerSettings;
class ANALYZER_EXPORT HKWireAnalyzer : public Analyzer2
{


public:
	HKWireAnalyzer();
	virtual ~HKWireAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::unique_ptr< HKWireAnalyzerSettings > mSettings;
	std::unique_ptr< HKWireAnalyzerResults > mResults;
	AnalyzerChannelData* mChannelData;

private:
	// simple dispatcher to Word or command frame functions
	void
	addFrame(const HKWire::HKWireState& state, const U32& endOfTransmission);
	void
	addWordFrame(const HKWire::HKWireState& state, const U32& endOfTransmission);
	void
	addCommandFrame(const HKWire::HKWireState& state, const U32& endOfTransmission);

};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //HKWire_ANALYZER_H
