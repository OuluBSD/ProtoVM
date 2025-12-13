#include "InstrumentEngineFactory.h"
#include "InstrumentToDsp.h"
#include "DspRuntime.h"

Result<ProtoVM_AudioEngine*> InstrumentEngineFactory::CreateEngineForInstrument(
    const InstrumentGraph& instrument,
    const ProtoVM_AudioEngineConfig& cfg
) {
    // Create the C ABI engine
    ProtoVM_AudioEngine* engine = ProtoVM_AudioEngine_Create(&cfg);
    if (!engine) {
        return Result<ProtoVM_AudioEngine*>::Error("Failed to create audio engine");
    }

    // Convert the instrument graph to a DSP graph
    DspGraph dsp_graph;
    InstrumentToDsp converter;
    auto conversion_result = converter.Convert(instrument, dsp_graph);
    if (!conversion_result.IsOk()) {
        ProtoVM_AudioEngine_Destroy(engine);
        return Result<ProtoVM_AudioEngine*>::Error("Failed to convert instrument to DSP graph");
    }

    // Store the instrument graph and DSP graph in the engine
    // Note: This assumes that the ProtoVM_AudioEngine struct can be accessed from here
    // In a real implementation, we might need to update the internal structure of ProtoVM_AudioEngine
    // or provide additional functions to associate these graphs with the engine
    
    // For now, we just return the engine as created
    return Result<ProtoVM_AudioEngine*>::Success(engine);
}