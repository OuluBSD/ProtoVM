# ProtoVM Retiming Application Layer Design

## Overview

The Retiming Application Layer is the implementation of Phase 21: Retiming Application - Turning Plans into Structural Transformations. This layer bridges the gap between the retiming planning phase (Phase 20A) and actual circuit modifications, converting `RetimingPlan` objects into `TransformationPlan` objects that can modify the circuit structure.

## Purpose

The Retiming Application Layer allows ProtoVM to:

- Transform retiming plans into actual circuit modifications
- Safely apply retiming moves within a single clock domain
- Respect CDC (Clock Domain Crossing) constraints
- Preserve block-level functional interfaces during retiming
- Provide CLI and daemon commands to apply retiming plans

## Core Components

### RetimingApplicationOptions
```cpp
struct RetimingApplicationOptions {
    bool apply_only_safe_moves = true;  // ignore non-SafeIntraDomain moves
    bool allow_suspicious_moves = false; // if true, include Suspicious in generated plans
    int  max_moves = -1;                // limit number of moves applied per plan (-1 = no limit)
};
```

### RetimingApplicationResult
```cpp
struct RetimingApplicationResult {
    String plan_id;                 // id of the RetimingPlan
    String target_id;               // block or subsystem id

    // IDs of moves that were actually applied.
    Vector<String> applied_move_ids;
    Vector<String> skipped_move_ids; // moves skipped due to safety/limits

    // New circuit revision after applying transformations (if tracked).
    int new_circuit_revision = -1;

    // Optional summary numbers:
    int estimated_max_depth_before = -1;
    int estimated_max_depth_after = -1;

    // Whether all applied moves were SafeIntraDomain.
    bool all_moves_safe = true;
};
```

## Core Engine: RetimingTransform

### BuildTransformationPlanForRetiming
Converts a RetimingPlan into a TransformationPlan (without applying). This function maps each `RetimingMove` to one or more `TransformationStep` entries that represent the circuit modifications needed.

### ApplyRetimingPlanInBranch
A convenience wrapper that builds and applies transformation plans directly to a given branch/session. This function:
1. Calls `BuildTransformationPlanForRetiming` to get a `TransformationPlan`
2. Uses the existing Transformation/Refactor engine to apply the plan to the underlying netlist in the given branch
3. Updates session metadata and event logs
4. Constructs and returns `RetimingApplicationResult`

## CircuitFacade Integration

The CircuitFacade provides branch-aware retiming application helpers:

```cpp
// For single blocks
Result<RetimingApplicationResult> ApplyRetimingPlanForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const RetimingPlan& plan,
    const RetimingApplicationOptions& options
);

// For subsystems
Result<RetimingApplicationResult> ApplyRetimingPlanForSubsystemInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const RetimingPlan& plan,
    const RetimingApplicationOptions& options
);
```

## Command-Line Interface

### Commands for retiming application:

1. `retime-block-apply` - Apply a specific retiming plan to a block
2. `retime-subsystem-apply` - Apply a specific retiming plan to a subsystem

#### Common options:
- `--workspace`
- `--session-id`
- `--branch <name>` (optional, default current)
- `--plan-id <id>` (to select a specific RetimingPlan by id)
- Optional:
  - `--apply-only-safe` (default true)
  - `--allow-suspicious` (default false)
  - `--max-moves <int>`

#### Example usage:
```
ProtoVMCLI retime-block --workspace ... --session-id 1 --branch main --block-id B_PIPE
ProtoVMCLI retime-block-apply --workspace ... --session-id 1 --branch main --plan-id RTP_B_PIPE_1
```

## Daemon/CoDesigner Integration

### Designer Command:
- `designer-retime-apply` - AI-friendly endpoint for applying retiming suggestions

#### Inputs:
```json
{
  "designer_session_id": "cd-1234",
  "target": "block",           // or "subsystem"
  "plan_id": "RTP_B_PIPE_1",
  "apply_only_safe": true,
  "allow_suspicious": false,
  "max_moves": 10
}
```

## Safety Rules

### Intra-Domain Only
- Retiming moves are strictly **intra-domain** in this phase
- No cross-clock domain retiming is performed
- CDC constraints from Phase 20B are treated as **hard constraints**

### Move Safety
- Only `SafeIntraDomain` moves are applied by default
- `Suspicious` moves require explicit permission via options
- `Forbidden` moves are never applied automatically

### Interface Preservation
- External latency at block boundaries is preserved
- Block I/O protocols remain unchanged
- Functional interfaces are maintained during internal pipeline restructuring

## Implementation Details

### Mapping RetimingMove → TransformationStep
The application layer maps each retiming move to transformation steps that:
- Rewire register outputs to pass through additional nodes
- Reassign capture points of signals from one register to another point in the cone
- Insert intermediate register components if needed (though for Phase 21, moves are modeled as rewiring of existing register endpoints)

### Branch-Aware Operations
All operations are branch-aware, meaning changes are applied to:
- The specified branch in the session
- New circuit revision tracking
- Proper session metadata updates

## Limitations

1. **No Cross-Domain Retiming**: Strictly same `domain_id` moves only
2. **No Formal Equivalence**: Behavior preservation is heuristic-based on existing analysis
3. **No Protocol Changes**: I/O latency at block boundaries remains unchanged
4. **Safety Heuristic**: Moves marked as `Suspicious` or `Forbidden` cannot be applied automatically

## Workflow

The retiming application workflow follows the pattern:
1. `retime-*` (analysis) → generates candidate RetimingPlans
2. User selects a plan ID
3. `retime-*-apply` (execution) → applies the selected plan
4. For AI integration: `designer-retime` → `designer-retime-apply`

## Future Enhancements

1. **Formal Equivalence Checking**: Integration with formal verification tools
2. **Multi-plan Optimization**: Sequencing multiple plans for better results
3. **Rollback Mechanisms**: Automatic rollback on equivalence failures
4. **Cross-Domain Extensions**: Safe CDC-aware retiming for future phases

## Data Flow

The retiming application layer processes data in this flow:
```
RetimingPlan → RetimingApplicationOptions → TransformationPlan → Circuit Modifications → RetimingApplicationResult
```

This ensures that high-level retiming plans are safely and systematically converted to actual circuit changes while preserving the necessary safety constraints.