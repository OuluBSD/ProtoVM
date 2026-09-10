# ProtoVM Automation Export Design

## Overview

The ProtoVM Automation Export module enables the conversion of musical intent plans, sessions, and profiles into automation artifacts that can be used outside ProtoVM. This includes:

- Export parameter automation curves (time → value) that recreate intent effects
- Support for multiple targets: ProtoVM instrument runtime, exported plugin projects (CLAP, LADSPA, LV2, VST3), and generic automation.json
- Verification through AudioQA analysis to ensure exported automation reproduces expected audio within tolerance

## Architecture

### Core Data Structures

#### AutomationTargetKind
Enum defining target platforms for automation export:

- `ProtoVmInstrument`: Apply to InstrumentGraph/runtime
- `Clap`: Export CLAP param automation metadata
- `Ladspa`: Export LADSPA control automation
- `Lv2`: Export LV2 control automation
- `Vst3`: Export VST3 automation
- `GenericJson`: Always supported format

#### AutomationPoint
A single time-value pair in an automation curve:
```cpp
struct AutomationPoint {
    double time_sec = 0.0;
    double value = 0.0;
};
```

#### AutomationLane
A collection of automation points for a specific parameter:
```cpp
struct AutomationLane {
    String param_name;            // canonical name
    int param_index = -1;         // optional
    Vector<AutomationPoint> points;
    String note;                  // intent rationale
};
```

#### AutomationClip
The main container for automation data:
```cpp
struct AutomationClip {
    String clip_id;
    double duration_sec = 0.0;
    int sample_rate = 48000;

    Vector<AutomationLane> lanes;

    // Optional static snapshot (initial params):
    ValueMap initial_params;

    // Provenance:
    String source_intent_session_id;
    Vector<int> source_step_indices;
    String source_profile_id;
};
```

#### AutomationExportResult
Results from the export operation:
```cpp
struct AutomationExportResult {
    AutomationTargetKind target;
    String output_dir;
    Vector<String> written_files;
    AudioQaReport before_report;
    AudioQaReport after_report;
    AudioQaDiff diff;
    bool verified = false;
    Vector<String> warnings;
};
```

## Core Conversion Logic

### Building Clips from Intent Plans

The `BuildClipFromIntentPlan` method converts intent plan actions to automation lanes:

- `SetInstrumentParam`: Creates a ramp from current → target across a short window (e.g. 20ms) or step at t=0
- `AddModulator`: Exports modulator parameters as lanes (rate/depth) and annotates as "modulation"
- `AdjustMix`: Creates gain/pan/width adjustments with linear transitions
- Fixed interpolation rules: linear between points, no randomization

### Building Clips from Intent Sessions

The `BuildClipFromIntentSession` method processes sessions with multiple steps:

- Processes each accepted step in the session
- Maps each action to appropriate automation lanes
- Handles up_to_step parameter for partial session export
- Maintains provenance information

### Building Clips from Intent Profiles

The `BuildClipFromIntentProfile` method processes profile steps sequentially:

- Maps intent kinds to appropriate parameter names
- Maps each step to points in time based on step position
- Maintains profile provenance information

## Export Formats

### Generic Automation (automation.json)

Always exported, contains the complete automation data:

```json
{
  "clip_id": "clip-id-string",
  "duration_sec": 5.0,
  "sample_rate": 48000,
  "lanes": [
    {
      "param_name": "gain",
      "param_index": -1,
      "points": [
        {"time_sec": 0.0, "value": 0.0},
        {"time_sec": 0.02, "value": 0.7}
      ],
      "note": "Gain adjustment for louder output"
    }
  ],
  "initial_params": {},
  "source_intent_session_id": "session-001",
  "source_step_indices": [0, 1],
  "source_profile_id": "profile-id"
}
```

### Platform-Specific Exports

#### CLAP (clap_automation.json)
Maps lanes to CLAP parameter identifiers with best-effort mapping.

#### LADSPA (ladspa_automation.json)
Maps lanes to control port indices with best-effort mapping.

#### LV2 (lv2_automation.json)
Maps lanes to port symbols with best-effort mapping.

#### VST3 (vst3_automation_stub.json)
Provides stub with notes on importing via SDK tooling.

## Verification

### Instrument Runtime Verification
- Renders "before" using the instrument runtime without automation (or baseline settings)
- Renders "after" by applying automation clip during rendering
- Analyzes both with AudioQA analysis
- Produces AudioQaDiff and marks verified=true if within tolerance

### Plugin Verification (Optional)
- If plugin_path is provided, uses PluginHostHarness to run the plugin with automated params
- Compares QA results with instrument runtime verification
- Allows slight differences between implementations

## CLI Commands

### Core Export Commands

#### `automation-export-plan`
Export automation from a saved intent plan JSON:
```
proto-vm-cli automation-export-plan \
  --plan_file path/to/plan.json \
  --target generic \
  --out_dir ./output \
  --duration-sec 5.0 \
  --sample-rate 48000 \
  --verify
```

#### `automation-export-session`
Export automation from an intent session:
```
proto-vm-cli automation-export-session \
  --session_dir path/to/session \
  --intent_session_id session-001 \
  --target clap \
  --out_dir ./output \
  --duration-sec 5.0 \
  --sample-rate 48000 \
  --up_to_step 2 \
  --verify
```

#### `automation-export-profile`
Export automation from an intent profile:
```
proto-vm-cli automation-export-profile \
  --session_dir path/to/session \
  --profile_id profile-001 \
  --target ladspa \
  --out_dir ./output \
  --duration-sec 5.0 \
  --sample-rate 48000 \
  --verify
```

### Verification Commands

#### `automation-verify`
Verify automation reproduction using QA:
```
proto-vm-cli automation-verify \
  --clip_file path/to/automation.json \
  --verify
```

#### `plugin-automation-qa-live`
Load plugin and apply automation lanes during run:
```
proto-vm-cli plugin-automation-qa-live \
  --plugin_path /path/to/plugin.so \
  --plugin_id plugin_id \
  --plugin_kind clap \
  --automation_clip_file path/to/automation.json
```

## Lane Naming Conventions

### Common Parameter Names
- `gain`: General gain control
- `pan`: Panning control
- `pan_lfo_rate`: Pan LFO rate
- `pan_lfo_depth`: Pan LFO depth
- `width`: Stereo width
- `cutoff`: Filter cutoff frequency
- `resonance`: Filter resonance
- `attack`: ADSR attack time
- `decay`: ADSR decay time
- `sustain`: ADSR sustain level
- `release`: ADSR release time

### Modulator Parameters
- `lfo_rate`: LFO rate parameter
- `lfo_depth`: LFO depth parameter
- `lfo_waveform`: LFO waveform selection
- `env_attack`: Envelope attack time
- `env_release`: Envelope release time

## Integration Points

### CircuitFacade Integration
Added methods for branch-aware automation export:
- `ExportAutomationFromIntentSessionInBranch`
- `ExportAutomationFromIntentProfileInBranch`

### CoDesigner Endpoints
- `designer-automation-export`: Operates on current designer context
- `designer-automation-verify`: Verification in designer context

## AudioQA Verification

The system performs deterministic verification by:
- Rendering the instrument before and after applying automation
- Comparing AudioQA metrics within tolerance
- Marking verification status in export results
- Supporting both instrument runtime and plugin verification paths

## Backward Compatibility

- All existing functionality preserved
- New export formats are optional additions
- CLI commands are additive
- No breaking changes to existing interfaces

## Future Enhancements

### Planned Features
- Propagation delay modeling for more realistic timing
- Setup/hold time checking
- Clock domain support for multi-clock circuits
- Advanced analysis capabilities (formal verification, power modeling)
- Industry standard compatibility (Verilog/VHDL import)
- Visualization tools and waveform viewers

### Implementation Notes
- Deterministic point generation ensures reproducible results
- Linear interpolation between points for predictable behavior
- Best-effort parameter mapping for plugin targets
- Comprehensive error handling and reporting