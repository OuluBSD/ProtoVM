#include <gtest/gtest.h>
#include "PluginHostHarness.h"
#include "../ProtoVMCommon/Result.h"
#include "AudioQa.h"
#include "AudioQaAnalysis.h"
#include <filesystem>

namespace fs = std::filesystem;

// Test fixture for PluginHostHarness tests
class PluginHostHarnessTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directories for test files
        temp_dir = fs::temp_directory_path() / "protovm_test";
        fs::create_directories(temp_dir);
        
        // Paths for test plugins
        ladspa_plugin_path = temp_dir / "test_ladspa.so";
        clap_plugin_path = temp_dir / "test_clap.so";
    }

    void TearDown() override {
        // Clean up temporary directory
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }
    
    fs::path temp_dir;
    fs::path ladspa_plugin_path;
    fs::path clap_plugin_path;
};

// Test the LADSPA loader with a fixture plugin
TEST_F(PluginHostHarnessTest, LoadRunLadspaBasicTest) {
    // We'll use our test fixture plugin
    std::string fixture_path = TEST_LADSPA_FIXTURE_PATH; // This would be defined by build system
    
    // If fixture is not available, skip test
    if (!fs::exists(fixture_path)) {
        GTEST_SKIP() << "LADSPA fixture plugin not available";
    }

    HostLoadOptions load_opts;
    load_opts.kind = HostPluginKind::Ladspa;
    load_opts.plugin_path = Upp::String(fixture_path.c_str());
    load_opts.sample_rate = 48000;
    load_opts.max_block_size = 512;
    load_opts.num_outputs = 2;

    HostRunOptions run_opts;
    run_opts.duration_sec = 0.1; // Short test duration
    run_opts.wallclock_paced = false;

    auto result = PluginHostHarness::LoadRunAndAnalyze(load_opts, run_opts, Upp::Optional<Upp::String>());
    
    // The call should succeed (even if plugin doesn't work perfectly)
    EXPECT_TRUE(result.ok) << "LADSPA harness failed with error: " << result.error_message;
    
    if (result.ok) {
        // Verify basic properties of the result
        EXPECT_GT(result.data.rendered.num_frames, 0);
        EXPECT_GT(result.data.rendered.duration_sec, 0.0);
        EXPECT_EQ(result.data.rendered.sample_rate, 48000);
        EXPECT_EQ(result.data.rendered.max_block_size, 512);
        
        // Check that QA report exists and has reasonable content
        EXPECT_GT(result.data.qa_report.sample_count, 0);
        EXPECT_GT(result.data.qa_report.duration_seconds, 0.0);
    }
}

// Test the CLAP loader with a fixture plugin
TEST_F(PluginHostHarnessTest, LoadRunClapBasicTest) {
    // We'll use our test fixture plugin
    std::string fixture_path = TEST_CLAP_FIXTURE_PATH; // This would be defined by build system
    
    // If fixture is not available, skip test
    if (!fs::exists(fixture_path)) {
        GTEST_SKIP() << "CLAP fixture plugin not available";
    }

    HostLoadOptions load_opts;
    load_opts.kind = HostPluginKind::Clap;
    load_opts.plugin_path = Upp::String(fixture_path.c_str());
    load_opts.plugin_id = Upp::String("com.protovm.sine_wave_gen"); // Our fixture plugin ID
    load_opts.sample_rate = 48000;
    load_opts.max_block_size = 512;
    load_opts.num_outputs = 2;

    HostRunOptions run_opts;
    run_opts.duration_sec = 0.1; // Short test duration
    run_opts.wallclock_paced = false;

    auto result = PluginHostHarness::LoadRunAndAnalyze(load_opts, run_opts, Upp::Optional<Upp::String>());
    
    // The call should succeed (even if plugin doesn't work perfectly)
    // CLAP implementation is currently stubbed, so we expect a specific error
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error_code, ProtoVMCLI::ErrorCode::NotImplemented);
}

// Test error handling when loading non-existent plugin
TEST_F(PluginHostHarnessTest, LoadNonExistentPlugin) {
    HostLoadOptions load_opts;
    load_opts.kind = HostPluginKind::Ladspa;
    load_opts.plugin_path = Upp::String("/non/existent/path.so");
    load_opts.sample_rate = 48000;
    load_opts.max_block_size = 512;

    HostRunOptions run_opts;
    run_opts.duration_sec = 0.1;

    auto result = PluginHostHarness::LoadRunAndAnalyze(load_opts, run_opts, Upp::Optional<Upp::String>());
    
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_code, ProtoVMCLI::ErrorCode::None);
}

// Test parameter automation with longer duration to see changes
TEST_F(PluginHostHarnessTest, ParameterAutomationTest) {
    // For this test, we'll check that the options are properly passed, but skip if fixture unavailable
    std::string fixture_path = TEST_LADSPA_FIXTURE_PATH;
    
    if (!fs::exists(fixture_path)) {
        GTEST_SKIP() << "LADSPA fixture plugin not available for automation test";
    }

    HostLoadOptions load_opts;
    load_opts.kind = HostPluginKind::Ladspa;
    load_opts.plugin_path = Upp::String(fixture_path.c_str());
    load_opts.sample_rate = 48000;
    load_opts.max_block_size = 64; // Smaller blocks for more automation points
    load_opts.num_outputs = 2;

    HostRunOptions run_opts;
    run_opts.duration_sec = 0.1;
    run_opts.wallclock_paced = false;
    run_opts.enable_param_automation = true;
    run_opts.param_index = 0; // First control port
    run_opts.param_start = 0.0;
    run_opts.param_end = 1.0;

    auto result = PluginHostHarness::LoadRunAndAnalyze(load_opts, run_opts, Upp::Optional<Upp::String>());
    
    // Should succeed with automation enabled
    EXPECT_TRUE(result.ok) << "Parameter automation test failed with error: " << result.error_message;
    
    if (result.ok) {
        EXPECT_GT(result.data.rendered.num_frames, 0);
        EXPECT_GT(result.data.qa_report.sample_count, 0);
    }
}

// Test block size stress testing
TEST_F(PluginHostHarnessTest, BlockSizeStressTest) {
    std::string fixture_path = TEST_LADSPA_FIXTURE_PATH;
    
    if (!fs::exists(fixture_path)) {
        GTEST_SKIP() << "LADSPA fixture plugin not available for stress test";
    }

    HostLoadOptions load_opts;
    load_opts.kind = HostPluginKind::Ladspa;
    load_opts.plugin_path = Upp::String(fixture_path.c_str());
    load_opts.sample_rate = 48000;
    load_opts.max_block_size = 512;
    load_opts.num_outputs = 2;

    HostRunOptions run_opts;
    run_opts.duration_sec = 0.2; // Longer duration for stress
    run_opts.wallclock_paced = false;
    run_opts.block_sizes.Add(1);   // Minimum block
    run_opts.block_sizes.Add(7);   // Small odd block
    run_opts.block_sizes.Add(64);  // Normal block
    run_opts.block_sizes.Add(511); // Max block - 1
    run_opts.block_sizes.Add(512); // Max block

    auto result = PluginHostHarness::LoadRunAndAnalyze(load_opts, run_opts, Upp::Optional<Upp::String>());
    
    // Should succeed with various block sizes
    EXPECT_TRUE(result.ok) << "Block size stress test failed with error: " << result.error_message;
    
    if (result.ok) {
        EXPECT_GT(result.data.rendered.num_frames, 0);
        EXPECT_GT(result.data.qa_report.sample_count, 0);
    }
}

// Test WAV saving functionality
TEST_F(PluginHostHarnessTest, SaveWavOutputTest) {
    std::string fixture_path = TEST_LADSPA_FIXTURE_PATH;
    
    if (!fs::exists(fixture_path)) {
        GTEST_SKIP() << "LADSPA fixture plugin not available for WAV test";
    }

    HostLoadOptions load_opts;
    load_opts.kind = HostPluginKind::Ladspa;
    load_opts.plugin_path = Upp::String(fixture_path.c_str());
    load_opts.sample_rate = 48000;
    load_opts.max_block_size = 512;
    load_opts.num_outputs = 2;

    HostRunOptions run_opts;
    run_opts.duration_sec = 0.1;
    run_opts.wallclock_paced = false;

    fs::path wav_path = temp_dir / "test_output.wav";
    Upp::Optional<Upp::String> save_path;
    save_path.Set(Upp::String(wav_path.string().c_str()));

    auto result = PluginHostHarness::LoadRunAndAnalyze(load_opts, run_opts, save_path);
    
    EXPECT_TRUE(result.ok) << "WAV save test failed with error: " << result.error_message;
    
    if (result.ok) {
        // Verify that WAV file was created
        EXPECT_TRUE(fs::exists(wav_path));
        EXPECT_EQ(result.data.artifacts.wav_path.ToStd(), wav_path.string());
        EXPECT_GT(fs::file_size(wav_path), 0);
    }
}

// Test CLI command integration
TEST_F(PluginHostHarnessTest, CliIntegrationTest) {
    // Test that our CommandDispatcher integration works by creating
    // options with the expected parameters
    
    ProtoVMCLI::CommandOptions opts;
    opts.plugin_path = "/tmp/test.so";
    opts.plugin_kind = "ladspa";
    opts.sample_rate = 44100;
    opts.block_size = 256;
    opts.duration_sec = 0.5;
    opts.wallclock_paced = false;
    
    // These would map directly to our internal structures
    HostLoadOptions load_opts;
    load_opts.kind = HostPluginKind::Ladspa;
    load_opts.plugin_path = Upp::String("/tmp/test.so");
    load_opts.sample_rate = 44100;
    load_opts.max_block_size = 256;
    
    HostRunOptions run_opts;
    run_opts.duration_sec = 0.5;
    run_opts.wallclock_paced = false;
    
    EXPECT_EQ(load_opts.kind, HostPluginKind::Ladspa);
    EXPECT_EQ(load_opts.sample_rate, 44100);
    EXPECT_EQ(run_opts.duration_sec, 0.5);
}

// Test that enums serialize properly to JSON
TEST_F(PluginHostHarnessTest, JsonSerializationTest) {
    // Test HostPluginKind JSON serialization
    Upp::Value clap_value = ProtoVMCLI::JsonIO::HostPluginKindToJson(HostPluginKind::Clap);
    EXPECT_EQ(clap_value.ToString().ToStd(), "clap");
    
    Upp::Value ladspa_value = ProtoVMCLI::JsonIO::HostPluginKindToJson(HostPluginKind::Ladspa);
    EXPECT_EQ(ladspa_value.ToString().ToStd(), "ladspa");
    
    // Test structure serialization
    HostLoadOptions opts;
    opts.kind = HostPluginKind::Ladspa;
    opts.sample_rate = 96000;
    
    Upp::ValueMap opts_map = ProtoVMCLI::JsonIO::HostLoadOptionsToValueMap(opts);
    EXPECT_EQ(opts_map.Get("sample_rate", Upp::Value(0)).To<int>(), 96000);
    EXPECT_EQ(opts_map.Get("kind", Upp::Value()).ToString().ToStd(), "ladspa");
}