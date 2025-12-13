#include "AudioEngineCAbi.h"
#include "PluginSkeletonExport.h"
#include "InstrumentGraph.h"
#include <gtest/gtest.h>
#include <cmath>
#include <memory>

// Test the C ABI audio engine functionality
class AudioEngineCAbiTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up default configuration
        cfg.sample_rate = 48000;
        cfg.max_block_size = 1024;
        cfg.num_channels = 2;
        cfg.voice_count = 1;
    }

    void TearDown() override {
        // Clean up if needed
    }

    ProtoVM_AudioEngineConfig cfg = {};
};

// Test engine creation and destruction
TEST_F(AudioEngineCAbiTest, CreateAndDestroyEngine) {
    ProtoVM_AudioEngine* engine = ProtoVM_AudioEngine_Create(&cfg);
    ASSERT_NE(engine, nullptr);

    ProtoVM_AudioEngine_Destroy(engine);
    // Note: We can't really verify the engine is destroyed here,
    // but we can verify the function doesn't crash
}

// Test engine creation with invalid config
TEST_F(AudioEngineCAbiTest, CreateEngineWithInvalidConfig) {
    ProtoVM_AudioEngineConfig invalid_cfg = {};
    invalid_cfg.sample_rate = 0;  // Invalid sample rate
    invalid_cfg.max_block_size = 1024;
    invalid_cfg.num_channels = 2;
    invalid_cfg.voice_count = 1;

    ProtoVM_AudioEngine* engine = ProtoVM_AudioEngine_Create(&invalid_cfg);
    EXPECT_EQ(engine, nullptr);
}

// Test parameter setting and getting
TEST_F(AudioEngineCAbiTest, SetAndGetParams) {
    ProtoVM_AudioEngine* engine = ProtoVM_AudioEngine_Create(&cfg);
    ASSERT_NE(engine, nullptr);

    // Set some parameter values
    ProtoVM_AudioEngineParams params = {};
    params.values[PROTOVM_PARAM_MAIN_FREQ] = 440.0f;  // A note
    params.values[PROTOVM_PARAM_MAIN_GAIN] = 0.5f;    // Half volume
    params.values[PROTOVM_PARAM_PAN_DEPTH] = 0.5f;    // Center

    ProtoVM_AudioEngine_SetParams(engine, &params);

    // Process a small buffer to verify the engine is still functional
    float inL[64] = {};
    float inR[64] = {};
    float outL[64] = {};
    float outR[64] = {};

    ProtoVM_AudioEngine_Process(engine, inL, inR, outL, outR, 64);

    // Check that output is not all zeros (we expect some synthesized audio)
    bool has_signal = false;
    for (int i = 0; i < 64; i++) {
        if (outL[i] != 0.0f || outR[i] != 0.0f) {
            has_signal = true;
            break;
        }
    }
    EXPECT_TRUE(has_signal);

    // Verify output values are finite (not NaN or infinity)
    for (int i = 0; i < 64; i++) {
        EXPECT_FALSE(std::isnan(outL[i]));
        EXPECT_FALSE(std::isnan(outR[i]));
        EXPECT_FALSE(std::isinf(outL[i]));
        EXPECT_FALSE(std::isinf(outR[i]));
    }

    ProtoVM_AudioEngine_Destroy(engine);
}

// Test engine reset functionality
TEST_F(AudioEngineCAbiTest, ResetEngine) {
    ProtoVM_AudioEngine* engine = ProtoVM_AudioEngine_Create(&cfg);
    ASSERT_NE(engine, nullptr);

    // Process once
    float inL1[64] = {};
    float inR1[64] = {};
    float outL1[64] = {};
    float outR1[64] = {};
    ProtoVM_AudioEngine_Process(engine, inL1, inR1, outL1, outR1, 64);

    // Reset the engine
    ProtoVM_AudioEngine_Reset(engine);

    // Process again with same inputs
    float inL2[64] = {};
    float inR2[64] = {};
    float outL2[64] = {};
    float outR2[64] = {};
    ProtoVM_AudioEngine_Process(engine, inL2, inR2, outL2, outR2, 64);

    // Both outputs should be finite
    for (int i = 0; i < 64; i++) {
        EXPECT_FALSE(std::isnan(outL1[i]));
        EXPECT_FALSE(std::isnan(outR1[i]));
        EXPECT_FALSE(std::isinf(outL1[i]));
        EXPECT_FALSE(std::isinf(outL1[i]));
        EXPECT_FALSE(std::isnan(outL2[i]));
        EXPECT_FALSE(std::isnan(outR2[i]));
        EXPECT_FALSE(std::isinf(outL2[i]));
        EXPECT_FALSE(std::isinf(outR2[i]));
    }

    ProtoVM_AudioEngine_Destroy(engine);
}

// Test the plugin skeleton export functionality
class PluginSkeletonExportTest : public ::testing::Test {
protected:
    void SetUp() override {
        opts_vst3.target = PluginTargetKind::Vst3;
        opts_vst3.plugin_name = "TestPlugin";
        opts_vst3.plugin_id = "test.plugin.id";
        opts_vst3.vendor = "TestVendor";
        opts_vst3.num_inputs = 0;
        opts_vst3.num_outputs = 2;
        opts_vst3.emit_comment_banner = true;

        opts_lv2.target = PluginTargetKind::Lv2;
        opts_lv2.plugin_name = "TestPlugin";
        opts_lv2.plugin_id = "test.plugin.id";
        opts_lv2.vendor = "TestVendor";
        opts_lv2.num_inputs = 0;
        opts_lv2.num_outputs = 2;
        opts_lv2.emit_comment_banner = true;

        opts_clap.target = PluginTargetKind::Clap;
        opts_clap.plugin_name = "TestPlugin";
        opts_clap.plugin_id = "test.plugin.id";
        opts_clap.vendor = "TestVendor";
        opts_clap.num_inputs = 0;
        opts_clap.num_outputs = 2;
        opts_clap.emit_comment_banner = true;

        opts_ladspa.target = PluginTargetKind::Ladspa;
        opts_ladspa.plugin_name = "TestPlugin";
        opts_ladspa.plugin_id = "test.plugin.id";
        opts_ladspa.vendor = "TestVendor";
        opts_ladspa.num_inputs = 0;
        opts_ladspa.num_outputs = 2;
        opts_ladspa.emit_comment_banner = true;
    }

    PluginSkeletonOptions opts_vst3;
    PluginSkeletonOptions opts_lv2;
    PluginSkeletonOptions opts_clap;
    PluginSkeletonOptions opts_ladspa;
};

// Test VST3 skeleton export
TEST_F(PluginSkeletonExportTest, Vst3SkeletonExport) {
    auto result = PluginSkeletonExport::EmitPluginSkeletonSource(opts_vst3);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.data.empty());

    // Check that the output contains expected VST3 elements
    std::string source = result.data;
    EXPECT_NE(source.find("class TestPlugin"), std::string::npos);
    EXPECT_NE(source.find("Steinberg::Vst::SingleComponentEffect"), std::string::npos);
    EXPECT_NE(source.find("ProtoVM_AudioEngine"), std::string::npos);
}

// Test LV2 skeleton export
TEST_F(PluginSkeletonExportTest, Lv2SkeletonExport) {
    auto result = PluginSkeletonExport::EmitPluginSkeletonSource(opts_lv2);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.data.empty());

    // Check that the output contains expected LV2 elements
    std::string source = result.data;
    EXPECT_NE(source.find("LV2_Descriptor"), std::string::npos);
    EXPECT_NE(source.find("ProtoVM_AudioEngine"), std::string::npos);
    EXPECT_NE(source.find("connect_port"), std::string::npos);
}

// Test CLAP skeleton export
TEST_F(PluginSkeletonExportTest, ClapSkeletonExport) {
    auto result = PluginSkeletonExport::EmitPluginSkeletonSource(opts_clap);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.data.empty());

    // Check that the output contains expected CLAP elements
    std::string source = result.data;
    EXPECT_NE(source.find("clap_plugin"), std::string::npos);
    EXPECT_NE(source.find("ProtoVM_AudioEngine"), std::string::npos);
    EXPECT_NE(source.find("clap_process"), std::string::npos);
}

// Test LADSPA skeleton export
TEST_F(PluginSkeletonExportTest, LadspaSkeletonExport) {
    auto result = PluginSkeletonExport::EmitPluginSkeletonSource(opts_ladspa);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.data.empty());

    // Check that the output contains expected LADSPA elements
    std::string source = result.data;
    EXPECT_NE(source.find("LADSPA_Descriptor"), std::string::npos);
    EXPECT_NE(source.find("ProtoVM_AudioEngine"), std::string::npos);
    EXPECT_NE(source.find("LADSPA_Handle"), std::string::npos);
}

// Test invalid plugin target
TEST_F(PluginSkeletonExportTest, InvalidPluginTarget) {
    PluginSkeletonOptions invalid_opts;
    invalid_opts.target = static_cast<PluginTargetKind>(999);  // Invalid enum value
    invalid_opts.plugin_name = "TestPlugin";
    invalid_opts.plugin_id = "test.plugin.id";
    invalid_opts.vendor = "TestVendor";
    invalid_opts.num_inputs = 0;
    invalid_opts.num_outputs = 2;
    invalid_opts.emit_comment_banner = true;

    auto result = PluginSkeletonExport::EmitPluginSkeletonSource(invalid_opts);
    EXPECT_FALSE(result.ok);
}