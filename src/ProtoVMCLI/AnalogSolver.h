#ifndef _ProtoVM_AnalogSolver_h_
#define _ProtoVM_AnalogSolver_h_

#include "AnalogModel.h"
#include "SessionTypes.h"  // Include session types
#include <string>
#include <vector>

namespace ProtoVMCLI {

struct AnalogSolverConfig {
    double sample_rate_hz;   // audio sample rate, e.g. 48000.0
    double dt;               // step size, e.g. 1.0 / sample_rate_hz
    Upp::String integrator;  // "euler", "tpt" (trapezoidal) etc., for future use
};

struct AnalogSolverState {
    AnalogBlockModel model;
    AnalogSolverConfig config;

    // Internal helper variables if needed.
    // These will be used during the solving process
    double last_output = 0.0;
};

class AnalogSolver {
public:
    static Result<AnalogSolverState> Initialize(
        const AnalogBlockModel& model,
        const AnalogSolverConfig& config
    );

    // Advance one time step, returning the current output sample.
    static Result<float> Step(AnalogSolverState& state);

    // Render N samples into a buffer.
    static Result<void> Render(
        AnalogSolverState& state,
        int total_samples,
        std::vector<float>& out_mono
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_AnalogSolver_h_