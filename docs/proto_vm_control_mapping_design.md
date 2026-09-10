# ProtoVM Control Mapping & Performance Layer Design

## Overview

The ProtoVM Control Mapping & Performance Layer enables real-time control of ProtoVM instruments using MIDI controllers and other control sources. This system bridges the gap between automation artifacts (offline authored content) and real-time performance domains, making ProtoVM instruments playable as actual musical instruments rather than just static sound generators.

## Core Concepts

### Control Source Kinds
- **MidiCC**: MIDI Control Change messages (0-127)
- **MidiNote**: MIDI Note On/Off events
- **MidiPitchBend**: MIDI Pitch Bend messages (14-bit value)
- **HostParam**: DAW plugin parameters
- **VirtualKnob**: GUI-based virtual controls
- **VirtualXY**: 2D XY pad controls

### Control Target Kinds
- **InstrumentParam**: Direct instrument parameter control
- **AutomationLane**: Modulation of automation lanes
- **IntentStrength**: Control of musical intent intensity
- **IntentProfileMix**: Crossfading between different intent profiles

### Control Mapping Structure

A ControlMapping defines the relationship between a control source and its target:

```cpp
struct ControlMapping {
    String mapping_id;
    ControlSource source;           // What sends the control signal
    ControlTarget target;           // What receives the control signal
    
    double min_value = 0.0;        // Minimum output value
    double max_value = 1.0;        // Maximum output value
    double curve = 1.0;            // Curve transformation (1.0 = linear)
    
    bool bipolar = false;          // Whether the control is bipolar
    String note;                   // Optional description
};
```

### Control Map Structure

A ControlMap is a collection of individual ControlMappings:

```cpp
struct ControlMap {
    String map_id;                 // Unique identifier for the map
    String name;                   // Human-readable name
    Vector<ControlMapping> mappings; // Collection of individual mappings
};
```

## MIDI Abstraction

ProtoVM includes a lightweight MIDI abstraction layer without external dependencies:

```cpp
enum class MidiEventKind {
    NoteOn,
    NoteOff,
    CC,           // Control Change
    PitchBend
};

struct MidiEvent {
    MidiEventKind kind;
    int channel;    // MIDI channel (0-15)
    int number;     // CC number, note number, etc.
    int value;      // 0-127 for CC/notes, 0-16383 for pitch bend
};
```

## Control Engine

The ControlMappingEngine provides the core functionality for creating, applying, and managing control mappings:

```cpp
class ControlMappingEngine {
public:
    // Create default control mappings for an instrument
    static Result<ControlMap> CreateDefaultMapForInstrument(
        const InstrumentGraph& instrument
    );

    // Apply a normalized control value to the runtime
    static Result<void> ApplyControlValue(
        InstrumentRuntime& runtime,
        const ControlMapping& mapping,
        double normalized_value
    );

    // Apply a MIDI event using the control map
    static Result<void> ApplyMidiEvent(
        InstrumentRuntime& runtime,
        const ControlMap& map,
        const MidiEvent& event
    );
};
```

## Real-time Parameter Updates

The system maintains low-latency parameter updates with:
- Deterministic mapping math
- Host-safe ranges enforcement
- No allocations in audio callback paths
- Smooth parameter interpolation when needed

## Automation & Intent Integration

The system provides live modulation capabilities:
- Bind controls to AutomationLanes for real-time modulation
- Scale Intent strength live for dynamic musical expression
- Crossfade between IntentProfiles via control for performance flexibility

Example scenarios:
- Knob 1 → "Warmth" intent strength
- Knob 2 → Pan LFO depth
- XY pad → (filter cutoff, resonance)

## Integration with Existing Systems

### Command Line Interface
The system provides several CLI commands:
- `control-map-create`: Auto-generate default mappings
- `control-map-list`: List available control maps
- `control-map-show`: Show details of a specific map
- `control-map-apply`: Simulate control events
- `midi-play`: Process MIDI files or event lists

### CoDesigner Endpoints
AI agents can interact with the system through endpoints:
- `designer-control-map-create`: Create control layouts
- `designer-control-map-apply`: Simulate live performance
- `designer-midi-play`: Evaluate playability

These allow AI agents to design control layouts, simulate live performance, and evaluate playability.

## Performance Considerations

### Audio Thread Safety
- Parameter updates are designed to work safely in audio callbacks
- Simple 1-pole smoothing is available for parameter changes when needed
- All critical operations avoid dynamic allocations

### Deterministic Behavior
- Mapping calculations are deterministic for consistent behavior
- Host parameters are clamped to safe ranges automatically
- Curve transformations support linear, exponential, and logarithmic responses

## MIDI CC Conventions

The system respects standard MIDI CC conventions while allowing custom mappings:
- CC 1: Modulation Wheel
- CC 7: Volume
- CC 10: Pan
- CC 64: Sustain Pedal
- CC 71: Resonance
- CC 74: Cutoff Frequency
- CC 75: Release Time
- CC 76: Attack Time

## JSON Serialization

All control mapping structures support JSON serialization for storage and transmission:
- ControlSource
- ControlTarget
- ControlMapping
- ControlMap
- MidiEvent

This enables:
- Storing control maps in session files
- Sharing control maps between users
- Versioning and revision control of control layouts

## Security and Safety

### Range Validation
- All parameter values are validated against instrument-defined ranges
- Intent strength values are clamped to 0.0-1.0 range
- Host parameters are validated against plugin specifications

### Error Handling
- Comprehensive error handling throughout the system
- Graceful degradation when control sources are unavailable
- Clear error messages for debugging mapping issues

## Future Enhancements

### Short-term Improvements
1. **Propagation Delay Modeling**: Add explicit timing models for more realistic simulation
2. **Setup/Hold Time Checking**: Implement timing constraint validation
3. **Clock Domain Support**: Enable simulation of multi-clock circuits

### Long-term Vision
1. **Advanced Analysis Capabilities**: Formal verification integration, power consumption modeling
2. **Industry Standard Compatibility**: Verilog/VHDL import, standard cell library support
3. **Visualization and Debugging**: Interactive waveform viewer, real-time visualization
4. **Schematic Drawing Tools**: GUI tools for creating schematics

## Conclusion

The ProtoVM Control Mapping & Performance Layer transforms ProtoVM from a static instrument designer into a playable musical instrument. By providing real-time control capabilities, the system enables musicians to interact with algorithmically-designed instruments in an expressive, performance-oriented context.

The design emphasizes safety, determinism, and tight integration with both the automation system and the runtime engine, providing a solid foundation for musical expression while maintaining the technical integrity of the underlying digital design system.