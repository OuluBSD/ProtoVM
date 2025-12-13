# Plugin Project Export Design

## Overview

The Plugin Project Export feature in ProtoVM enables developers to export complete, disk-ready plugin project scaffolds from hybrid instruments. This represents an enhancement over the plugin skeleton export (Phase 27), which only generated source code snippets. The project export produces ready-to-build projects for major plugin formats:

- VST3
- LV2
- CLAP
- LADSPA

## Key Differences: Skeleton vs. Project Export

### Plugin Skeleton Export (Phase 27)
- Generated only source code snippets
- No build system
- No project structure
- Developer needed to create complete project manually

### Plugin Project Export (Phase 28)
- Generates complete project directory structure
- Includes build configuration files (CMakeLists.txt)
- Provides format-specific metadata files (LV2 TTL, CLAP manifest, etc.)
- Supplies README with build instructions
- Creates disk-ready project that can be built with standard tools (e.g., `cmake && make`)

## PluginProjectExportOptions

Defines the parameters for plugin project export:

```cpp
struct PluginProjectExportOptions {
    PluginTargetKind target;       // Vst3, Lv2, Clap, Ladspa

    String plugin_name;            // Human-readable name
    String plugin_id;              // Unique ID / URI (format-dependent)
    String vendor;                 // Vendor / author name
    String version;                // e.g. "1.0.0"

    String output_dir;             // Path where project scaffold is written

    // Audio / instrument settings:
    int num_inputs = 0;            // For now: 0 (instrument)
    int num_outputs = 2;           // Stereo
    int default_sample_rate = 48000;
    int default_block_size = 512;
    int default_voice_count = 4;

    bool emit_readme = true;
    bool emit_build_files = true;
};
```

## PluginProjectExport Class

The main export class with the following interface:

```cpp
class PluginProjectExport {
public:
    // Generate a full plugin project scaffold on disk.
    static Result<void> ExportPluginProject(
        const InstrumentGraph& instrument,
        const PluginProjectExportOptions& opts
    );
};
```

### Target-Specific Export Methods

The class delegates to format-specific exporters:
- `ExportVst3Project()`: Creates VST3 project
- `ExportLv2Project()`: Creates LV2 project
- `ExportClapProject()`: Creates CLAP project
- `ExportLadspaProject()`: Creates LADSPA project

## Per-Format Project Structure

### VST3 Projects
- `src/PluginWrapper.cpp` - Main wrapper with ProtoVM C ABI integration
- `CMakeLists.txt` - Build configuration with module suffix `.vst3`
- `README.md` - Build instructions for VST3

### LV2 Projects
- `src/Lv2Plugin.cpp` - LV2 descriptor and callback implementations
- `metadata/manifest.ttl` - LV2 manifest file
- `metadata/<plugin_id>.ttl` - Plugin-specific TTL file
- `CMakeLists.txt` - Build configuration with module suffix `_lv2.so`
- `README.md` - Build and installation instructions

### CLAP Projects
- `src/ClapPlugin.cpp` - CLAP plugin interface implementation
- `metadata/clap_manifest.json` - CLAP manifest file
- `CMakeLists.txt` - Build configuration with module suffix `.clap`
- `README.md` - Build instructions for CLAP

### LADSPA Projects
- `src/LadspaPlugin.c` - LADSPA descriptor and callback implementations
- `CMakeLists.txt` - Build configuration with module suffix `.so`
- `README.md` - Build and installation instructions

## Integration with ProtoVM_AudioEngine C ABI

All generated wrappers connect to the ProtoVM C ABI engine:

```cpp
// Example wrapper structure
class PluginWrapper {
private:
    ProtoVM_AudioEngine* engine;
    
public:
    // Initialize engine with configuration
    bool initialize(const ProtoVM_AudioEngineConfig& cfg) {
        engine = ProtoVM_AudioEngine_Create(&cfg);
        return engine != nullptr;
    }
    
    // Process audio through engine
    void process(float** outputs, int num_frames) {
        ProtoVM_AudioEngine_Process(engine, nullptr, nullptr, 
                                    outputs[0], outputs[1], num_frames);
    }
    
    // Clean up engine
    ~PluginWrapper() {
        if (engine) ProtoVM_AudioEngine_Destroy(engine);
    }
};
```

## CLI Command: instrument-export-plugin-project

Usage:
```
protovm instrument-export-plugin-project \
    --workspace /path/to/workspace \
    --session-id 1 \
    --branch main \
    --plugin-target vst3 \
    --plugin-name "MyPlugin" \
    --plugin-id "com.vendor.myplugin" \
    --vendor "Vendor Name" \
    --version "1.0.0" \
    --output-dir /path/to/output \
    --instrument-id "HYBRID_OSC_1"
```

### Parameters
- `--plugin-target {vst3|lv2|clap|ladspa}`: Target plugin format
- `--plugin-name`: Human-readable plugin name
- `--plugin-id`: Unique plugin identifier (format-dependent)
- `--vendor`: Plugin vendor/author name
- `--version`: Plugin version string
- `--output-dir`: Output directory for project files
- `--workspace`, `--session-id`, `--branch`: Session parameters
- `--instrument-id`: ID of instrument to export

## CoDesigner Endpoint: designer-instrument-export-plugin-project

JSON request:
```json
{
  "command": "designer-instrument-export-plugin-project",
  "id": "req-123",
  "payload": {
    "designer_session_id": "cd-1234",
    "instrument_id": "HYBRID_OSC_1",
    "plugin_target": "lv2",
    "plugin_name": "ProtoVMLv2HybridOsc",
    "plugin_id": "http://example.org/protovm/hybrid/osc",
    "vendor": "ProtoVM",
    "version": "1.0.0",
    "output_dir": "/tmp/protovm_lv2_osc",
    "sample_rate_hz": 48000,
    "voice_count": 4
  }
}
```

Response:
```json
{
  "id": "req-123",
  "ok": true,
  "command": "designer-instrument-export-plugin-project",
  "data": {
    "designer_session": "...",
    "instrument_id": "HYBRID_OSC_1",
    "plugin_target": "lv2",
    "plugin_name": "ProtoVMLv2HybridOsc",
    "plugin_id": "http://example.org/protovm/hybrid/osc",
    "output_dir": "/tmp/protovm_lv2_osc",
    "status": "ok"
  }
}
```

## Building Exported Projects

After generation, projects can typically be built with:
```
cd /path/to/plugin/project
mkdir build && cd build
cmake ..
cmake --build .
```

The resulting binary should be installed to appropriate plugin directories:
- VST3: `~/VST3/` or `Program Files/Common Files/VST3/`
- LV2: `~/.lv2/` or `/usr/lib/lv2/`
- CLAP: `~/CLAP/` or `Program Files/CLAP/`
- LADSPA: `~/.ladspa/` or `/usr/lib/ladspa/`

## Next Steps for Developers

The generated projects serve as starting points that developers may need to enhance:
- Add proper parameter mapping for instrument controls
- Implement GUI support if required
- Add proper licensing information
- Test with various host applications
- Integrate with actual SDK headers/libraries for each format