# ProtoVM Performance Scenes & Preset Morphing Design

## Overview

The ProtoVM Performance Scenes & Preset Morphing system provides a powerful performance-layer abstraction that enables expressive transitions between different instrument states. Building on IntentProfiles, AutomationExport, and ControlMapping, this system allows ProtoVM instruments to hold multiple named "scenes" and morph smoothly between them in real time while remaining deterministic, QA-verifiable, and replayable.

## Core Concepts

### Performance Scene
A **Performance Scene** represents a coherent instrument state that includes:
- A resolved IntentProfile (or session snapshot)
- Static parameters + automation baselines
- Intent strengths
- Optional descriptive metadata ("Bright Chorus", "Dark Pad")

### Preset Morphing
**Preset Morphing** allows smooth blending between two scenes with:
- Continuous morph values from 0.0 (from scene) to 1.0 (to scene)
- Applied to parameters, automation lanes, and intent strengths
- Designed to be smooth, bounded, and real-time safe

The fundamental principle: *Scenes are states, morphing is movement between states.*

## Data Structures

### SceneSnapshot
```cpp
struct SceneSnapshot {
    String scene_id;
    String name;
    ValueMap params;                    // resolved instrument parameters
    Vector<AutomationLane> automation;  // baseline automation lanes
    ValueMap intent_strengths;          // intent_id → strength mapping
};
```

### SceneSet
```cpp
struct SceneSet {
    String set_id;
    String name;
    Vector<SceneSnapshot> scenes;
};
```

### MorphState
```cpp
struct MorphState {
    String from_scene_id;
    String to_scene_id;
    double morph = 0.0;   // 0.0 → 1.0
};
```

## Key Components

### SceneBuilder
The SceneBuilder class creates scene snapshots from various sources:

```cpp
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
```

### SceneMorphEngine
The SceneMorphEngine provides the core morphing functionality:

```cpp
class SceneMorphEngine {
public:
    static Result<void> ApplyMorph(
        InstrumentRuntime& runtime,
        const SceneSnapshot& a,
        const SceneSnapshot& b,
        double morph
    );
};
```

Morphing semantics:
- Parameters: linear interpolation between scene values
- Intent strengths: linear interpolation
- Automation baselines: blend lane values
- Missing values: fallback to nearest scene value

## Real-time Constraints

The system is designed with real-time safety in mind:
- No dynamic allocations during morph application
- No container resizing operations
- O(N) predictable computational cost
- Deterministic parameter resolution

## Control Integration

The system extends the `ControlTargetKind` enum to include `SceneMorph`, allowing controls to drive morph positions:

```cpp
enum class ControlTargetKind {
    InstrumentParam,
    AutomationLane,
    IntentStrength,
    IntentProfileMix,
    SceneMorph        // NEW
};
```

This enables:
- Knobs/CC → morph position control
- XY pad → morph between 4 scenes (optional advanced feature)
- Host automation → morph lanes

## Integration Points

### CircuitFacade Integration
The CircuitFacade provides methods for scene management:
- `CreateSceneSetForInstrumentInBranch`: Create scene sets for instruments
- `ApplySceneMorphInBranch`: Apply morph state to active branches
- `RenderSceneMorphPreviewInBranch`: Generate audio previews of morph transitions

### Command Line Interface
The system provides several CLI commands:
- `scene-create`: Create scenes from profiles or sessions
- `scene-set-create`: Group scenes into sets
- `scene-set-show`: Display scene sets
- `scene-morph`: Apply morph between two scenes
- `scene-morph-render`: Render audio preview of morph transitions

### CoDesigner Endpoints
AI agents can interact with the system through endpoints:
- `designer-scene-create`: Create scenes from intent profiles
- `designer-scene-morph`: Generate morphed scene sets
- `designer-scene-set-create`: Create collections of scenes

These allow AI agents to construct expressive performance layouts, evaluate musical transitions, and tune morph curves safely.

## Implementation Details

### Morphing Algorithm
The morphing algorithm performs linear interpolation between scene parameters:

For parameters present in both scenes A and B:
```
interpolated_value = value_A + morph * (value_B - value_A)
```

For parameters present in only one scene:
- If only in A: Use A's value directly
- If only in B: Scale B's value by morph position

### Scene Determinism
Scene snapshots are fully resolved and self-contained:
- No references back to mutable session state
- Deterministic resolution from intent profiles
- Stable parameter values regardless of external state

## Performance and Safety

### Memory Management
- All critical paths avoid dynamic allocations
- Pre-allocated buffers where possible
- Efficient data structures optimized for audio performance

### Error Handling
- Comprehensive error handling throughout the system
- Graceful degradation when scenes are unavailable
- Clear error messages for debugging morph issues

## Use Cases

### Live Performance
Musicians can perform continuous transitions between different instrument sounds, creating evolving textures and evolving soundscapes without discrete jumps between presets.

### Sound Design
Sound designers can create smooth transitions between different character states of an instrument, such as "warm" to "bright" or "dark" to "airy".

### Automation
DAWs and sequencers can automate morph positions over time, creating complex evolving sounds automatically.

## Future Enhancements

### Advanced Morphing
- Non-linear morph curves (logarithmic, exponential, custom)
- Multi-dimensional morphing (X/Y pad control between 4 scenes)
- Morphing with different timing for different parameters

### Scene Management
- Scene banks and organization systems
- Scene preset libraries
- Import/export of scene collections

## Comparison with Other Systems

### Scene vs Profile vs Session
- **Intent Profiles**: Abstract musical intentions, resolved to concrete parameters
- **Sessions**: Mutable workflow states with history
- **Scenes**: Resolved, static instrument states ready for performance

## Conclusion

The ProtoVM Performance Scenes & Preset Morphing system transforms ProtoVM instruments from static sounds to expressive, continuously-variable instruments. By providing smooth, deterministic transitions between different musical states, the system enables musicians to explore the space between presets and create evolving, dynamic performances while maintaining the technical integrity and reproducibility that ProtoVM provides.

This represents a significant step forward in making algorithmically-designed instruments suitable for expressive musical performance.