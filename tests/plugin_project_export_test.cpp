#include "ProtoVMCLI/PluginProjectExport.h"
#include "ProtoVMCLI/InstrumentGraph.h"
#include "ProtoVMCLI/InstrumentBuilder.h"
#include "ProtoVMCLI/PluginSkeletonExport.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// Test fixture for plugin project export tests
void TestPluginProjectExport() {
    std::cout << "Testing Plugin Project Export functionality...\n";
    
    // Create a temporary directory for testing
    fs::path temp_dir = fs::temp_directory_path() / "protovm_plugin_test";
    if (fs::exists(temp_dir)) {
        fs::remove_all(temp_dir);
    }
    fs::create_directories(temp_dir);
    
    // Create a basic instrument graph for testing
    ProtoVMCLI::InstrumentGraph instrument;
    instrument.instrument_id = Upp::String("TEST_INSTRUMENT");
    instrument.sample_rate_hz = 48000.0;
    instrument.voice_count = 4;
    
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
    
    // Set up voices
    ProtoVMCLI::VoiceConfig voice;
    voice.id = Upp::String("voice0");
    voice.detune_cents = 0.0;
    voice.use_analog_source = true;
    instrument.voices.push_back(voice);
    
    // Test 1: Export VST3 project
    {
        std::cout << "  Testing VST3 project export...\n";
        
        PluginProjectExportOptions opts;
        opts.target = PluginTargetKind::Vst3;
        opts.plugin_name = Upp::String("TestVst3Plugin");
        opts.plugin_id = Upp::String("com.test.vst3.testplugin");
        opts.vendor = Upp::String("TestVendor");
        opts.version = Upp::String("1.0.0");
        opts.output_dir = Upp::String((temp_dir / "vst3_test").string().c_str());
        opts.num_inputs = 0;
        opts.num_outputs = 2;
        opts.default_sample_rate = 48000;
        opts.default_voice_count = 4;
        
        auto result = PluginProjectExport::ExportPluginProject(instrument, opts);
        assert(result.ok && "VST3 export should succeed");
        
        // Verify expected files exist
        std::string output_path = (temp_dir / "vst3_test").string();
        assert(fs::exists(output_path + "/src/PluginWrapper.cpp") && "PluginWrapper.cpp should exist");
        assert(fs::exists(output_path + "/CMakeLists.txt") && "CMakeLists.txt should exist");
        assert(fs::exists(output_path + "/README.md") && "README.md should exist");
        std::cout << "    VST3 project export successful\n";
    }
    
    // Test 2: Export LV2 project
    {
        std::cout << "  Testing LV2 project export...\n";
        
        PluginProjectExportOptions opts;
        opts.target = PluginTargetKind::Lv2;
        opts.plugin_name = Upp::String("TestLv2Plugin");
        opts.plugin_id = Upp::String("http://test.org/lv2/testplugin");
        opts.vendor = Upp::String("TestVendor");
        opts.version = Upp::String("1.0.0");
        opts.output_dir = Upp::String((temp_dir / "lv2_test").string().c_str());
        opts.num_inputs = 0;
        opts.num_outputs = 2;
        opts.default_sample_rate = 48000;
        opts.default_voice_count = 4;
        
        auto result = PluginProjectExport::ExportPluginProject(instrument, opts);
        assert(result.ok && "LV2 export should succeed");
        
        // Verify expected files exist
        std::string output_path = (temp_dir / "lv2_test").string();
        assert(fs::exists(output_path + "/src/PluginWrapper.cpp") && "PluginWrapper.cpp should exist");
        assert(fs::exists(output_path + "/CMakeLists.txt") && "CMakeLists.txt should exist");
        assert(fs::exists(output_path + "/README.md") && "README.md should exist");
        assert(fs::exists(output_path + "/metadata/manifest.ttl") && "LV2 manifest.ttl should exist");
        std::cout << "    LV2 project export successful\n";
    }
    
    // Test 3: Export CLAP project
    {
        std::cout << "  Testing CLAP project export...\n";
        
        PluginProjectExportOptions opts;
        opts.target = PluginTargetKind::Clap;
        opts.plugin_name = Upp::String("TestClapPlugin");
        opts.plugin_id = Upp::String("com.test.clap.testplugin");
        opts.vendor = Upp::String("TestVendor");
        opts.version = Upp::String("1.0.0");
        opts.output_dir = Upp::String((temp_dir / "clap_test").string().c_str());
        opts.num_inputs = 0;
        opts.num_outputs = 2;
        opts.default_sample_rate = 48000;
        opts.default_voice_count = 4;
        
        auto result = PluginProjectExport::ExportPluginProject(instrument, opts);
        assert(result.ok && "CLAP export should succeed");
        
        // Verify expected files exist
        std::string output_path = (temp_dir / "clap_test").string();
        assert(fs::exists(output_path + "/src/PluginWrapper.cpp") && "PluginWrapper.cpp should exist");
        assert(fs::exists(output_path + "/CMakeLists.txt") && "CMakeLists.txt should exist");
        assert(fs::exists(output_path + "/README.md") && "README.md should exist");
        std::cout << "    CLAP project export successful\n";
    }
    
    // Test 4: Export LADSPA project
    {
        std::cout << "  Testing LADSPA project export...\n";
        
        PluginProjectExportOptions opts;
        opts.target = PluginTargetKind::Ladspa;
        opts.plugin_name = Upp::String("TestLadspaPlugin");
        opts.plugin_id = Upp::String("testladspa.1234");
        opts.vendor = Upp::String("TestVendor");
        opts.version = Upp::String("1.0.0");
        opts.output_dir = Upp::String((temp_dir / "ladspa_test").string().c_str());
        opts.num_inputs = 0;
        opts.num_outputs = 2;
        opts.default_sample_rate = 48000;
        opts.default_voice_count = 4;
        
        auto result = PluginProjectExport::ExportPluginProject(instrument, opts);
        assert(result.ok && "LADSPA export should succeed");
        
        // Verify expected files exist
        std::string output_path = (temp_dir / "ladspa_test").string();
        assert(fs::exists(output_path + "/src/PluginWrapper.cpp") && "PluginWrapper.cpp should exist");
        assert(fs::exists(output_path + "/CMakeLists.txt") && "CMakeLists.txt should exist");
        assert(fs::exists(output_path + "/README.md") && "README.md should exist");
        std::cout << "    LADSPA project export successful\n";
    }
    
    // Test 5: Verify content of generated files contains expected plugin names/IDs
    {
        std::cout << "  Testing content of generated files...\n";
        
        PluginProjectExportOptions opts;
        opts.target = PluginTargetKind::Vst3;
        opts.plugin_name = Upp::String("ContentTestPlugin");
        opts.plugin_id = Upp::String("com.test.content.testplugin");
        opts.vendor = Upp::String("ContentTestVendor");
        opts.version = Upp::String("2.1.0");
        opts.output_dir = Upp::String((temp_dir / "content_test").string().c_str());
        opts.num_inputs = 0;
        opts.num_outputs = 2;
        opts.default_sample_rate = 48000;
        opts.default_voice_count = 4;
        
        auto result = PluginProjectExport::ExportPluginProject(instrument, opts);
        assert(result.ok && "Content test export should succeed");
        
        // Read generated wrapper file to check content
        std::string wrapper_path = (temp_dir / "content_test/src/PluginWrapper.cpp").string();
        std::ifstream wrapper_file(wrapper_path);
        assert(wrapper_file.is_open() && "Generated wrapper file should be readable");
        
        std::string content((std::istreambuf_iterator<char>(wrapper_file)), std::istreambuf_iterator<char>());
        wrapper_file.close();
        
        // Check that the plugin name appears in the content
        assert(content.find("ContentTestPlugin") != std::string::npos && "Plugin name should appear in generated content");
        assert(content.find("com.test.content.testplugin") != std::string::npos && "Plugin ID should appear in generated content");
        assert(content.find("ContentTestVendor") != std::string::npos && "Vendor should appear in generated content");
        
        std::cout << "    Content verification successful\n";
    }
    
    // Clean up temporary directory
    fs::remove_all(temp_dir);
    
    std::cout << "Plugin Project Export tests passed!\n";
}

int main() {
    try {
        TestPluginProjectExport();
        std::cout << "All tests passed successfully!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}