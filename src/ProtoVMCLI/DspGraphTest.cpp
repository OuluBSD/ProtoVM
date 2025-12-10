#include "DspGraph.h"
#include "DspGraphBuilder.h"
#include "DspRuntime.h"
#include "AudioDsl.h"
#include "SessionTypes.h"
#include <iostream>
#include <vector>
#include <cmath>

namespace ProtoVMCLI {

// Test for DspGraphBuilder functionality
bool TestDspGraphBuilder() {
    std::cout << "Testing DspGraphBuilder..." << std::endl;
    
    // Create an AudioDslGraph
    AudioDslGraph audio_graph;
    audio_graph.block_id = "TEST_OSC";
    audio_graph.osc.id = "osc1";
    audio_graph.osc.frequency_hz = 440.0;
    audio_graph.pan_lfo.id = "pan_lfo1";
    audio_graph.pan_lfo.rate_hz = 0.25;
    audio_graph.output.sample_rate_hz = 48000.0;
    audio_graph.output.duration_sec = 1.0; // 1 second for simpler testing
    
    // Build the DSP graph
    auto result = DspGraphBuilder::BuildGraphFromAudioDsl(audio_graph);
    
    if (!result.ok) {
        std::cout << "ERROR: BuildGraphFromAudioDsl failed: " << result.error_message << std::endl;
        return false;
    }
    
    DspGraph graph = result.data;
    
    // Verify basic properties
    if (graph.graph_id != "DSP_TEST_OSC") {
        std::cout << "ERROR: Expected graph_id 'DSP_TEST_OSC', got '" << graph.graph_id << "'" << std::endl;
        return false;
    }
    
    if (graph.sample_rate_hz != 48000.0) {
        std::cout << "ERROR: Expected sample_rate_hz 48000.0, got " << graph.sample_rate_hz << std::endl;
        return false;
    }
    
    if (graph.total_samples != 48000) { // 48000 * 1.0
        std::cout << "ERROR: Expected total_samples 48000, got " << graph.total_samples << std::endl;
        return false;
    }
    
    // Check node counts
    if (graph.nodes.size() != 4) {
        std::cout << "ERROR: Expected 4 nodes, got " << graph.nodes.size() << std::endl;
        return false;
    }
    
    // Check connection counts
    if (graph.connections.size() != 4) {
        std::cout << "ERROR: Expected 4 connections, got " << graph.connections.size() << std::endl;
        return false;
    }
    
    // Verify specific node IDs exist
    bool has_osc_node = false, has_pan_lfo_node = false, has_panner_node = false, has_output_node = false;
    for (const auto& node : graph.nodes) {
        if (node.id == graph.osc_node_id) has_osc_node = true;
        if (node.id == graph.pan_lfo_node_id) has_pan_lfo_node = true;
        if (node.id == graph.panner_node_id) has_panner_node = true;
        if (node.id == graph.output_node_id) has_output_node = true;
    }
    
    if (!has_osc_node || !has_pan_lfo_node || !has_panner_node || !has_output_node) {
        std::cout << "ERROR: Missing required nodes in graph" << std::endl;
        return false;
    }
    
    std::cout << "DspGraphBuilder test PASSED" << std::endl;
    return true;
}

// Test for DspRuntime functionality
bool TestDspRuntime() {
    std::cout << "Testing DspRuntime..." << std::endl;
    
    // Create a simple AudioDslGraph
    AudioDslGraph audio_graph;
    audio_graph.block_id = "TEST_OSC";
    audio_graph.osc.id = "osc1";
    audio_graph.osc.frequency_hz = 1.0;  // 1 Hz for predictable testing
    audio_graph.pan_lfo.id = "pan_lfo1";
    audio_graph.pan_lfo.rate_hz = 0.25;  // 0.25 Hz for predictable testing
    audio_graph.output.sample_rate_hz = 100.0;  // Lower rate for simpler testing
    audio_graph.output.duration_sec = 0.1;  // 10 samples total
    
    // Build the DSP graph
    auto graph_result = DspGraphBuilder::BuildGraphFromAudioDsl(audio_graph);
    if (!graph_result.ok) {
        std::cout << "ERROR: Failed to build graph for runtime test" << std::endl;
        return false;
    }
    
    DspGraph graph = graph_result.data;
    
    // Initialize the runtime
    auto init_result = DspRuntime::Initialize(graph);
    if (!init_result.ok) {
        std::cout << "ERROR: Failed to initialize runtime: " << init_result.error_message << std::endl;
        return false;
    }
    
    DspRuntimeState state = init_result.data;
    
    // Check that output buffers are properly sized
    if (state.out_left.size() != 10 || state.out_right.size() != 10) {
        std::cout << "ERROR: Expected output buffers of size 10, got " 
                  << state.out_left.size() << " and " << state.out_right.size() << std::endl;
        return false;
    }
    
    // Render the audio
    auto render_result = DspRuntime::Render(state);
    if (!render_result.ok) {
        std::cout << "ERROR: Render failed: " << render_result.error_message << std::endl;
        return false;
    }
    
    // Check that output values are within valid range [-1, 1]
    bool all_valid = true;
    for (float sample : state.out_left) {
        if (sample < -1.0f || sample > 1.0f) {
            std::cout << "ERROR: Left sample out of range: " << sample << std::endl;
            all_valid = false;
        }
    }
    for (float sample : state.out_right) {
        if (sample < -1.0f || sample > 1.0f) {
            std::cout << "ERROR: Right sample out of range: " << sample << std::endl;
            all_valid = false;
        }
    }
    
    if (!all_valid) {
        std::cout << "ERROR: Some output samples are out of valid range [-1, 1]" << std::endl;
        return false;
    }
    
    std::cout << "DspRuntime test PASSED" << std::endl;
    return true;
}

// Test for DspRuntime single sample rendering
bool TestDspRuntimeSample() {
    std::cout << "Testing DspRuntime single sample rendering..." << std::endl;
    
    // Create an AudioDslGraph with a simple 1 Hz oscillator
    AudioDslGraph audio_graph;
    audio_graph.block_id = "TEST_OSC_SAMPLE";
    audio_graph.osc.id = "osc1";
    audio_graph.osc.frequency_hz = 1.0;
    audio_graph.pan_lfo.id = "pan_lfo1";
    audio_graph.pan_lfo.rate_hz = 0.25;
    audio_graph.output.sample_rate_hz = 10.0;  // 10 Hz for easy testing
    audio_graph.output.duration_sec = 0.1;    // 1 sample only
    
    // Build the DSP graph
    auto graph_result = DspGraphBuilder::BuildGraphFromAudioDsl(audio_graph);
    if (!graph_result.ok) {
        std::cout << "ERROR: Failed to build graph for sample test" << std::endl;
        return false;
    }
    
    DspGraph graph = graph_result.data;
    
    // Initialize the runtime
    auto init_result = DspRuntime::Initialize(graph);
    if (!init_result.ok) {
        std::cout << "ERROR: Failed to initialize runtime for sample test" << std::endl;
        return false;
    }
    
    DspRuntimeState state = init_result.data;
    
    // Render a single sample (index 0)
    auto sample_result = DspRuntime::RenderSample(state, 0);
    if (!sample_result.ok) {
        std::cout << "ERROR: RenderSample failed: " << sample_result.error_message << std::endl;
        return false;
    }
    
    // Verify the sample values are finite
    if (!std::isfinite(state.out_left[0]) || !std::isfinite(state.out_right[0])) {
        std::cout << "ERROR: Rendered sample values are not finite" << std::endl;
        return false;
    }
    
    std::cout << "DspRuntime single sample test PASSED" << std::endl;
    return true;
}

// Main test function
bool RunDspGraphTests() {
    std::cout << "\n=== Running DSP Graph and Runtime Tests ===" << std::endl;
    
    bool all_passed = true;
    
    all_passed &= TestDspGraphBuilder();
    all_passed &= TestDspRuntime();
    all_passed &= TestDspRuntimeSample();
    
    if (all_passed) {
        std::cout << "\n=== All DSP Graph and Runtime Tests PASSED ===" << std::endl;
    } else {
        std::cout << "\n=== Some DSP Graph and Runtime Tests FAILED ===" << std::endl;
    }
    
    return all_passed;
}

} // namespace ProtoVMCLI