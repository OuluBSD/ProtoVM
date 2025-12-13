#include "ProtoVMCLI/CircuitFacade.h"
#include "ProtoVMCLI/InstrumentGraph.h"
#include "ProtoVMCLI/PluginProjectExport.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

// Test CircuitFacade integration with plugin project export
void TestCircuitFacadeIntegration() {
    std::cout << "Testing CircuitFacade Plugin Project Export Integration...\n";
    
    // Create a temporary directory for testing
    fs::path temp_dir = fs::temp_directory_path() / "protovm_facade_test";
    if (fs::exists(temp_dir)) {
        fs::remove_all(temp_dir);
    }
    fs::create_directories(temp_dir);
    
    // Create a basic instrument graph for testing
    ProtoVMCLI::InstrumentGraph instrument;
    instrument.instrument_id = Upp::String("FACADE_TEST_INSTRUMENT");
    instrument.sample_rate_hz = 48000.0;
    instrument.voice_count = 2;
    
    // Set up voice template
    instrument.voice_template.id = Upp::String("main_voice");
    instrument.voice_template.analog_block_id = Upp::String("TEST_ANALOG_BLOCK");
    instrument.voice_template.digital_block_id = Upp::String("TEST_DIGITAL_BLOCK");
    instrument.voice_template.has_pan_lfo = true;
    instrument.voice_template.pan_lfo_hz = 0.25;
    
    // Set up note descriptor
    instrument.note.base_freq_hz = 440.0;
    instrument.note.velocity = 1.0;
    instrument.note.duration_sec = 3.0;
    
    // Set up voice
    ProtoVMCLI::VoiceConfig voice;
    voice.id = Upp::String("voice0");
    voice.detune_cents = 0.0;
    voice.use_analog_source = true;
    instrument.voices.push_back(voice);
    
    // Create a CircuitFacade instance (with no session store since we're not actually accessing a session)
    ProtoVMCLI::CircuitFacade facade;
    
    // Create plugin project export options
    PluginProjectExportOptions opts;
    opts.target = PluginTargetKind::Lv2;
    opts.plugin_name = Upp::String("FacadeTestPlugin");
    opts.plugin_id = Upp::String("http://facade.test.org/lv2/testplugin");
    opts.vendor = Upp::String("FacadeTestVendor");
    opts.version = Upp::String("1.0.0");
    opts.output_dir = Upp::String((temp_dir / "facade_lv2_test").string().c_str());
    opts.num_inputs = 0;
    opts.num_outputs = 2;
    opts.default_sample_rate = 48000;
    opts.default_voice_count = 2;
    
    // Mock session data for the test (these are just to satisfy the function signature)
    ProtoVMCLI::SessionMetadata session;
    session.session_id = 1;
    std::string session_dir = "/tmp/dummy_session_dir";
    std::string branch_name = "main";
    
    // Call the CircuitFacade method
    auto result = facade.ExportPluginProjectForInstrumentInBranch(
        session,
        session_dir,
        branch_name,
        instrument,
        opts
    );
    
    assert(result.ok && "CircuitFacade export should succeed");
    
    // Verify expected files exist
    std::string output_path = (temp_dir / "facade_lv2_test").string();
    assert(fs::exists(output_path + "/src/PluginWrapper.cpp") && "PluginWrapper.cpp should exist");
    assert(fs::exists(output_path + "/CMakeLists.txt") && "CMakeLists.txt should exist");
    assert(fs::exists(output_path + "/README.md") && "README.md should exist");
    assert(fs::exists(output_path + "/metadata/manifest.ttl") && "LV2 manifest.ttl should exist");
    
    std::cout << "  CircuitFacade integration test successful!\n";
    
    // Clean up temporary directory
    fs::remove_all(temp_dir);
}

int main() {
    try {
        TestCircuitFacadeIntegration();
        std::cout << "CircuitFacade Integration tests passed successfully!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Integration test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}