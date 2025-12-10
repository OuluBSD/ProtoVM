#include "CodeEmitter.h"
#include "CodegenIr.h"
#include "CodegenCpp.h"
#include "AudioDsl.h"
#include "JsonIO.h"
#include "SessionTypes.h"  // For Result<T>
#include <cassert>
#include <iostream>
#include <string>

using namespace ProtoVMCLI;

// Test C++ class emission
void test_cpp_class_emission() {
    std::cout << "Testing C++ class emission..." << std::endl;
    
    // Create a simple CodegenModule
    CodegenModule module;
    module.id = "test_module";
    module.block_id = "TEST_BLOCK";
    module.is_oscillator_like = true;
    
    // Add a couple of state variables
    module.state.push_back(CodegenValue("phase", "float", 32, CodegenStorageKind::State));
    module.state.push_back(CodegenValue("freq", "float", 32, CodegenStorageKind::State));
    
    // Add some local variables
    module.locals.push_back(CodegenValue("temp", "float", 32, CodegenStorageKind::Local));
    
    // Add output variables
    module.outputs.push_back(CodegenValue("out", "float", 32, CodegenStorageKind::Output));
    
    // Configure CppClassOptions
    CppClassOptions options;
    options.class_name = "TestOsc";
    options.state_class_name = "TestState";
    options.namespace_name = "TestNs";
    options.generate_render_method = true;
    
    // Emit the C++ class
    auto result = CodeEmitter::EmitCppClassForModule(module, options);
    
    if (!result.ok) {
        std::cout << "ERROR: " << result.error_message << std::endl;
        assert(false);
    }
    
    std::cout << "Generated code:\n" << result.data << std::endl;
    
    // Verify expected content is in the output
    assert(result.data.find("namespace TestNs") != std::string::npos);
    assert(result.data.find("struct TestState") != std::string::npos);
    assert(result.data.find("class TestOsc") != std::string::npos);
    assert(result.data.find("void Step(") != std::string::npos);
    assert(result.data.find("void Render(") != std::string::npos);
    
    std::cout << "C++ class emission test passed!" << std::endl;
}

// Test audio demo emission
void test_audio_demo_emission() {
    std::cout << "Testing audio demo emission..." << std::endl;
    
    // Create a simple CodegenModule
    CodegenModule module;
    module.id = "osc_module";
    module.block_id = "OSC_BLOCK";
    module.is_oscillator_like = true;
    
    // Add a state variable
    module.state.push_back(CodegenValue("phase", "float", 32, CodegenStorageKind::State));
    
    // Configure CppClassOptions
    CppClassOptions class_opts;
    class_opts.class_name = "OscBlock";
    class_opts.state_class_name = "OscState";
    class_opts.namespace_name = "";
    
    // Configure AudioDslGraph
    AudioDslGraph graph;
    graph.block_id = "OSC_BLOCK";
    graph.osc.id = "osc1";
    graph.osc.frequency_hz = 440.0;
    graph.pan_lfo.id = "pan_lfo1";
    graph.pan_lfo.rate_hz = 0.25;
    graph.output.sample_rate_hz = 48000.0;
    graph.output.duration_sec = 3.0;
    
    // Emit the audio demo
    auto result = CodeEmitter::EmitAudioDemoForOscillator(module, class_opts, graph);
    
    if (!result.ok) {
        std::cout << "ERROR: " << result.error_message << std::endl;
        assert(false);
    }
    
    std::cout << "Audio demo code generated successfully" << std::endl;
    
    // Verify expected content is in the output
    assert(result.data.find("440") != std::string::npos);
    assert(result.data.find("0.25") != std::string::npos);
    assert(result.data.find("48000") != std::string::npos);
    assert(result.data.find("OscState") != std::string::npos);
    assert(result.data.find("OscBlock") != std::string::npos);
    assert(result.data.find("main()") != std::string::npos);
    
    std::cout << "Audio demo emission test passed!" << std::endl;
}

// Test Audio DSL JSON serialization
void test_audio_dsl_serialization() {
    std::cout << "Testing Audio DSL JSON serialization..." << std::endl;
    
    // Create an AudioDslGraph
    AudioDslGraph graph;
    graph.block_id = "TEST_OSC";
    graph.osc.id = "osc1";
    graph.osc.frequency_hz = 440.0;
    graph.pan_lfo.id = "pan_lfo1";
    graph.pan_lfo.rate_hz = 0.25;
    graph.output.sample_rate_hz = 48000.0;
    graph.output.duration_sec = 3.0;
    
    // Serialize to ValueMap
    Upp::ValueMap serialized = JsonIO::AudioDslGraphToValueMap(graph);
    
    // Verify the serialized data has expected keys
    assert(serialized.Get("block_id").ToString().ToStd() == "TEST_OSC");
    assert(serialized.Get("osc").Get("frequency_hz").GetDouble() == 440.0);
    assert(serialized.Get("pan_lfo").Get("rate_hz").GetDouble() == 0.25);
    assert(serialized.Get("output").Get("sample_rate_hz").GetDouble() == 48000.0);
    assert(serialized.Get("output").Get("duration_sec").GetDouble() == 3.0);
    
    std::cout << "Audio DSL serialization test passed!" << std::endl;
}

int main() {
    std::cout << "Running C++ Class & Audio DSL Tests..." << std::endl;
    
    test_cpp_class_emission();
    test_audio_demo_emission();
    test_audio_dsl_serialization();
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}