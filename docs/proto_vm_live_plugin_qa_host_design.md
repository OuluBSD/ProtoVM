# ProtoVM Live Plugin QA Host Design

## Overview

The **ProtoVM Live Plugin QA Host** is a specialized system for loading, running, and validating exported plugin binaries in a host environment. This system enables ProtoVM to function as a self-testing plugin toolchain that can:

1. Export hybrid instruments as plugin projects
2. Build the plugin binaries
3. Load and run the plugins in a host environment
4. Execute Phase-29-quality audio analysis and generate reports
5. Provide structured validation feedback

## Core Architecture

### PluginHostHarness

The central component is the `PluginHostHarness` class which implements a minimal host for various plugin formats:

- **LADSPA** (required) - Loadable Audio Digital Signal Processing Architecture
- **CLAP** (required) - CLAP Audio Plugin API
- **LV2** (optional) - LV2 plugin standard via lilv dependency
- **VST3** (stub) - Steinberg VST3 format with external SDK requirement

### Key Structures

#### HostPluginKind
```cpp
enum class HostPluginKind {
    Clap,
    Ladspa,
    Lv2,
    Vst3
};
```

#### HostLoadOptions
Configuration for loading a plugin:
- `kind` - Plugin format type
- `plugin_path` - Path to the shared library/bundle
- `plugin_id` - Format-specific identifier (CLAP ID, LV2 URI, LADSPA index)
- `sample_rate` - Audio sample rate
- `max_block_size` - Maximum processing block size

#### HostRunOptions
Configuration for running the plugin:
- `duration_sec` - Duration of the test run
- `wallclock_paced` - Flag to pace processing with real-time delays
- `enable_param_automation` - Enable parameter automation during test
- `param_name/param_index` - Parameter to automate (format-specific)
- `param_start/param_end` - Range for parameter automation
- `block_sizes` - Stress test with different block sizes
- `enable_denormal_flush` - Enable denormal number flushing

#### HostRunResult
Results from the plugin run:
- `rendered` - Summary of audio rendering
- `artifacts` - File paths for outputs
- `qa_report` - Audio quality analysis results
- `host_warnings` - Host-level warnings

## CLI Commands

### `plugin-build`
Builds a plugin project from source:

```
protovm-cli plugin-build --project-dir PATH --build-dir PATH
```

- `--project-dir` - Path to plugin project directory
- `--build-dir` - Build output directory (optional, defaults to project_dir/build)

### `plugin-qa-live`
Runs QA analysis on a loaded plugin:

```
protovm-cli plugin-qa-live --plugin-kind KIND --plugin-path PATH [options]
```

**Required:**
- `--plugin-kind` - Plugin format (clap, ladspa, lv2, vst3)
- `--plugin-path` - Path to plugin binary

**Optional:**
- `--plugin-id` - Plugin identifier (required for CLAP)
- `--sample-rate` - Sample rate in Hz (default: 48000)
- `--block-size` - Block size (default: 512)
- `--duration-sec` - Test duration in seconds (default: 3.0)
- `--wallclock-paced` - Enable real-time pacing
- `--param-name` - Parameter name for automation (CLAP)
- `--param-index` - Parameter index for automation (LADSPA)
- `--param-start` - Automation start value (default: 0.0)
- `--param-end` - Automation end value (default: 1.0)
- `--block-sizes` - Comma-separated block sizes for stress test
- `--save-wav` - Path to save WAV output

### `plugin-export-build-qa`
End-to-end pipeline command combining export, build, and QA:

```
protovm-cli plugin-export-build-qa [export options] [build options] [qa options]
```

## Daemon Endpoints

### `designer-plugin-qa-live`
JSON-RPC endpoint equivalent to `plugin-qa-live` command.

### `designer-plugin-export-build-qa`
JSON-RPC endpoint equivalent to `plugin-export-build-qa` command.

## Audio QA Integration

The system integrates with ProtoVM's Phase-29 Audio QA system to provide comprehensive analysis:

- **Peak level detection**
- **RMS level measurement**
- **THD (Total Harmonic Distortion)**
- **Fundamental frequency analysis**
- **Stereo balance and width**
- **Phase correlation**
- **Silence detection**
- **Harmonic energy analysis**

## LADSPA Implementation

The LADSPA loader implements a minimal host with:
- Plugin descriptor lookup via `ladspa_descriptor`
- Port identification for audio/control ports
- Parameter automation through control port connections
- Sample-accurate processing

## CLAP Implementation

The CLAP loader provides:
- Entry point discovery via `clap_entry`
- Plugin factory interaction
- Minimal host implementation
- Audio buffer management
- Parameter automation via CLAP events

## Limitations and Considerations

### Real-time vs. Offline Processing
- The host simulates real-time processing but runs offline
- Wallclock pacing option allows timing simulation
- No real-time priority or thread management

### Parameter Automation
- CLAP: Uses parameter IDs/names for automation
- LADSPA: Uses control port indices for automation
- Automation follows linear interpolation over test duration

### Memory Management
- All host buffers allocated once outside processing loop
- Plugin resources properly managed with RAII
- No memory allocations in processing path

### Error Handling
- Comprehensive error reporting at each stage
- Graceful degradation when optional formats unavailable
- Clear error messages with specific error codes

## Dependencies

### Required
- libdl (dynamic loading) - POSIX systems

### Optional
- lilv - for LV2 host support
- VST3 SDK - for VST3 host support

## Testing

The system includes test fixture plugins for both LADSPA and CLAP formats to verify functionality.