# ProtoVM Performance Capture Design

## Overview

The Performance Capture system enables the recording of live performance data (MIDI, control mappings, scene morphs) in a deterministic, time-stamped format. This captured data can be quantized into semantic actions and converted back into IntentSessions for replay, editing, diffing, and export.

This system closes the creative loop between design → play → design by allowing the system to learn from how instruments were played.

## Key Principles

- **Recording must not break determinism**: All recorded data must be fully timestamped and reproducible at any sample rate.
- **Time-stamped control events**: All performance events are recorded with precise timing.
- **Semantic derivation**: Raw performance data is converted into higher-level automation and intent proposals.

## Data Model

### PerformanceEvent

```cpp
struct PerformanceEvent {
    double time_sec;                    // Absolute time in seconds from capture start
    ControlSource source;               // Source of the control event (MIDI CC, knob, etc.)
    double normalized_value;           // Normalized value (0.0 to 1.0)
};
```

### PerformanceRecording

```cpp
struct PerformanceRecording {
    String recording_id;               // Unique identifier for this recording
    String instrument_id;              // ID of the instrument being controlled
    int sample_rate;                   // Sample rate used during capture
    double duration_sec;               // Total duration in seconds
    
    Vector<PerformanceEvent> events;   // Sequence of captured events
    
    // Optional derived artifacts:
    Optional<AutomationClip> derived_automation;      // Converted automation clip
    Optional<IntentSessionState> derived_intent_session; // Converted intent session
};
```

## Core Engine

### PerformanceCaptureEngine

The core engine provides thread-safe methods for managing the capture lifecycle:

- `BeginCapture()`: Initiates recording with a sample rate
- `CaptureControlEvent()`: Records an individual control event
- `EndCapture()`: Finalizes recording and returns the complete PerformanceRecording

### Thread Safety

- Lock-free or minimally locked design
- Events recorded in strictly increasing time order
- No heap allocation in audio callbacks

## Derivation Engine

The system provides two levels of semantic derivation:

### 1. Performance → Automation

Converts dense control movements into clean automation lanes:

- Collapse dense control movements into clean automation lanes
- Quantize time to fixed resolution (e.g., audio block boundaries)
- Deterministic smoothing with fixed kernel

```cpp
Result<AutomationClip> DeriveAutomationFromPerformance(
    const PerformanceRecording& recording,
    double smoothing_sec = 0.01,
    double min_delta = 0.001
);
```

### 2. Performance → IntentSession (Heuristic)

Maps performance to higher-level intent proposals:

- Detect stable regions → intent proposals
- Detect morph sweeps → IntentProfileMix
- Detect repeated CC patterns → modulator suggestions

```cpp
Result<IntentSessionState> DeriveIntentSessionFromPerformance(
    const PerformanceRecording& recording,
    const InstrumentGraph& base_instrument
);
```

## Integration Points

### CircuitFacade Integration

- `BeginPerformanceCaptureInBranch()`
- `EndPerformanceCaptureInBranch()`
- `DeriveAutomationFromLastPerformanceInBranch()`
- `DeriveIntentSessionFromLastPerformanceInBranch()`

### CLI Commands

- `performance-capture-start` - Begin recording performance
- `performance-capture-stop` - Stop recording and return summary
- `performance-capture-show` - Show current capture status
- `performance-derive-automation` - Derive automation from last performance
- `performance-derive-intent` - Derive intent session from last performance

### CoDesigner/Daemon Endpoints

- `designer-performance-capture-start`
- `designer-performance-capture-stop`
- `designer-performance-derive`

These endpoints allow AI agents to observe human play, turn it into reusable structure, and suggest refinements.

## Determinism Guarantees

The system ensures deterministic capture and replay by:

1. **Time-stamped Events**: Every event is timestamped with microsecond precision
2. **Fixed Sample Rate**: All recordings use a fixed sample rate for consistency
3. **Reproducible Derivation**: Derivation algorithms are deterministic
4. **Ordered Processing**: Events are processed in strictly increasing time order

## Limitations

1. **Intent Derivation**: Heuristic mapping is not perfect - it's conservative and auditable
2. **Real-time Constraints**: Capture must be efficient to avoid audio dropouts
3. **Memory Usage**: Long recordings may require memory management strategies

## Recommended Capture Practices

1. **Consistent Timing**: Maintain consistent sample rates across sessions
2. **Validated Sources**: Ensure control sources are properly normalized
3. **Appropriate Smoothing**: Use appropriate smoothing values for target applications
4. **Regular Cleanup**: Periodically clear old recording data to manage memory

## Future Enhancements

1. **Advanced Derivation**: More sophisticated intent detection algorithms
2. **Performance Analysis**: Tools to analyze captured performances
3. **Pattern Recognition**: Enhanced pattern detection for automation generation
4. **MIDI File Export**: Export captured performance as MIDI files
5. **Multi-track Support**: Capture multiple instruments simultaneously

## Usage Examples

### Basic Recording Workflow
```bash
# Start recording at 44100 Hz
./proto_vm performance-capture-start --session-id 123 --branch main --sample-rate 44100

# Play your instrument...

# Stop recording
./proto_vm performance-capture-stop --session-id 123 --branch main --duration 5.0

# Derive automation from the performance
./proto_vm performance-derive-automation --session-id 123 --branch main --smoothing 0.02
```

### CoDesigner Integration
```cpp
CoDesignerManager manager(circuit_facade);
auto perf_result = manager.DesignerPerformanceCaptureStart("designer_sess_456", 44100);

// During performance...
// Call PerformanceCaptureEngine::CaptureControlEvent() for each event...

auto recording = manager.DesignerPerformanceCaptureStop("designer_sess_456", 5.0);
auto automation = manager.DesignerPerformanceDerive("designer_sess_456", 0.01, 0.001);
```

## Conclusion

The Performance Capture system enables ProtoVM to "listen to musicians" by capturing their expressive performances and converting them into reusable, deterministic structures. This creates a closed creative loop where human intuition feeds back into systematic design improvements.