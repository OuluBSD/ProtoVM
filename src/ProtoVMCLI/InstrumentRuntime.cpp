#include "InstrumentRuntime.h"
#include "InstrumentToDsp.h"
#include "DspRuntime.h"
#include <cmath>  // For standard math functions

namespace ProtoVMCLI {

Result<void> InstrumentRuntime::RenderInstrument(
    const InstrumentGraph& instrument,
    CircuitFacade& facade,
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    std::vector<float>& out_left,
    std::vector<float>& out_right) {
    
    // Build the DSP graph for the instrument
    auto dsp_graph_result = InstrumentToDsp::BuildDspGraphForInstrument(
        instrument,
        facade,
        session,
        session_dir,
        branch_name
    );
    
    if (!dsp_graph_result.ok) {
        return Result<void>::MakeError(
            dsp_graph_result.error_code,
            "Failed to build DSP graph for instrument: " + dsp_graph_result.error_message
        );
    }
    
    // Initialize the DSP runtime with the graph
    auto runtime_init_result = DspRuntime::Initialize(dsp_graph_result.data);
    if (!runtime_init_result.ok) {
        return Result<void>::MakeError(
            runtime_init_result.error_code,
            "Failed to initialize DSP runtime: " + runtime_init_result.error_message
        );
    }
    
    DspRuntimeState runtime_state = runtime_init_result.data;
    
    // Render the instrument
    auto render_result = DspRuntime::Render(runtime_state);
    if (!render_result.ok) {
        return Result<void>::MakeError(
            render_result.error_code,
            "Failed to render instrument: " + render_result.error_message
        );
    }
    
    // Copy the rendered output to the output buffers
    out_left = runtime_state.out_left;
    out_right = runtime_state.out_right;
    
    return Result<void>::MakeOk({});
}

} // namespace ProtoVMCLI