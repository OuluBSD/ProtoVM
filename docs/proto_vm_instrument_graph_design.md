# ProtoVM: Hybrid DSP+Analog Instrument Graph System

## Overview

The Hybrid DSP+Analog Instrument Graph system provides a unified framework for composing and rendering instruments that combine both digital DSP components and analog circuit emulations. This system enables users to create complex, multi-voice instruments that leverage both computational DSP algorithms and analog circuit behavior.

## Architecture

### Core Concepts

The hybrid instrument system introduces several key abstractions:

1. **InstrumentGraph** - High-level description of a musical instrument
2. **InstrumentVoiceTemplate** - Template defining the structure of individual voices
3. **NoteDesc** - Musical note parameters (frequency, duration, etc.)
4. **VoiceConfig** - Per-voice configuration (detuning, source type, etc.)

### Data Structures

#### InstrumentGraph
The central data structure that describes a complete instrument:

- `instrument_id` - Unique identifier for the instrument
- `sample_rate_hz` - Audio sample rate (e.g., 48000 Hz)
- `voice_count` - Number of simultaneous voices (for polyphony)
- `voice_template` - Template used to create individual voices
- `voices` - Array of per-voice configurations
- `note` - Musical note to be played
- `use_analog_primary` - Whether analog or digital sources are primary

#### Voice Configuration
Each voice in an instrument can be configured independently:

- `id` - Unique voice identifier (e.g., "voice0", "voice1")
- `detune_cents` - Frequency detuning in cents (±50 cents typical)
- `use_analog_source` - Whether this voice uses analog or digital source

#### Instrument Voice Template
Defines the signal path topology for each voice:

- `analog_block_id` - Reference to analog circuit block (optional)
- `digital_block_id` - Reference to digital oscillator block (optional)
- `has_pan_lfo` - Whether voice includes pan LFO
- `pan_lfo_hz` - Pan LFO frequency (e.g., 0.25 Hz)
- `has_filter` - Whether voice includes filter (reserved for future)

## Components

### InstrumentBuilder
Responsible for constructing `InstrumentGraph` instances from high-level parameters:

- Distributes detuning across multiple voices
- Sets up basic voice configurations
- Validates parameter ranges

### InstrumentToDsp
Converts high-level `InstrumentGraph` to low-level `DspGraph` representation:

- Maps instrument voices to DSP nodes
- Creates mixer nodes for combining voices
- Establishes signal routing between components
- Handles both analog and digital source integration

### InstrumentRuntime
Executes the rendering process for an instrument:

- Uses `InstrumentToDsp` to create DSP graph
- Leverages existing `DspRuntime` for execution
- Produces stereo output buffers

### CircuitFacade Integration
The `CircuitFacade` provides high-level interfaces:

- `BuildHybridInstrumentInBranch()` - Create instrument graph for a branch
- `RenderHybridInstrumentInBranch()` - Render instrument audio for a branch

## CLI Commands

### instrument-build-hybrid
Builds a hybrid instrument configuration:

```
proto-vm-cli instrument-build-hybrid \
  --workspace /path/to/workspace \
  --session-id 123 \
  --branch main \
  --instrument-id "ANALOG_PLUS_DIGITAL" \
  --analog-block-id "RC_OSC_BLOCK" \
  --digital-block-id "DIGITAL_OSC_BLOCK" \
  --voice-count 4 \
  --sample-rate 48000 \
  --duration-sec 3.0 \
  --base-freq-hz 440.0 \
  --detune-spread-cents 10.0 \
  --pan-lfo-hz 0.25
```

### instrument-render-hybrid
Builds and renders a hybrid instrument:

```
proto-vm-cli instrument-render-hybrid \
  --workspace /path/to/workspace \
  --session-id 123 \
  --branch main \
  --instrument-id "ANALOG_PLUS_DIGITAL" \
  --analog-block-id "RC_OSC_BLOCK" \
  --digital-block-id "DIGITAL_OSC_BLOCK" \
  --voice-count 4 \
  --sample-rate 48000 \
  --duration-sec 3.0 \
  --base-freq-hz 440.0 \
  --detune-spread-cents 10.0 \
  --pan-lfo-hz 0.25
```

## CoDesigner Endpoints

### designer-instrument-build-hybrid
Endpoint for building hybrid instruments via CoDesigner API:

```json
{
  "command": "designer-instrument-build-hybrid",
  "designer_session_id": "cd-12345",
  "instrument_id": "HYBRID_OSC_1",
  "analog_block_id": "RC_OSC_BLOCK",
  "digital_block_id": "DIGITAL_OSC_BLOCK",
  "voice_count": 4,
  "sample_rate": 48000.0,
  "duration_sec": 3.0,
  "base_freq_hz": 440.0,
  "detune_spread_cents": 10.0,
  "pan_lfo_hz": 0.25,
  "use_analog_primary": true
}
```

### designer-instrument-render-hybrid
Endpoint for building and rendering hybrid instruments:

```json
{
  "command": "designer-instrument-render-hybrid",
  "designer_session_id": "cd-12345",
  "instrument_id": "HYBRID_OSC_1",
  "analog_block_id": "RC_OSC_BLOCK",
  "digital_block_id": "DIGITAL_OSC_BLOCK",
  "voice_count": 4,
  "sample_rate": 48000.0,
  "duration_sec": 3.0,
  "base_freq_hz": 440.0,
  "detune_spread_cents": 10.0,
  "pan_lfo_hz": 0.25,
  "use_analog_primary": true
}
```

## Signal Flow

A typical voice signal flow includes:

1. **Source** - Either analog circuit emulation or digital oscillator
2. **Detuning** - Per-voice frequency detuning for rich unison effects
3. **Pan LFO** - Low-frequency oscillator for stereo positioning
4. **Panner** - Maps mono source to stereo output based on pan control
5. **Mixer** - Combines multiple voices into final stereo output

## Voice Allocation & Polyphony

The system supports simple static voice allocation:

- Fixed number of voices (N voices playing same note for unison)
- Linear detuning distribution across voices
- Each voice can be configured to use analog or digital source

## Example: 4-Voice Unison Instrument

Creating a 4-voice unison instrument with analog RC oscillator as source:

```cpp
// Define voice template
InstrumentVoiceTemplate voice_template;
voice_template.analog_block_id = "RC_OSC_BLOCK";
voice_template.has_pan_lfo = true;
voice_template.pan_lfo_hz = 0.25;

// Define musical note
NoteDesc note;
note.base_freq_hz = 440.0;  // A4
note.duration_sec = 3.0;

// Build instrument with 4 voices and 10 cent spread
auto result = InstrumentBuilder::BuildHybridInstrument(
    "ANALOG_UNISON_A4",
    voice_template,
    48000.0,    // 48kHz sample rate
    4,          // 4 voices
    note,
    10.0        // 10 cent detune spread
);

// Convert to DSP graph and render
auto dsp_graph = InstrumentToDsp::BuildDspGraphForInstrument(
    result.data,
    circuit_facade,
    session,
    session_dir,
    "main"
);

std::vector<float> left_buffer, right_buffer;
auto render_result = InstrumentRuntime::RenderInstrument(
    result.data,
    circuit_facade,
    session,
    session_dir,
    "main",
    left_buffer,
    right_buffer
);
```

## Integration with Existing Systems

The hybrid instrument system leverages existing infrastructure:

- **DspGraph/DspRuntime** - For audio rendering execution
- **AnalogSolver** - For analog circuit emulation
- **CircuitFacade** - For circuit analysis and extraction
- **JsonIO** - For serialization and API responses
- **SessionServer** - For daemon command processing

This layered approach maintains compatibility while enabling complex hybrid signal chains.