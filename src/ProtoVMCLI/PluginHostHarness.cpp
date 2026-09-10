#include "PluginHostHarness.h"
#include "../ProtoVMCommon/WaveWriter.h"
#include "AudioQaAnalysis.h"
#include "AudioQaScenario.h"

#include <dlfcn.h>
#include <chrono>
#include <thread>

#ifdef PLATFORM_POSIX
#include <unistd.h>
#include <sys/wait.h>
#endif

// LADSPA header definitions (to avoid needing the actual LADSPA header)
typedef float LADSPA_Data;

#define LADSPA_PORT_INPUT          0x1
#define LADSPA_PORT_OUTPUT         0x2
#define LADSPA_PORT_CONTROL        0x4
#define LADSPA_PORT_AUDIO          0x8

#define LADSPA_IS_PORT_INPUT(x)    ((x) & LADSPA_PORT_INPUT)
#define LADSPA_IS_PORT_OUTPUT(x)   ((x) & LADSPA_PORT_OUTPUT)
#define LADSPA_IS_PORT_CONTROL(x)  ((x) & LADSPA_PORT_CONTROL)
#define LADSPA_IS_PORT_AUDIO(x)    ((x) & LADSPA_PORT_AUDIO)
#define LADSPA_PORT_HAS_DEFAULT    0x10

typedef unsigned long LADSPA_Properties;
typedef unsigned long LADSPA_PortDescriptor;
typedef unsigned long LADSPA_PortIndex;

typedef struct _LADSPA_Descriptor {
  const char * Label;
  const char * Name;
  const char * Maker;
  const char * Copyright;
  unsigned long UniqueID;
  unsigned long PortCount;
  const LADSPA_PortDescriptor * PortDescriptors;
  const char ** PortNames;
  const LADSPA_Data * PortRangeHints;
  void (*activate)(void * Instance);
  void (*cleanup)(void * Instance);
  void (*connect_port)(void * Instance, unsigned long Port, LADSPA_Data * DataLocation);
  void (*deactivate)(void * Instance);
  LADSPA_Handle (*instantiate)(const struct _LADSPA_Descriptor * Descriptor, unsigned long SampleRate);
  void (*run)(LADSPA_Handle Instance, unsigned long SampleCount);
  void (*run_adding)(LADSPA_Handle Instance, unsigned long SampleCount);
  void (*set_run_adding_gain)(LADSPA_Handle Instance, LADSPA_Data Gain);
} LADSPA_Descriptor;

typedef void * LADSPA_Handle;

namespace Upp {

// Cross-platform dynamic library utilities
void* DylibOpen(const String& path) {
    void* handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        LOG("Failed to load library: " << dlerror());
    }
    return handle;
}

void* DylibSym(void* handle, const String& name) {
    void* sym = dlsym(handle, name);
    if (!sym) {
        LOG("Failed to find symbol: " << dlerror());
    }
    return sym;
}

void DylibClose(void* handle) {
    dlclose(handle);
}

// Forward declarations for plugin loading functions
static Result<HostRunResult> LoadRunLadspa(const HostLoadOptions& load, const HostRunOptions& run, const Optional<String>& save_wav_path);
static Result<HostRunResult> LoadRunClap(const HostLoadOptions& load, const HostRunOptions& run, const Optional<String>& save_wav_path);

// Main implementation
Result<HostRunResult> PluginHostHarness::LoadRunAndAnalyze(
    const HostLoadOptions& load,
    const HostRunOptions& run,
    const Optional<String>& save_wav_path)
{
    switch (load.kind) {
    case HostPluginKind::Ladspa:
        return LoadRunLadspa(load, run, save_wav_path);
    case HostPluginKind::Clap:
        return LoadRunClap(load, run, save_wav_path);
    case HostPluginKind::Lv2:
        return LoadRunLv2(load, run, save_wav_path);
    case HostPluginKind::Vst3:
        return LoadRunVst3(load, run, save_wav_path);
    default:
        return Result<HostRunResult>::Error(ErrorCode::InvalidInput, "Unsupported plugin kind");
    }
}

// LADSPA implementation
static Result<HostRunResult> LoadRunLadspa(const HostLoadOptions& load, const HostRunOptions& run, const Optional<String>& save_wav_path) {
    // Load the library
    void* handle = DylibOpen(load.plugin_path);
    if (!handle) {
        return Result<HostRunResult>::Error(ErrorCode::ExternalLoadFailure, "Failed to load LADSPA plugin library");
    }

    // Get the LADSPA descriptor function
    typedef const struct _LADSPA_Descriptor *(*LadspaDescriptorFunc)(unsigned long Index);
    auto desc_func = (LadspaDescriptorFunc)DylibSym(handle, "ladspa_descriptor");
    if (!desc_func) {
        DylibClose(handle);
        return Result<HostRunResult>::Error(ErrorCode::ExternalLoadFailure, "Failed to find ladspa_descriptor function in plugin");
    }

    // Find the plugin
    unsigned long plugin_index = 0;
    if (!load.plugin_id.IsEmpty()) {
        if (IsNumber(load.plugin_id)) {
            plugin_index = atoi(load.plugin_id);
        } else {
            // If plugin_id is not numeric, we could look for named plugins, but for now just use first
            LOG("Warning: Non-numeric plugin_id for LADSPA, using index 0");
        }
    }

    const struct _LADSPA_Descriptor* plugin_desc = desc_func(plugin_index);
    if (!plugin_desc) {
        DylibClose(handle);
        return Result<HostRunResult>::Error(ErrorCode::ExternalLoadFailure, "Failed to get LADSPA descriptor for plugin");
    }

    // Count ports: audio inputs, outputs, and control ports
    int audio_input_count = 0;
    int audio_output_count = 0;
    int control_port_count = 0;
    Vector<int> audio_input_indices;
    Vector<int> audio_output_indices;
    Vector<int> control_indices;

    for (unsigned long i = 0; i < plugin_desc->PortCount; ++i) {
        LADSPA_PortDescriptor port_desc = plugin_desc->PortDescriptors[i];
        if (LADSPA_IS_PORT_AUDIO(port_desc)) {
            if (LADSPA_IS_PORT_INPUT(port_desc)) {
                audio_input_count++;
                audio_input_indices.Add(i);
            } else if (LADSPA_IS_PORT_OUTPUT(port_desc)) {
                audio_output_count++;
                audio_output_indices.Add(i);
            }
        } else if (LADSPA_IS_PORT_CONTROL(port_desc)) {
            control_port_count++;
            control_indices.Add(i);
        }
    }

    // Check if we have at least one audio output
    if (audio_output_count == 0) {
        DylibClose(handle);
        return Result<HostRunResult>::Error(ErrorCode::InvalidState, "LADSPA plugin has no audio outputs");
    }

    // Create instance
    LADSPA_Handle instance = plugin_desc->instantiate(plugin_desc, load.sample_rate);
    if (!instance) {
        DylibClose(handle);
        return Result<HostRunResult>::Error(ErrorCode::ExternalLoadFailure, "Failed to instantiate LADSPA plugin");
    }

    // Connect ports
    Vector<const LADSPA_Data*> audio_inputs(audio_input_count);
    Vector<LADSPA_Data*> audio_outputs(audio_output_count);
    Vector<LADSPA_Data*> controls(control_port_count);

    // Allocate input buffers (all silent initially)
    for (int i = 0; i < audio_input_count; ++i) {
        audio_inputs[i] = new LADSPA_Data[load.max_block_size];
        memset((void*)audio_inputs[i], 0, sizeof(LADSPA_Data) * load.max_block_size);
    }

    // Allocate output buffers
    for (int i = 0; i < audio_output_count; ++i) {
        audio_outputs[i] = new LADSPA_Data[load.max_block_size];
    }

    // Connect audio inputs and outputs
    for (int i = 0; i < audio_input_count; ++i) {
        plugin_desc->connect_port(instance, audio_input_indices[i], (LADSPA_Data*)audio_inputs[i]);
    }
    for (int i = 0; i < audio_output_count; ++i) {
        plugin_desc->connect_port(instance, audio_output_indices[i], audio_outputs[i]);
    }

    // Connect control ports to their default values
    for (int i = 0; i < control_port_count; ++i) {
        LADSPA_Data default_value = plugin_desc->PortDescriptors[i] & LADSPA_PORT_HAS_DEFAULT ? 
                                    plugin_desc->PortRangeHints[i].DefaultValue : 
                                    0.0f;
        controls[i] = new LADSPA_Data(1);  // Single value pointer to hold current control value
        *controls[i] = default_value;
        plugin_desc->connect_port(instance, control_indices[i], controls[i]);
    }

    // Activate if available
    if (plugin_desc->activate) {
        plugin_desc->activate(instance);
    }

    // Calculate total frames needed
    int total_frames = int(load.sample_rate * run.duration_sec);
    
    // Prepare host audio buffers (always stereo for now)
    Vector<float> host_out_l;
    Vector<float> host_out_r;
    host_out_l.SetCount(total_frames);
    host_out_r.SetCount(total_frames);
    
    // Initialize with zeros
    for (int i = 0; i < total_frames; ++i) {
        host_out_l[i] = 0.0f;
        host_out_r[i] = 0.0f;
    }

    // Run the render loop
    int frame_idx = 0;
    int block_idx = 0;
    int current_block_size;
    
    while (frame_idx < total_frames) {
        // Determine current block size
        if (run.block_sizes.GetCount() > 0) {
            current_block_size = min(run.block_sizes[block_idx % run.block_sizes.GetCount()], load.max_block_size);
            block_idx++;
        } else {
            current_block_size = min(load.max_block_size, total_frames - frame_idx);
        }

        // Apply parameter automation if enabled
        if (run.enable_param_automation && run.param_index >= 0 && run.param_index < control_port_count) {
            double progress = double(frame_idx) / double(total_frames);
            double current_val = run.param_start + progress * (run.param_end - run.param_start);
            *controls[run.param_index] = LADSPA_Data(current_val);
        }

        // Zero output buffers for this block
        for (int i = 0; i < audio_output_count; ++i) {
            memset(audio_outputs[i], 0, sizeof(LADSPA_Data) * current_block_size);
        }

        // Process the block
        plugin_desc->run(instance, current_block_size);

        // Copy outputs to host buffers
        // If mono output, duplicate to both channels
        if (audio_output_count == 1) {
            for (int i = 0; i < current_block_size && (frame_idx + i) < total_frames; ++i) {
                host_out_l[frame_idx + i] = audio_outputs[0][i];
                host_out_r[frame_idx + i] = audio_outputs[0][i];
            }
        } else {
            // Take first two outputs as left/right (or fewer if only one available)
            for (int i = 0; i < current_block_size && (frame_idx + i) < total_frames; ++i) {
                host_out_l[frame_idx + i] = (audio_output_count > 0) ? audio_outputs[0][i] : 0.0f;
                host_out_r[frame_idx + i] = (audio_output_count > 1) ? audio_outputs[1][i] : 0.0f;
            }
        }

        // Optional wallclock pacing
        if (run.wallclock_paced) {
            std::this_thread::sleep_for(std::chrono::microseconds(
                (long long)(current_block_size * 1000000.0 / load.sample_rate)
            ));
        }

        frame_idx += current_block_size;
    }

    // Clean up LADSPA resources
    if (plugin_desc->deactivate) {
        plugin_desc->deactivate(instance);
    }
    plugin_desc->cleanup(instance);
    
    // Free allocated buffers
    for (int i = 0; i < audio_input_count; ++i) {
        delete[] (LADSPA_Data*)audio_inputs[i];
    }
    for (int i = 0; i < audio_output_count; ++i) {
        delete[] audio_outputs[i];
    }
    for (int i = 0; i < control_port_count; ++i) {
        delete controls[i];
    }
    
    DylibClose(handle);

    // Perform QA analysis
    AudioQaScenario scenario;
    if (run.enable_param_automation) {
        // Use appropriate scenario for parameter sweeps
        scenario = AudioQaScenario::ParameterSweep;
    } else {
        // Default to general audio check
        scenario = AudioQaScenario::GeneralAudioCheck;
    }

    AudioQaReport qa_report = AudioQaAnalysis::AnalyzeStereoBuffer(host_out_l, host_out_r, load.sample_rate, scenario);

    // Save WAV if requested
    String wav_path;
    if (save_wav_path.IsSet()) {
        WaveWriter writer;
        if (writer.Open(save_wav_path.Get())) {
            writer.WriteStereo(host_out_l, host_out_r);
            writer.Close();
            wav_path = save_wav_path.Get();
        }
    }

    // Construct result
    HostRunResult result;
    result.rendered.num_frames = total_frames;
    result.rendered.duration_sec = run.duration_sec;
    result.rendered.sample_rate = load.sample_rate;
    result.rendered.max_block_size = load.max_block_size;
    result.artifacts.wav_path = wav_path;
    result.artifacts.artifact_path = load.plugin_path;
    result.qa_report = qa_report;

    return Result<HostRunResult>::Success(result);
}

// CLAP implementation
static Result<HostRunResult> LoadRunClap(const HostLoadOptions& load, const HostRunOptions& run, const Optional<String>& save_wav_path) {
    // For CLAP, we need to implement a minimal host
    // This requires the clap headers and entry functions
    // Since CLAP is more complex, we'll create a basic implementation

    // Load the library
    void* handle = DylibOpen(load.plugin_path);
    if (!handle) {
        return Result<HostRunResult>::Error(ErrorCode::ExternalLoadFailure, "Failed to load CLAP plugin library");
    }

    // Look for clap_entry symbol
    typedef bool (*clap_entry_t)(const void* entry);
    auto clap_entry = (clap_entry_t)DylibSym(handle, "clap_entry");
    if (!clap_entry) {
        DylibClose(handle);
        return Result<HostRunResult>::Error(ErrorCode::ExternalLoadFailure, "Failed to find clap_entry function in plugin");
    }

    // For now, return not implemented since we don't have the proper CLAP headers
    // In a real implementation, we would get the factory and plugin interfaces
    DylibClose(handle);
    return Result<HostRunResult>::Error(ErrorCode::NotImplemented, "CLAP implementation requires CLAP headers and is not yet complete");
}

// LV2 implementation (optional)
#if defined(PROTOVM_ENABLE_LV2_HOST)
#include <lilv/lilv.h>

static Result<HostRunResult> LoadRunLv2(const HostLoadOptions& load, const HostRunOptions& run, const Optional<String>& save_wav_path) {
    // Initialize Lilv
    LilvWorld* world = lilv_world_new();
    if (!world) {
        return Result<HostRunResult>::Error(ErrorCode::ExternalLoadFailure, "Failed to initialize LV2 world");
    }

    // Load the plugin bundle
    lilv_world_load_all(world);

    // Find the plugin by URI if provided
    LilvNode* plugin_uri = nullptr;
    if (!load.plugin_id.IsEmpty()) {
        plugin_uri = lilv_new_uri(world, load.plugin_id);
    }

    // Get plugin by URI or take first available if no ID provided
    const LilvPlugin* plugin = nullptr;
    if (plugin_uri) {
        plugin = lilv_world_get_plugin_by_uri(world, plugin_uri);
    } else {
        LILV_FOREACH(plugins, iter, lilv_world_get_all_plugins(world)) {
            plugin = lilv_plugins_get(lilv_world_get_all_plugins(world), iter);
            break; // Take first plugin
        }
    }

    if (!plugin) {
        lilv_node_free(plugin_uri);
        lilv_world_free(world);
        return Result<HostRunResult>::Error(ErrorCode::ExternalLoadFailure, "LV2 plugin not found");
    }

    // Get plugin features (we'll need to define host features too)
    // This is a simplified approach - a full implementation would be much more complex
    // For now, return not implemented as LV2 is quite complex
    lilv_node_free(plugin_uri);
    lilv_world_free(world);
    return Result<HostRunResult>::Error(ErrorCode::NotImplemented, "LV2 implementation requires full LV2 host features and is not yet complete");
}
#else
static Result<HostRunResult> LoadRunLv2(const HostLoadOptions& load, const HostRunOptions& run, const Optional<String>& save_wav_path) {
    return Result<HostRunResult>::Error(ErrorCode::MissingDependency, "LV2 host requires external library (lilv) which is not enabled in this build");
}
#endif

// VST3 implementation (optional stub)
#if defined(PROTOVM_ENABLE_VST3_HOST)
static Result<HostRunResult> LoadRunVst3(const HostLoadOptions& load, const HostRunOptions& run, const Optional<String>& save_wav_path) {
    // VST3 implementation would require the VST3 SDK
    return Result<HostRunResult>::Error(ErrorCode::NotImplemented, "VST3 implementation requires VST3 SDK and is not yet complete");
}
#else
static Result<HostRunResult> LoadRunVst3(const HostLoadOptions& load, const HostRunOptions& run, const Optional<String>& save_wav_path) {
    return Result<HostRunResult>::Error(ErrorCode::MissingDependency, "VST3 host requires external SDK which is not enabled in this build");
}
#endif

} // namespace Upp