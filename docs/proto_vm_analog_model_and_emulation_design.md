# ProtoVM Analog Model and Emulation Design

## Overview

The Analog Model and Emulation system provides an intermediate representation (IR) and simulation engine for analog circuits in ProtoVM. This system enables audio-rate simulation of analog oscillator circuits and other simple analog blocks, integrating them into the existing DSP graph and runtime system.

## Analog Model IR (AnalogBlockModel)

The Analog Model IR provides a compact representation of small-scale analog circuits suitable for audio applications. The system targets simple, self-contained analog blocks such as:

- RC oscillators (single R, single C with feedback)
- Simple RC filters (1-2 poles)
- Single-transistor amplifier stages

### Core Concepts

- **AnalogBlockModel**: Represents a simplified analog circuit as a set of state variables and parameters
- **AnalogBlockKind**: Indicates the type of analog circuit (RcOscillator, SimpleFilter, TransistorStage)
- **AnalogStateVar**: Represents a voltage or current state variable (e.g., capacitor voltage)
- **AnalogParam**: Represents a circuit parameter (R, C, gain, bias)

### Structure Details

- **State Variables**: Track internal voltages/currents that evolve over time
- **Parameters**: Fixed values like resistances, capacitances, gains that define the circuit behavior
- **Output Mapping**: Specifies which state variable represents the audio output
- **Frequency Estimate**: Optional estimate of the circuit's natural frequency (useful for oscillators)

## Analog Solver

The Analog Solver provides a time-stepping simulation engine for AnalogBlockModel instances. It uses simple numerical integration to advance the circuit state from one time step to the next.

### Solver Configuration

- **Sample Rate**: Audio sampling rate (typically 44.1kHz or 48kHz)
- **Integration Step**: Time step (typically `1/sample_rate`)
- **Integrator Type**: Currently uses Euler integration (for simplicity)

### Supported Behavior

- **RcOscillator**: Simple RC oscillator with feedback, modeled with basic differential equations
- **SimpleFilter**: Basic RC filter with 1-pole response
- **TransistorStage**: Simplified transistor amplifier with bias and gain parameters

## Integration with DSP Runtime

The analog model integrates with the existing DSP graph and runtime system by introducing a new node kind:

### New DSP Node Kind

- **AnalogBlockSource**: Generates audio samples using an AnalogBlockModel and AnalogSolver

### Runtime Integration

The DspRuntime has been extended to:

- Maintain AnalogSolverState instances for AnalogBlockSource nodes
- Initialize solvers during DspRuntime::Initialize
- Advance analog simulation during DspRuntime::Render
- Route analog output to the DSP graph for further processing

## CLI Commands

### analog-model-inspect

Inspect the analog model extracted from an analog circuit block.

**Request Parameters**:
- `--workspace`: Path to workspace
- `--session-id`: Session ID
- `--branch`: Branch name
- `--block-id`: Block ID to analyze

**Response**:
```json
{
  "ok": true,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "ANALOG_OSC_BLOCK",
    "analog_model": {
      "id": "ANALOG_OSC1",
      "block_id": "ANALOG_OSC_BLOCK",
      "kind": "RcOscillator",
      "state": [
        { "name": "v_out", "kind": "Voltage", "value": 0.0 }
      ],
      "params": [
        { "name": "R", "value": 10000.0 },
        { "name": "C", "value": 1e-7 }
      ],
      "output_state_name": "v_out",
      "estimated_freq_hz": 430.0
    }
  }
}
```

### analog-render-osc

Render analog oscillator output to stereo audio with panning.

**Request Parameters**:
- `--workspace`: Path to workspace
- `--session-id`: Session ID
- `--branch`: Branch name
- `--block-id`: Block ID to render
- `--sample-rate <double>`: Sample rate (e.g. 48000)
- `--duration-sec <double>`: Duration in seconds (>= 3.0)
- `--pan-lfo-hz <double>`: Pan LFO rate (default 0.25)

**Response**:
```json
{
  "ok": true,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "ANALOG_OSC_BLOCK",
    "sample_rate_hz": 48000.0,
    "duration_sec": 3.0,
    "estimated_freq_hz": 430.0,
    "pan_lfo_hz": 0.25,
    "total_samples": 144000,
    "left_preview": [0.1, 0.2, 0.3, ...],
    "right_preview": [0.1, 0.2, 0.3, ...],
    "render_stats": {
      "left_rms": 0.5,
      "right_rms": 0.5,
      "left_min": -0.8,
      "left_max": 0.8,
      "right_min": -0.8,
      "right_max": 0.8
    }
  }
}
```

## CoDesigner Endpoints

### designer-analog-model-inspect

CoDesigner endpoint for analog model inspection.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "target": "block",
  "block_id": "ANALOG_OSC_BLOCK"
}
```

**Response**:
```json
{
  "ok": true,
  "data": {
    "designer_session": { ... },
    "analog_model": { ... }
  }
}
```

### designer-analog-render-osc

CoDesigner endpoint for analog oscillator rendering.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "target": "block",
  "block_id": "ANALOG_OSC_BLOCK",
  "sample_rate_hz": 48000.0,
  "duration_sec": 3.0,
  "pan_lfo_hz": 0.25
}
```

**Response**:
```json
{
  "ok": true,
  "data": {
    "designer_session": { ... },
    "left_samples": [...],
    "right_samples": [...],
    "render_stats": { ... }
  }
}
```

## Implementation Details

### AnalogBlockExtractor

The AnalogBlockExtractor analyzes CircuitGraph instances to identify analog circuit patterns and create corresponding AnalogBlockModel representations. It currently recognizes:

- RC oscillator patterns (resistor + capacitor with feedback/inversion)
- Simple RC filter configurations
- Single-transistor amplifier stages

### Numerical Integration

The AnalogSolver uses basic Euler integration for time-stepping:

`state_new = state_old + dt * derivative`

This approach provides acceptable results for audio applications while maintaining computational efficiency.

### Stability Considerations

- Basic voltage clamping to prevent runaway
- Conservative parameter values for stable operation
- Feedback mechanisms to maintain oscillation

## Example Usage

An RC-based oscillator can be configured to approximate ~440 Hz, connected to the 0.25 Hz pan LFO and stereo panner for 3 seconds of stereo audio rendering. This creates a continuously oscillating tone that smoothly pans from left to right and back.

## Limitations

- Single-input or no-input analog blocks only
- Limited to small, self-contained circuits (1-3 nodes)
- Simplified models without complex non-linearities
- No AC analysis, only time-domain simulation
- No multi-block coupling (each analog block is self-contained)