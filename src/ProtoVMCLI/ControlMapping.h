#ifndef PROTOVM_CONTROL_MAPPING_H
#define PROTOVM_CONTROL_MAPPING_H

#include "../ProtoVM/DspGraph.h"
#include "InstrumentGraph.h"
#include "InstrumentRuntime.h"
#include "Result.h"
#include "String.h"
#include <Vector.h>

// MIDI Event Definitions
enum class MidiEventKind {
    NoteOn,
    NoteOff,
    CC,
    PitchBend
};

struct MidiEvent {
    MidiEventKind kind;
    int channel;
    int number;
    int value;     // 0–127 or pitchbend range
};

// Control Source Types
enum class ControlSourceKind {
    MidiCC,
    MidiNote,
    MidiPitchBend,
    HostParam,
    VirtualKnob,
    VirtualXY
};

// Control Target Types
enum class ControlTargetKind {
    InstrumentParam,
    AutomationLane,
    IntentStrength,
    IntentProfileMix,
    SceneMorph
};

// Control Structures
struct ControlSource {
    ControlSourceKind kind;
    int channel = 0;
    int number = -1;        // CC number / note
    String name;
};

struct ControlTarget {
    ControlTargetKind kind;
    String target_id;       // param name / lane name / intent id
};

struct ControlMapping {
    String mapping_id;
    ControlSource source;
    ControlTarget target;

    double min_value = 0.0;
    double max_value = 1.0;
    double curve = 1.0;     // 1.0 = linear, <1 exp, >1 log

    bool bipolar = false;
    String note;
};

struct ControlMap {
    String map_id;
    String name;
    Vector<ControlMapping> mappings;
};

// Main Control Engine
class ControlMappingEngine {
public:
    static Result<ControlMap> CreateDefaultMapForInstrument(
        const InstrumentGraph& instrument
    );

    static Result<void> ApplyControlValue(
        InstrumentRuntime& runtime,
        const ControlMapping& mapping,
        double normalized_value
    );

    static Result<void> ApplyMidiEvent(
        InstrumentRuntime& runtime,
        const ControlMap& map,
        const MidiEvent& event
    );
};

#endif // PROTOVM_CONTROL_MAPPING_H