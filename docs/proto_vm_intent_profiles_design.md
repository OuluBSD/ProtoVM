# ProtoVM Intent Profiles Design Document

## Overview

This document describes the Intent Profiles system in ProtoVM, which allows musical intent sequences to be named, stored, reused, parameterized, and shared as *personas* or *profiles*.

## Purpose

Intent Profiles provide a way to create reusable musical transformations that can be applied across different instruments and sessions, enabling workflows like:
- "Apply the *Warm Analog Pad* profile"
- "Use the *Tight Mono Bass* persona, but weaker"
- "Apply profile X to another instrument"
- "Compare two profiles by their audio QA footprint"

## Architecture

### Core Data Structures

#### IntentProfileScope
```cpp
enum class IntentProfileScope {
    Global,        // reusable across projects
    Workspace      // local to a workspace
};
```

#### IntentProfileMetadata
```cpp
struct IntentProfileMetadata {
    String profile_id;           // e.g. "ip-warm-analog-pad"
    String name;                 // human-readable
    String description;
    IntentProfileScope scope;
    String created_at;
    String created_with;
    String author;               // optional
    Vector<String> tags;         // e.g. ["pad", "analog", "warm"]
};
```

#### IntentProfileStep
```cpp
struct IntentProfileStep {
    IntentRequest request;       // same structure as Phase 29
    String note;                 // optional description
};
```

#### IntentProfile
```cpp
struct IntentProfile {
    IntentProfileMetadata meta;
    Vector<IntentProfileStep> steps;
};
```

### IntentProfileManager

The `IntentProfileManager` class provides static methods for profile operations:

```cpp
class IntentProfileManager {
public:
    static Result<IntentProfile> CreateProfile(
        const IntentProfileMetadata& meta,
        const Vector<IntentProfileStep>& steps,
        const String& base_dir
    );

    static Result<IntentProfile> LoadProfile(
        const String& base_dir,
        const String& profile_id
    );

    static Result<Vector<IntentProfileMetadata>> ListProfiles(
        const String& base_dir,
        Optional<String> tag_filter
    );

    static Result<IntentSessionState> ApplyProfileToSession(
        IntentSessionState& session,
        const IntentProfile& profile,
        const String& user_id,
        double strength_scale   // e.g. 0.5 = softer, 1.0 = normal
    );

    static Result<InstrumentGraph> ApplyProfileToInstrument(
        const InstrumentGraph& initial,
        const IntentProfile& profile,
        double strength_scale,
        bool require_accepted
    );
};
```

### Persistence Model

#### Global profiles
* `~/.protovm/intent_profiles/`
  * `<profile_id>/`
    * `profile.json`

#### Workspace profiles
* `<workspace>/intent_profiles/`
  * `<profile_id>/profile.json`

Profiles are stored as JSON files with atomic write operations and schema versioning.

## Key Features

### 1. Purely Declarative
- Intent profiles never contain executable code
- Only structured intent descriptions
- Deterministic replayability

### 2. Parameterization
- Profiles can be scaled with strength values
- Flexible application across different contexts

### 3. Tagging System
- Profiles can be tagged with descriptive keywords
- Efficient filtering and discovery

### 4. Deterministic Replay
- Profiles can be reliably applied across different instruments
- Consistent results guaranteed

### 5. Versioning
- Profiles are immutable once published
- Versioning instead of mutation

## Integration Points

### CircuitFacade Integration
```cpp
Result<IntentProfile> CreateIntentProfileFromSession(
    const IntentSessionState& session,
    const Vector<int>& step_indices,
    const IntentProfileMetadata& meta
);

Result<IntentSessionState> ApplyIntentProfileToSession(
    IntentSessionState& session,
    const String& profile_id,
    double strength_scale,
    bool require_accepted
);
```

### CLI Commands
- `intent-profile-create` - Create a profile from a session or explicit data
- `intent-profile-list` - List available profiles with optional tag filter
- `intent-profile-show` - Show profile details
- `intent-profile-apply` - Apply a profile to an instrument or session
- `intent-profile-diff` - Compare two profiles

### Designer Endpoints
- `designer-intent-profile-create`
- `designer-intent-profile-list`
- `designer-intent-profile-apply`
- `designer-intent-profile-diff`

## Workflow Examples

### Creating a Profile from a Successful Session
1. Complete an intent session with desired transformations
2. Create a profile from specific steps: `intent-profile-create --session-id <id> --step-indices 0,1,2 --profile-id ip-warm-analog-pad --name "Warm Analog Pad" --tags pad,analog,warm`

### Applying a Profile
1. Apply a profile with normal strength: `intent-profile-apply --profile-id ip-warm-analog-pad --instrument-id my-instrument`
2. Apply with weaker strength: `intent-profile-apply --profile-id ip-warm-analog-pad --instrument-id my-instrument --strength-scale 0.5`

### Comparing Profiles
1. Compare two profiles: `intent-profile-diff --profile-id-1 ip-warm-analog-pad --profile-id-2 ip-bright-lead`
2. Compare via Designer: `designer-intent-profile-diff --profile-id-1 ip-warm-analog-pad --profile-id-2 ip-bright-lead`

## Determinism Guarantees

Intent profiles are designed to be:
- **Reproducible**: Same profile applied to same instrument yields same result
- **Parameterized**: Strength scaling is consistent across applications
- **Verifiable**: Profile applications can be audited and compared

## Implementation Considerations

### Atomic Operations
- Profile creation uses atomic write operations
- Temporary files are used to ensure consistency
- File locking prevents corruption in concurrent access

### Validation
- Profile IDs must follow the "ip-" prefix convention
- Metadata is validated before creation
- Steps are validated during deserialization

### Error Handling
- Comprehensive error codes for all failure modes
- Clear error messages for debugging
- Graceful degradation when possible

## Non-Goals

- No machine learning
- No automatic profile generation
- No fuzzy matching
- No online sharing (yet)

## Future Enhancements

- Online profile sharing and marketplace
- AI-assisted profile creation
- Advanced profile comparison based on audio QA signatures
- Profile inheritance mechanisms
- A/B testing framework for comparing profiles