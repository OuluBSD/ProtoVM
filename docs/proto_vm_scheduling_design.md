# ProtoVM Scheduled IR Design ("HLS-Mini")

## Overview

ProtoVM introduces a **Scheduled IR layer** that extends the existing HLS-lite IR (`IrModule`) with stage and cycle information. This "HLS-Mini" capability enables the analysis and description of how logic could be distributed across cycles/stages in pipelined implementations, without actually modifying the underlying circuit.

The scheduled IR is designed to be:
- **Branch-aware**: Operates on specific branches in the versioned circuit system
- **Read-only**: Provides analysis views without structural changes in this phase
- **Heuristic**: Uses conservative algorithms to infer reasonable stage assignments
- **Deterministic**: Produces consistent results for the same inputs

## Scheduled IR Model

### Core Data Structures

#### StageIndex

A simple integer type representing a stage index in a pipeline (0, 1, 2, ...).

```cpp
using StageIndex = int;
```

#### ScheduledOpId

Represents a scheduled operation derived from an `IrExpr` or `IrRegAssign`.

```cpp
struct ScheduledOpId {
    std::string name;   // e.g. "SUM_add", "TMP1_and"
};
```

#### ScheduledExpr

Represents a scheduled expression with stage information.

```cpp
struct ScheduledExpr {
    IrExpr expr;        // original expression
    StageIndex stage;   // stage where this expr is evaluated
};
```

#### ScheduledRegAssign

Represents a scheduled register assignment with stage information.

```cpp
struct ScheduledRegAssign {
    IrRegAssign reg_assign; // original reg assignment
    StageIndex stage;       // stage in which next-state value is considered "ready"
};
```

#### ScheduledModule

Top-level scheduled IR for a block/subcircuit.

```cpp
struct ScheduledModule {
    std::string id;                             // same as IrModule id
    std::vector<IrValue> inputs;
    std::vector<IrValue> outputs;

    int num_stages;                           // number of pipeline stages

    std::vector<ScheduledExpr> comb_ops;      // expressions annotated with stages
    std::vector<ScheduledRegAssign> reg_ops;  // register ops annotated with stages
};
```

## Scheduling Strategies

The system implements three scheduling strategies:

### 1. SingleStage (`SchedulingStrategy::SingleStage`)

- All combinational operations are assigned to stage 0
- All register operations are assigned to stage 0
- Provides a baseline for comparison
- Equivalent to a purely combinational view

### 2. DepthBalancedStages (`SchedulingStrategy::DepthBalancedStages`)

- Uses timing depth information to balance operations across stages
- Determines number of stages based on critical path depth and user's requested upper bound
- Assigns operations to stages using linear bucketing: `stage = (depth * num_stages) / (max_depth + 1)`
- Attempts to balance load across stages while respecting dependency order

### 3. FixedStageCount (`SchedulingStrategy::FixedStageCount`)

- Distributes operations across a user-specified number of stages
- Uses the same linear bucketing approach as DepthBalancedStages but with fixed final stage count
- Ensures operations are distributed across exactly the requested number of stages

## Scheduling Configuration

The `SchedulingConfig` struct specifies how scheduling should be performed:

```cpp
struct SchedulingConfig {
    SchedulingStrategy strategy;
    int requested_stages;       // used for FixedStageCount, else ignored or advisory
};
```

## Scheduling Engine

The `SchedulingEngine` class provides the core scheduling functionality:

```cpp
class SchedulingEngine {
public:
    // Build a ScheduledModule from an IrModule and optional timing/graph info.
    static Result<ScheduledModule> BuildSchedule(
        const IrModule& ir,
        const TimingAnalysis* timing,          // optional pointer
        const CircuitGraph* graph,            // optional pointer
        const SchedulingConfig& config
    );
};
```

### Algorithm Implementation

The scheduling engine follows this process:

1. **Depth Calculation**: Computes timing depths for each expression based on dependency structure
2. **Stage Count Determination**: Determines the number of stages based on the selected strategy
3. **Stage Assignment**: Assigns each operation to a specific stage based on its computed depth and the strategy
4. **Result Generation**: Creates the final `ScheduledModule` with annotated operations

## CircuitFacade Integration

The `CircuitFacade` provides branch-aware access to scheduled IR:

### BuildScheduledIrForBlockInBranch

Generates scheduled IR for a specific block in a specific branch:

```cpp
Result<ScheduledModule> BuildScheduledIrForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id,
    const SchedulingConfig& config
);
```

### BuildScheduledIrForNodeRegionInBranch

Generates scheduled IR for a small region around a specific node in a branch:

```cpp
Result<ScheduledModule> BuildScheduledIrForNodeRegionInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& node_id,
    const std::string& node_kind_hint,
    int max_depth,
    const SchedulingConfig& config
);
```

## CLI Commands

### `schedule-block`

Generate scheduled IR for a specific circuit block with configurable pipeline stages.

**Usage:**
```
proto-vm-cli schedule-block --workspace PATH --session-id ID --block-id BLOCK_ID [--branch BRANCH_NAME] --strategy STRATEGY [--stages N]
```

**Options:**
- `--strategy`: Scheduling strategy (SingleStage, DepthBalancedStages, FixedStageCount)
- `--stages`: Number of stages (for FixedStageCount strategy)

**Output:**
Returns a `ScheduledModule` JSON representation containing inputs, outputs, and operations annotated with stage information.

### `schedule-node-region`

Generate scheduled IR for a small region around a specific node with configurable pipeline stages.

**Usage:**
```
proto-vm-cli schedule-node-region --workspace PATH --session-id ID --node-id NODE_ID [--node-kind KIND] [--max-depth N] [--branch BRANCH_NAME] --strategy STRATEGY [--stages N]
```

**Options:**
- `--strategy`: Scheduling strategy (SingleStage, DepthBalancedStages, FixedStageCount)
- `--stages`: Number of stages (for FixedStageCount strategy)
- `--max-depth`: Maximum depth to explore from the node (default: 4)

**Output:**
Returns a `ScheduledModule` JSON representation for the local region around the specified node.

## Daemon Commands

The daemon supports the same functionality via JSON-RPC:

### `"schedule-block"` command

**Request:**
```json
{
  "id": "1",
  "command": "schedule-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "block_id": "B3",
    "branch": "main",
    "strategy": "DepthBalancedStages",
    "stages": 3
  }
}
```

**Response:**
```json
{
  "id": "1",
  "ok": true,
  "command": "schedule-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "B3",
    "scheduled_ir": {
      "id": "B3",
      "num_stages": 3,
      "inputs": [ ... ],
      "outputs": [ ... ],
      "comb_ops": [
        {
          "stage": 0,
          "expr": { ... IrExpr ... }
        },
        {
          "stage": 1,
          "expr": { ... IrExpr ... }
        }
      ],
      "reg_ops": [
        {
          "stage": 2,
          "reg_assign": { ... IrRegAssign ... }
        }
      ]
    }
  }
}
```

### `"schedule-node-region"` command

Similar to `schedule-block` but operates on a local region around a specified node.

## JSON Serialization

All scheduled IR structures are serializable to JSON with deterministic field ordering:

- `SchedulingStrategy` → string values: "SingleStage", "DepthBalancedStages", "FixedStageCount"
- `ScheduledExpr` → JSON object with `stage` and embedded `expr` fields
- `ScheduledRegAssign` → JSON object with `stage` and embedded `reg_assign` fields
- `ScheduledModule` → JSON object with `id`, `num_stages`, `inputs`, `outputs`, `comb_ops`, and `reg_ops` fields

## Use Cases

1. **Pipeline Analysis**: Understand how logic could be distributed across pipeline stages
2. **Critical Path Identification**: Identify operations that limit clock frequency in pipelined implementations
3. **Transformation Planning**: Feed scheduled IR into reasoning tools for pipeline optimization
4. **Verification**: Compare expected vs. actual pipeline behavior using structured stage information
5. **AI-assisted Design**: Provide stage-aware representations for AI tools to understand and modify circuits

## Limitations

- Scheduling is approximate and heuristic, not formal timing closure grade
- No automatic register insertion/removal in the underlying circuit in this phase
- Depth-based balancing does not consider actual gate delays or setup/hold times
- Assumes idealized pipeline behavior without considering stall/hazard conditions
- Stage assignment is conservative and may not achieve optimal performance

## Future Enhancements

1. **Timing-driven placement**: Incorporate actual gate delay models
2. **Advanced scheduling algorithms**: Implement more sophisticated resource-constrained scheduling
3. **Automatic pipeline register insertion**: Structural modifications to implement suggested pipelines
4. **Hazard detection**: Identify pipeline hazards and suggest mitigation strategies
5. **Multi-clock domain support**: Handle designs with multiple clock domains