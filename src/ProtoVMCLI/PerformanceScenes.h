#ifndef PROTOVM_PERFORMANCE_SCENES_H
#define PROTOVM_PERFORMANCE_SCENES_H

#include "InstrumentGraph.h"
#include "InstrumentRuntime.h"
#include "IntentProfiles.h"
#include "IntentSessions.h"
#include "AutomationExport.h"
#include "ControlMapping.h"  // For extending ControlTargetKind
#include "Result.h"
#include "String.h"
#include <Vector.h>
#ifdef USE_UPP
    #include <ProtoVM/ProtoVM.h>  // Include U++ types
#else
    #include "../ProtoVM/MockProtoVM.h"  // Include mock types
#endif  // For U++ types

// Data structures for performance scenes
struct SceneSnapshot {
    String scene_id;
    String name;
    Upp::ValueMap params;                    // resolved instrument params
    Vector<AutomationLane> automation;       // baseline lanes
    Upp::ValueMap intent_strengths;          // intent_id → strength
};

struct SceneSet {
    String set_id;
    String name;
    Vector<SceneSnapshot> scenes;
};

struct MorphState {
    String from_scene_id;
    String to_scene_id;
    double morph = 0.0;   // 0.0 → 1.0
};

// Builder for creating scene snapshots
class SceneBuilder {
public:
    static Result<SceneSnapshot> BuildSceneFromProfile(
        const IntentProfile& profile,
        const InstrumentGraph& base_instrument
    );

    static Result<SceneSnapshot> BuildSceneFromSession(
        const IntentSessionState& session,
        const InstrumentGraph& base_instrument
    );
};

// Engine for morphing between scene snapshots
class SceneMorphEngine {
public:
    static Result<void> ApplyMorph(
        InstrumentRuntime& runtime,
        const SceneSnapshot& a,
        const SceneSnapshot& b,
        double morph
    );
};

#endif // PROTOVM_PERFORMANCE_SCENES_H