#include "AnalogModel.h"
#include "AnalogBlockExtractor.h"
#include "AnalogSolver.h"
#include "CircuitGraph.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace ProtoVMCLI;

bool testAnalogModelCreation() {
    std::cout << "Testing AnalogModel creation..." << std::endl;
    
    AnalogBlockModel model;
    model.id = "TEST_MODEL";
    model.block_id = "TEST_BLOCK";
    model.kind = AnalogBlockKind::RcOscillator;
    
    // Add a state variable
    AnalogStateVar state;
    state.name = "v_out";
    state.kind = AnalogStateKind::Voltage;
    state.value = 0.0;
    model.state.push_back(state);
    
    // Add a parameter
    AnalogParam param;
    param.name = "R";
    param.value = 10000.0;
    model.params.push_back(param);
    
    model.output_state_name = "v_out";
    model.estimated_freq_hz = 159.15;
    
    // Test serialization
    assert(model.id == "TEST_MODEL");
    assert(model.kind == AnalogBlockKind::RcOscillator);
    assert(model.state.size() == 1);
    assert(model.state[0].name == "v_out");
    assert(model.params.size() == 1);
    assert(model.params[0].name == "R");
    assert(model.params[0].value == 10000.0);
    assert(model.output_state_name == "v_out");
    assert(std::abs(model.estimated_freq_hz - 159.15) < 0.01);
    
    std::cout << "AnalogModel creation test passed." << std::endl;
    return true;
}

bool testAnalogBlockExtractor() {
    std::cout << "Testing AnalogBlockExtractor..." << std::endl;
    
    // Create a mock CircuitGraph with an RC oscillator pattern
    CircuitGraph graph;
    graph.graph_id = "TEST_GRAPH";
    
    // Add some nodes that would represent an RC oscillator
    CircuitGraphNode resistor;
    resistor.id = "R1";
    resistor.name = "R1";
    resistor.kind = "Resistor";
    resistor.param_keys.push_back("resistance");
    resistor.param_values.push_back(10000.0); // 10kΩ
    graph.nodes.push_back(resistor);
    
    CircuitGraphNode capacitor;
    capacitor.id = "C1";
    capacitor.name = "C1";
    capacitor.kind = "Capacitor";
    capacitor.param_keys.push_back("capacitance");
    capacitor.param_values.push_back(1e-7); // 0.1μF
    graph.nodes.push_back(capacitor);
    
    CircuitGraphNode inverter;
    inverter.id = "U1";
    inverter.name = "U1";
    inverter.kind = "Inverter";
    graph.nodes.push_back(inverter);
    
    // Test extraction
    auto result = AnalogBlockExtractor::ExtractAnalogModelForBlock("TEST_BLOCK", graph);
    
    if (!result.ok) {
        std::cout << "Extraction failed: " << result.error_message << std::endl;
        return false;
    }
    
    const auto& model = result.data;
    assert(model.kind == AnalogBlockKind::RcOscillator);
    assert(model.state.size() >= 1); // Should have at least one state var
    assert(model.params.size() >= 2); // Should have R and C parameters
    
    // Check if parameters match
    bool has_R = false, has_C = false;
    for (const auto& param : model.params) {
        if (param.name == "R") {
            has_R = true;
            assert(std::abs(param.value - 10000.0) < 1.0); // Within 1Ω tolerance
        } else if (param.name == "C") {
            has_C = true;
            assert(std::abs(param.value - 1e-7) < 1e-9); // Within 1nF tolerance
        }
    }
    assert(has_R && has_C);
    
    std::cout << "AnalogBlockExtractor test passed." << std::endl;
    return true;
}

bool testAnalogSolver() {
    std::cout << "Testing AnalogSolver..." << std::endl;
    
    // Create a simple analog model for testing
    AnalogBlockModel model;
    model.id = "TEST_SOLVER_MODEL";
    model.block_id = "TEST_SOLVER_BLOCK";
    model.kind = AnalogBlockKind::RcOscillator;
    
    // Add state variables
    AnalogStateVar v_cap;
    v_cap.name = "v_cap";
    v_cap.kind = AnalogStateKind::Voltage;
    v_cap.value = 0.0;
    model.state.push_back(v_cap);
    
    AnalogStateVar v_out;
    v_out.name = "v_out";
    v_out.kind = AnalogStateKind::Voltage;
    v_out.value = 0.0;
    model.state.push_back(v_out);
    
    // Add parameters
    AnalogParam r_param;
    r_param.name = "R";
    r_param.value = 10000.0;
    model.params.push_back(r_param);
    
    AnalogParam c_param;
    c_param.name = "C";
    c_param.value = 1e-7;
    model.params.push_back(c_param);
    
    model.output_state_name = "v_out";
    model.estimated_freq_hz = 1.0 / (2 * M_PI * 10000.0 * 1e-7); // ~159 Hz
    
    // Configure solver
    AnalogSolverConfig config;
    config.sample_rate_hz = 48000.0;
    config.dt = 1.0 / 48000.0;
    config.integrator = "euler";
    
    // Test initialization
    auto init_result = AnalogSolver::Initialize(model, config);
    if (!init_result.ok) {
        std::cout << "Solver initialization failed: " << init_result.error_message << std::endl;
        return false;
    }
    
    AnalogSolverState state = init_result.data;
    
    // Test step function
    std::vector<float> samples;
    const int num_samples = 1000;
    samples.reserve(num_samples);
    
    for (int i = 0; i < num_samples; ++i) {
        auto step_result = AnalogSolver::Step(state);
        if (!step_result.ok) {
            std::cout << "Solver step failed: " << step_result.error_message << std::endl;
            return false;
        }
        samples.push_back(step_result.data);
        
        // Basic sanity checks
        assert(std::isfinite(step_result.data));
        assert(step_result.data >= -1.1 && step_result.data <= 1.1); // Reasonable audio range
    }
    
    // Check that we have some variation (not all zeros)
    float max_val = *std::max_element(samples.begin(), samples.end());
    float min_val = *std::min_element(samples.begin(), samples.end());
    assert(max_val > 0.01 || min_val < -0.01); // Should have some signal
    
    std::cout << "AnalogSolver test passed." << std::endl;
    return true;
}

bool testAnalogSolverRender() {
    std::cout << "Testing AnalogSolver render..." << std::endl;
    
    // Create a simple analog model for testing
    AnalogBlockModel model;
    model.id = "TEST_RENDER_MODEL";
    model.block_id = "TEST_RENDER_BLOCK";
    model.kind = AnalogBlockKind::SimpleFilter;
    
    // Add state variables
    AnalogStateVar v_in;
    v_in.name = "v_in";
    v_in.kind = AnalogStateKind::Voltage;
    v_in.value = 0.0;
    model.state.push_back(v_in);
    
    AnalogStateVar v_out;
    v_out.name = "v_out";
    v_out.kind = AnalogStateKind::Voltage;
    v_out.value = 0.0;
    model.state.push_back(v_out);
    
    // Add parameters
    AnalogParam r_param;
    r_param.name = "R";
    r_param.value = 1000.0;
    model.params.push_back(r_param);
    
    AnalogParam c_param;
    c_param.name = "C";
    c_param.value = 1e-6;
    model.params.push_back(c_param);
    
    model.output_state_name = "v_out";
    model.estimated_freq_hz = 159.0; // Approximate for this RC combination
    
    // Configure solver
    AnalogSolverConfig config;
    config.sample_rate_hz = 48000.0;
    config.dt = 1.0 / 48000.0;
    config.integrator = "euler";
    
    // Test initialization
    auto init_result = AnalogSolver::Initialize(model, config);
    if (!init_result.ok) {
        std::cout << "Solver initialization failed: " << init_result.error_message << std::endl;
        return false;
    }
    
    AnalogSolverState state = init_result.data;
    
    // Test render function
    std::vector<float> buffer;
    const int total_samples = 480; // 10ms at 48kHz
    auto render_result = AnalogSolver::Render(state, total_samples, buffer);
    
    if (!render_result.ok) {
        std::cout << "Solver render failed: " << render_result.error_message << std::endl;
        return false;
    }
    
    // Check buffer size and content
    assert(buffer.size() == static_cast<size_t>(total_samples));
    
    // Check for finite values
    for (const auto& sample : buffer) {
        assert(std::isfinite(sample));
    }
    
    std::cout << "AnalogSolver render test passed." << std::endl;
    return true;
}

int main() {
    std::cout << "Starting Analog Model Tests..." << std::endl;
    
    bool all_tests_passed = true;
    
    all_tests_passed &= testAnalogModelCreation();
    all_tests_passed &= testAnalogBlockExtractor();
    all_tests_passed &= testAnalogSolver();
    all_tests_passed &= testAnalogSolverRender();
    
    if (all_tests_passed) {
        std::cout << "All analog tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests failed!" << std::endl;
        return 1;
    }
}