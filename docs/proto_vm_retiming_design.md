# ProtoVM Retiming Design

## Overview

The retiming planning layer enables ProtoVM to analyze digital circuits and identify safe intra-clock-domain retiming opportunities. This planning system proposes moves of registers along combinational cones to potentially reduce critical path delays without altering the external behavior of circuit blocks or subsystems.

**Important**: This phase only *plans* retiming moves but does not apply them. Actual transformations will be handled in a later phase.

## Key Concepts

### Retiming
Retiming is a circuit optimization technique that involves moving registers within a circuit while preserving its functionality. By shifting register locations, the technique can help balance the pipeline stages and reduce the critical path delay.

### Intra-Domain Constraint
All retiming moves are restricted to a single clock domain:
- `domain_id` must match between source and destination ends of any modified path
- Cross-domain boundaries identified by `CdcReport` are treated as hard fences
- No moves are proposed that would cross clock domain boundaries

## Data Structures

### RetimingMoveDirection
Indicates the direction of register movement:
- `Forward`: Move register(s) closer to outputs/later in the pipeline
- `Backward`: Move register(s) closer to inputs/earlier in the pipeline

### RetimingMoveSafety
Classifies the safety level of a proposed move:
- `SafeIntraDomain`: No CDC issues, no known hazards
- `Suspicious`: Heuristically possible issues; needs review
- `Forbidden`: Should not be applied (e.g., crosses CDC boundaries)

### RetimingMove
Captures a single retiming opportunity:
- `move_id`: Stable ID for this move (e.g., "RTM_0001")
- `src_reg_id` / `dst_reg_id`: Register IDs involved in the path
- `direction`: Movement direction (Forward/Backward)
- `domain_id`: Clock domain of the move
- `src_stage_index` / `dst_stage_index`: Pipeline stage indices
- `before_comb_depth` / `after_comb_depth_est`: Combinational depth before/after move (estimated)
- `safety`: Safety classification
- `safety_reason`: Explanation for safety assessment
- `affected_ops`: List of intermediate operations affected

### RetimingPlan
Bundles related retiming moves with common goals:
- `id`: Plan ID (e.g., "RTP_ALU_BALANCE_1")
- `target_id`: Block or subsystem ID being analyzed
- `description`: Human-readable summary
- `moves`: Collection of `RetimingMove` objects
- `estimated_max_depth_before` / `after`: Aggregate depth estimates
- `respects_cdc_fences`: True if no moves cross CDC hazards

## Analysis Process

### Identifying Candidate Paths
1. Use `PipelineMap` to identify register-to-register paths
2. Filter for same-domain paths (`crosses_clock_domain == false`)
3. Focus on paths exceeding a threshold depth (e.g., > 4 levels)
4. Verify paths are not anchored by CDC crossings

### Respecting CDC Boundaries
- Any register involved in a `CdcCrossing` is treated as anchored
- No moves are proposed that would affect CDC-related registers
- All proposed moves are verified to avoid CDC boundaries

### Estimating Impact
- Estimate depth reduction based on simple heuristics
- For a move "in the middle" of a combinational path, the depth might be approximately halved
- Actual implementation uses `TimingAnalysis` and `ScheduledModule` for more precise estimates

## CLI Commands

### `retime-block`
Analyzes a single block for retiming opportunities:

```bash
protovm-cli retime-block --workspace=/path/to/workspace --session-id=1 --block-id=ALU_CORE --min-depth=5 --max-plans=10
```

#### Parameters
- `--workspace`: Path to workspace directory
- `--session-id`: ProtoVM session ID
- `--block-id`: Target block ID
- `--branch`: Branch name (optional, defaults to current branch)
- `--min-depth`: Minimum combinational depth threshold (optional, default: 5)
- `--max-plans`: Maximum plans to return (optional, default: 10)

#### Response
Returns a JSON object containing:
- Session and branch information
- List of `RetimingPlan` objects
- Depth estimates before/after for each plan

### `retime-subsystem`
Analyzes a multi-block subsystem for retiming opportunities:

```bash
protovm-cli retime-subsystem --workspace=/path/to/workspace --session-id=1 --subsystem-id=PIPELINE --block-ids=STAGE1,STAGE2,STAGE3 --min-depth=6
```

#### Parameters
- `--workspace`: Path to workspace directory
- `--session-id`: ProtoVM session ID
- `--subsystem-id`: Target subsystem ID
- `--block-ids`: Comma-separated list of block IDs
- `--branch`: Branch name (optional, defaults to current branch)
- `--min-depth`: Minimum combinational depth threshold (optional, default: 5)
- `--max-plans`: Maximum plans to return (optional, default: 10)

#### Response
Returns a JSON object similar to `retime-block` but covering the entire subsystem.

## CoDesigner Integration

### `designer-retime`
Provides retiming analysis through the CoDesigner interface:

```json
{
  "command": "designer-retime",
  "payload": {
    "designer_session_id": "cd-1234",
    "target": "block",             // or "subsystem"
    "block_id": "ALU_STAGE1",      // for block target
    "subsystem_id": "ALU_PIPE",    // for subsystem target
    "block_ids": ["STAGE1", "STAGE2", "STAGE3"], // for subsystem target
    "min_depth": 6,
    "max_plans": 10
  }
}
```

## JSON Serialization

All retiming data structures are serialized to JSON with consistent field naming:
- Enums serialized as string values ("Forward", "SafeIntraDomain", etc.)
- Collections serialized as arrays
- All fields follow camelCase naming convention

## Limitations and Future Work

### Current Limitations
- No actual retiming application (transformation phase is future work)
- No formal equivalence checking
- Heuristic-based depth estimates
- Intra-domain only (no cross-domain retiming)

### Future Enhancements
- Integration with formal verification for equivalence checking
- More precise timing models
- Cross-clock domain considerations (future phase)
- Conversion of `RetimingPlan` to `TransformationPlan` objects
- Advanced scheduling algorithms to optimize pipeline depth

## Safety Considerations

1. **CDC Compliance**: All moves respect CDC boundaries identified by the `CdcReport`
2. **Domain Isolation**: No moves cross clock domain boundaries
3. **Interface Preservation**: Moves do not alter external block interfaces
4. **Behavioral Integrity**: Internal moves should preserve functional behavior

The retiming planning layer provides the foundation for automated pipeline optimization while ensuring safety constraints are maintained.