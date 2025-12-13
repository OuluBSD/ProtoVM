# ProtoVM Plugin Skeleton Export Design

## Overview

This document describes the plugin skeleton export feature in ProtoVM. This feature enables the export of hybrid analog+digital instruments as plugin-ready source code templates for VST3, LV2, CLAP, and LADSPA formats.

The plugin skeleton export turns ProtoVM into a plugin-oriented instrument engine exporter, allowing hybrid instruments to be used as building blocks for real-world audio plugins. The system provides a host-agnostic C ABI audio engine interface and plugin skeleton export layer that can emit C or C++ source templates.

## C ABI Audio Engine Interface

### Goals

The C ABI audio engine defines a small C-compatible API that can:
- Initialize an instrument instance
- Process audio blocks in-place (interleaved or non-interleaved)
- Set parameters (e.g., frequency, gain, pan depth)
- Reset/flush internal state

The ABI is host-agnostic (does not mention VST/LV2/CLAP/LADSPA directly) and operates on:
- A handle/opaque pointer to an engine instance
- Arrays of float samples for input/output
- A parameter interface with simple indices or string IDs

### C ABI Data Structures

The C ABI defines several key data structures:

```cpp
// Basic opaque handle
typedef struct ProtoVM_AudioEngine ProtoVM_AudioEngine;

// Simple parameter identifiers (index-based)
typedef enum ProtoVM_ParamId {
    PROTOVM_PARAM_MAIN_FREQ = 0,  // Main frequency parameter
    PROTOVM_PARAM_MAIN_GAIN = 1,  // Main gain parameter
    PROTOVM_PARAM_PAN_DEPTH = 2,  // Pan depth parameter
    // More parameters can be added
    PROTOVM_PARAM_COUNT
} ProtoVM_ParamId;

// Configuration for the engine
typedef struct ProtoVM_AudioEngineConfig {
    int sample_rate;    // e.g. 48000
    int max_block_size; // e.g. 1024
    int num_channels;   // For now: 2 (stereo)
    int voice_count;    // e.g. 4
} ProtoVM_AudioEngineConfig;

// Parameter values
typedef struct ProtoVM_AudioEngineParams {
    float values[PROTOVM_PARAM_COUNT];
} ProtoVM_AudioEngineParams;
```

### C ABI Functions

The C ABI provides the following functions:

```cpp
// Create/destroy engine instances
ProtoVM_AudioEngine* ProtoVM_AudioEngine_Create(const ProtoVM_AudioEngineConfig* cfg);
void ProtoVM_AudioEngine_Destroy(ProtoVM_AudioEngine* engine);

// Reset / flush state
void ProtoVM_AudioEngine_Reset(ProtoVM_AudioEngine* engine);

// Set parameters (RT-safe: just store values, no allocation)
void ProtoVM_AudioEngine_SetParams(ProtoVM_AudioEngine* engine,
                                   const ProtoVM_AudioEngineParams* params);

// Audio processing (non-interleaved stereo)
void ProtoVM_AudioEngine_Process(
    ProtoVM_AudioEngine* engine,
    const float* inL,
    const float* inR,
    float* outL,
    float* outR,
    int num_frames
);
```

All functions in the audio callback path are exception-free and allocation-free (RT-safe semantics).

## Plugin Skeleton Export

### Targets

The plugin skeleton export supports four major plugin formats:

- **VST3**: Steinberg's modern plugin format
- **LV2**: Linux Audio Developer's Simple Plugin API
- **CLAP**: New standard plugin format
- **LADSPA**: Linux Audio Developer's Simple Plugin API (legacy)

### Export Options

The export functionality uses `PluginSkeletonOptions` to configure the export:

```cpp
enum class PluginTargetKind {
    Vst3,
    Lv2,
    Clap,
    Ladspa
};

struct PluginSkeletonOptions {
    PluginTargetKind target;      // Target plugin format
    String plugin_name;           // Plugin name (e.g. "ProtoVMHybridOsc")
    String plugin_id;             // Plugin ID (e.g. "protovm.hybrid.osc")
    String vendor;                // Vendor name (e.g. "ProtoVM")
    int num_inputs = 0;           // Number of input channels (0 for instruments)
    int num_outputs = 2;          // Number of output channels (2 for stereo)
    bool emit_comment_banner = true; // Include comment banner
};
```

### Implementation Architecture

The plugin skeleton export architecture consists of:

1. **AudioEngineCAbi**: The core C ABI audio engine interface
2. **InstrumentEngineFactory**: Maps InstrumentGraph to C ABI engine
3. **PluginSkeletonExport**: Generates plugin source templates
4. **CircuitFacade**: Integration point for plugin skeleton export
5. **CLI Command**: `instrument-export-plugin-skeleton` command
6. **Designer Endpoint**: `designer-instrument-export-plugin-skeleton` endpoint

## Plugin Mapping Guidelines

### VST3 Mapping

- In the plugin initialize/constructor, call `ProtoVM_AudioEngine_Create(&cfg)`
- In the plugin process(audioBuffer, numFrames), call `ProtoVM_AudioEngine_Process(engine, nullptr, nullptr, outL, outR, numFrames)`
- Map plugin parameters to `ProtoVM_AudioEngineParams`

### LV2 Mapping

- In instantiate, create the audio engine
- In activate, reset the engine state
- In run, process the audio frames
- In cleanup, destroy the engine

### CLAP Mapping

- In init, perform any necessary initialization
- In activate, create the audio engine
- In process, call the engine's process function
- In deactivate, destroy the engine

### LADSPA Mapping

- In instantiate, create the audio engine
- In connect_port, connect audio ports
- In run, process the audio frames
- In cleanup, destroy the engine

## CLI Usage

The plugin skeleton can be exported using the CLI command:

```
instrument-export-plugin-skeleton
  --workspace <path>
  --session-id <id>
  --branch <branch_name>
  --plugin-target {vst3|lv2|clap|ladspa}
  --plugin-name <name>
  --plugin-id <id>
  --vendor <vendor>
  --instrument-from-json <file>  # Optional: load instrument from JSON
  # Or build instrument inline:
  --instrument-id <id>
  --analog-block-id <id>
  --digital-block-id <id>
  --voice-count <count>
  --sample-rate <rate>
  --duration-sec <seconds>
  --base-freq-hz <frequency>
  --detune-spread-cents <cents>
  --pan-lfo-hz <rate>
  --use-analog-primary  # Optional flag
```

The command exports JSON output with the skeleton source code.

## Designer API Usage

The designer API provides an endpoint for plugin skeleton export:

```
"designer-instrument-export-plugin-skeleton" : {
  "designer_session_id": "cd-1234",
  "instrument_id": "HYB_OSC_1",
  "plugin_target": "vst3",
  "plugin_name": "ProtoVMHybridOsc",
  "plugin_id": "protovm.hybrid.osc",
  "vendor": "ProtoVM",
  # Additional instrument parameters...
}
```

## Implementation Details

### Audio Engine Internal Structure

The `ProtoVM_AudioEngine` internally holds:

- Sample rate and block size parameters
- Instrument graph and converted DSP graph
- Runtime state for DSP processing
- Analog solver state if needed
- Current parameter values

### RT Safety Considerations

The audio processing path is designed to be real-time safe:
- No dynamic allocations in audio callbacks
- No exceptions raised in audio thread
- No blocking operations
- Minimal computational overhead

### Parameter Mapping

The engine maps high-level parameters to internal instrument parameters:
- `PROTOVM_PARAM_MAIN_FREQ` maps to underlying source frequency
- `PROTOVM_PARAM_MAIN_GAIN` maps to instrument gain
- `PROTOVM_PARAM_PAN_DEPTH` maps to pan depth control

## Future Enhancements

1. **Enhanced Parameter Mapping**: More sophisticated parameter mapping with custom parameter descriptors
2. **MIDI Support**: Adding MIDI input handling to the plugin skeletons
3. **Advanced DSP**: Support for more complex DSP operations in the engine
4. **GUI Support**: Basic GUI templates for each plugin format
5. **Multi-channel Support**: Support for more than stereo output
6. **Polyphonic Support**: Better voice management for polyphonic instruments