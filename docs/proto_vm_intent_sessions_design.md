# ProtoVM Intent Sessions Design Documentation

## Overview

Intent Sessions represent a stateful, persistent system for managing musical intent transformations over time. Unlike the stateless Phase 29 musical intent system, Intent Sessions maintain:

- Current instrument state
- History of applied intent plans
- Audit trail of transformations
- Support for undo/redo operations
- Deterministic replay capabilities

## Intent Session Concept

An Intent Session provides a continuous workflow for evolving a musical instrument through multiple intent transformations. Each session:

- Maintains a current instrument state
- Records a sequence of intent steps (proposed and applied)
- Tracks the current position in the evolution (head_step_index)
- Supports deterministic replay to recreate instrument states

## Data Model

### IntentSessionMetadata
```cpp
struct IntentSessionMetadata {
    String intent_session_id;     // stable id, e.g. "is-000001"
    String created_at;
    String created_with;          // version string
    String current_branch;        // branch this session is attached to
    int instrument_revision = 0;  // increments per accepted step
    int head_step_index = -1;     // current applied step index (for undo/redo)
    IntentSessionStatus status = IntentSessionStatus::Active;
};
```

### IntentStepRecord
```cpp
struct IntentStepRecord {
    int step_index = 0;
    String user_id;               // optional caller id
    IntentRequest request;
    IntentPlan plan;              // includes before/after QA reports + diff + accepted
    bool applied = false;         // true if applied to current instrument state
    String note;                  // optional user note
};
```

### IntentSessionState
```cpp
struct IntentSessionState {
    IntentSessionMetadata meta;
    InstrumentGraph current_instrument;  // Current working instrument
    Vector<IntentStepRecord> steps;      // History of steps
};
```

## Persistence Layout

Intent sessions are stored under the workspace/session directory with the following layout:

```
intent_sessions/
├── <intent_session_id>/
│   ├── intent_session.json        # metadata and summary
│   ├── instrument_current.json    # current instrument graph
│   ├── steps/
│   │   ├── step_00000000.json
│   │   ├── step_00000001.json
│   │   └── ...
│   └── artifacts/                 # optional exports (wav, QA reports)
```

All writes use atomic operations (write to temp file then rename) to ensure consistency.

## Command List & Examples

### intent-session-create
Initialize a new intent session with an initial instrument.

```bash
proto-vm-cli intent-session-create \
  --workspace=/path/to/workspace \
  --session-id=12345 \
  --branch=main \
  --user-id=alice \
  --instrument-from-json=/path/to/instrument.json
```

### intent-session-propose
Propose a new intent transformation step (not yet applied).

```bash
proto-vm-cli intent-session-propose \
  --workspace=/path/to/workspace \
  --session-id=12345 \
  --intent-session-id=is-000001 \
  --user-id=alice \
  --intent=warmer,brighter \
  --strength=0.7 \
  --duration-sec=2.0 \
  --allow-regression=false
```

### intent-session-apply
Apply a previously proposed step.

```bash
proto-vm-cli intent-session-apply \
  --workspace=/path/to/workspace \
  --session-id=12345 \
  --intent-session-id=is-000001 \
  --step-index=0 \
  --require-accepted=true
```

### intent-session-undo
Move back one step in the transformation history.

```bash
proto-vm-cli intent-session-undo \
  --workspace=/path/to/workspace \
  --session-id=12345 \
  --intent-session-id=is-000001
```

### intent-session-redo
Re-apply the next step after an undo.

```bash
proto-vm-cli intent-session-redo \
  --workspace=/path/to/workspace \
  --session-id=12345 \
  --intent-session-id=is-000001
```

### intent-session-status
Get the current state of the intent session.

```bash
proto-vm-cli intent-session-status \
  --workspace=/path/to/workspace \
  --session-id=12345 \
  --intent-session-id=is-000001 \
  --full  # optional: include full step plans
```

### intent-session-replay
Apply the same intent sequence to a different initial instrument.

```bash
proto-vm-cli intent-session-replay \
  --workspace=/path/to/workspace \
  --session-id=12345 \
  --intent-session-id=is-000001 \
  --instrument-from-json=/path/to/other-instrument.json \
  --up-to-step=2 \
  --out=/path/to/result.json
```

## Determinism Guarantee

Intent sessions ensure determinism through replay-based operations. When undoing or redoing:

1. The system returns to the initial instrument state
2. Applies all steps up to the target position in sequence
3. This ensures consistent results regardless of previous operations

## Undo/Redo Semantics

- Undo moves `head_step_index` back by one and reconstructs `current_instrument` by replaying applied steps from the initial instrument
- Redo moves `head_step_index` forward and applies the next step
- No "inverse transforms" are used - all state reconstruction is done through forward replay

## Collaboration Considerations

- User IDs are stored as metadata only
- No multi-user conflict resolution is provided (future enhancement)
- Sessions are tied to specific branches but can be replayed to other instruments

## Security and Privacy

- User IDs are optional metadata
- No authentication required (aligns with existing ProtoVM architecture)
- No audio buffers stored in JSON - only metrics and artifact paths