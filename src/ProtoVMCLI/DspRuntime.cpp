#include "DspRuntime.h"
#include "DspGraph.h"
#include "AnalogSolver.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace ProtoVMCLI {

Result<DspRuntimeState> DspRuntime::Initialize(const DspGraph& graph) {
    DspRuntimeState state;
    state.graph = graph;

    // Allocate output buffers
    state.out_left.resize(graph.total_samples, 0.0f);
    state.out_right.resize(graph.total_samples, 0.0f);

    // Initialize internal phases to zero
    state.osc_phase = 0.0;
    state.pan_lfo_phase = 0.0;
    state.current_sample_index = 0;

    // Initialize analog solver states for any AnalogBlockSource nodes
    for (const auto& node : graph.nodes) {
        if (node.kind == DspNodeKind::AnalogBlockSource) {
            // We need to find the corresponding AnalogBlockModel for this node
            // For now, we'll use a placeholder - in a real implementation, this would come from the circuit facade
            // The actual model would need to be provided from elsewhere
            Upp::String model_id = "PLACEHOLDER";

            // Find the analog model ID parameter from the node
            for (size_t i = 0; i < node.param_keys.size(); ++i) {
                if (node.param_keys[i] == "analog_model_id") {
                    model_id = Upp::String().Cat() << node.param_values[i];
                    break;
                }
            }

            // Create a placeholder analog block model for demonstration
            // In a real implementation, this would be passed into the Initialize function
            AnalogBlockModel placeholder_model;
            placeholder_model.id = model_id;
            placeholder_model.block_id = "PLACEHOLDER_BLOCK";
            placeholder_model.kind = AnalogBlockKind::RcOscillator;

            // Add a placeholder state variable
            AnalogStateVar v_out;
            v_out.name = "v_out";
            v_out.kind = AnalogStateKind::Voltage;
            v_out.value = 0.0;
            placeholder_model.state.push_back(v_out);

            // Add placeholder parameters
            AnalogParam r_param;
            r_param.name = "R";
            r_param.value = 10000.0;
            placeholder_model.params.push_back(r_param);

            AnalogParam c_param;
            c_param.name = "C";
            c_param.value = 1e-7;
            placeholder_model.params.push_back(c_param);

            placeholder_model.output_state_name = "v_out";
            placeholder_model.estimated_freq_hz = 1.0 / (2 * M_PI * 10000.0 * 1e-7); // ~159 Hz

            // Configure the solver
            AnalogSolverConfig config;
            config.sample_rate_hz = graph.sample_rate_hz;
            config.dt = 1.0 / graph.sample_rate_hz;
            config.integrator = "euler";

            // Initialize the analog solver
            auto solver_result = AnalogSolver::Initialize(placeholder_model, config);
            if (solver_result.ok) {
                state.analog_solver_states[node.id] = solver_result.data;
            } else {
                // If initialization failed, we can still continue but log an error
                // In a real implementation, this might be a fatal error depending on requirements
            }
        }
    }

    return Result<DspRuntimeState>::MakeOk(state);
}

Result<void> DspRuntime::Render(DspRuntimeState& state) {
    // Render all samples one by one
    for (int sample_index = 0; sample_index < state.graph.total_samples; sample_index++) {
        Result<void> result = RenderSample(state, sample_index);
        if (!result.ok) {
            return result;
        }
    }
    
    return Result<void>::MakeOk({});
}

Result<void> DspRuntime::RenderSample(DspRuntimeState& state, int sample_index) {
    // Compute time for the current sample
    double t = static_cast<double>(sample_index) / state.graph.sample_rate_hz;

    // Find the oscillator, pan_lfo, panner, and output nodes by ID
    const DspNode* osc_node = nullptr;
    const DspNode* pan_lfo_node = nullptr;
    const DspNode* panner_node = nullptr;
    const DspNode* output_node = nullptr;

    // Also find any AnalogBlockSource nodes
    std::vector<const DspNode*> analog_source_nodes;

    for (const auto& node : state.graph.nodes) {
        if (node.id == state.graph.osc_node_id) {
            osc_node = &node;
        } else if (node.id == state.graph.pan_lfo_node_id) {
            pan_lfo_node = &node;
        } else if (node.id == state.graph.panner_node_id) {
            panner_node = &node;
        } else if (node.id == state.graph.output_node_id) {
            output_node = &node;
        } else if (node.kind == DspNodeKind::AnalogBlockSource) {
            analog_source_nodes.push_back(&node);
        }
    }

    // Check which type of source we have - digital oscillator or analog block source
    double source_sample = 0.0;

    if (!analog_source_nodes.empty()) {
        // Use the first analog block source as the signal source
        const DspNode* analog_node = analog_source_nodes[0];

        // Find the analog solver state for this node
        auto it = state.analog_solver_states.find(analog_node->id);
        if (it != state.analog_solver_states.end()) {
            // Run one step of the analog solver to get the output sample
            auto step_result = AnalogSolver::Step(it->second);
            if (step_result.ok) {
                source_sample = static_cast<double>(step_result.data);
            } else {
                // If step failed, use 0.0 as the sample
                source_sample = 0.0;
            }
        } else {
            // If no solver state exists for this node, use 0.0
            source_sample = 0.0;
        }
    } else {
        // Use the traditional digital oscillator
        if (!osc_node) {
            return Result<void>::MakeError(
                ErrorCode::InternalError,
                "Oscillator node not found in DSP graph"
            );
        }

        // Find the oscillator frequency parameter
        double osc_freq = 440.0; // Default to 440 Hz if parameter not found
        for (size_t i = 0; i < osc_node->param_keys.size(); ++i) {
            if (osc_node->param_keys[i] == "frequency_hz") {
                osc_freq = osc_node->param_values[i];
                break;
            }
        }

        // Calculate oscillator sample (using phase accumulation for more accurate results)
        double osc_phase_increment = 2.0 * M_PI * osc_freq / state.graph.sample_rate_hz;
        state.osc_phase += osc_phase_increment;
        if (state.osc_phase > 2.0 * M_PI) {
            state.osc_phase -= 2.0 * M_PI; // Wrap phase
        }
        source_sample = std::sin(state.osc_phase);
    }

    // Handle the pan LFO - this can work with either digital or analog source
    double pan_value = 0.5; // Default center position

    if (pan_lfo_node) {
        // Find the pan LFO rate parameter
        double pan_lfo_rate = 0.25; // Default to 0.25 Hz if parameter not found
        for (size_t i = 0; i < pan_lfo_node->param_keys.size(); ++i) {
            if (pan_lfo_node->param_keys[i] == "rate_hz") {
                pan_lfo_rate = pan_lfo_node->param_values[i];
                break;
            }
        }

        // Calculate pan LFO value (using phase accumulation)
        double pan_lfo_phase_increment = 2.0 * M_PI * pan_lfo_rate / state.graph.sample_rate_hz;
        state.pan_lfo_phase += pan_lfo_phase_increment;
        if (state.pan_lfo_phase > 2.0 * M_PI) {
            state.pan_lfo_phase -= 2.0 * M_PI; // Wrap phase
        }
        pan_value = 0.5 * (1.0 + std::sin(state.pan_lfo_phase)); // 0..1 range
    }

    // Perform stereo panning
    double gainL = 1.0 - pan_value;
    double gainR = pan_value;
    double left_sample = source_sample * gainL;
    double right_sample = source_sample * gainR;

    // Write to output buffers
    if (sample_index < static_cast<int>(state.out_left.size()) &&
        sample_index < static_cast<int>(state.out_right.size())) {
        state.out_left[sample_index] = static_cast<float>(left_sample);
        state.out_right[sample_index] = static_cast<float>(right_sample);
    }

    return Result<void>::MakeOk({});
}

} // namespace ProtoVMCLI