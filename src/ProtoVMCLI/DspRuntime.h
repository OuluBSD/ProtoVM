#ifndef _ProtoVM_DSP_RUNTIME_h_
#define _ProtoVM_DSP_RUNTIME_h_

#include "DspGraph.h"
#include "SessionTypes.h"  // For the Result template
#include "AnalogSolver.h"  // For analog solver integration
#include <vector>
#include <string>
#include <map>

namespace ProtoVMCLI {

struct DspRuntimeState {
    DspGraph graph;

    // Buffers for audio and control signals per node/port.
    // For simplicity, you can keep per-node sample values for current frame,
    // and main output buffers for L/R.
    std::vector<float> out_left;
    std::vector<float> out_right;

    // Internal phases for built-in nodes (osc, LFO) if you implement them directly.
    // Alternatively, you can call into generated C++ code for oscillator behavior.
    double osc_phase;
    double pan_lfo_phase;

    // Analog solver states for analog block source nodes
    std::map<Upp::String, AnalogSolverState> analog_solver_states;

    // Current sample index for tracking position in rendering
    int current_sample_index;
};

class DspRuntime {
public:
    static Result<DspRuntimeState> Initialize(const DspGraph& graph);

    // Render the entire graph offline into state.out_left/right.
    static Result<void> Render(DspRuntimeState& state);
    
    // Render a single sample frame
    static Result<void> RenderSample(DspRuntimeState& state, int sample_index);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_DSP_RUNTIME_h_