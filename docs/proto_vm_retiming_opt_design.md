# ProtoVM Retiming Optimization Layer Design

## Overview

The Retiming Optimization Layer provides an automated selection and evaluation system for retiming plans. It builds upon the retiming analysis and application infrastructure to provide intelligent, objective-driven retiming optimization under user-configurable constraints.

## Architecture

The optimization layer sits between:
- **Lower Layer**: Retiming analysis and transformation engines
- **Upper Layer**: CLI/daemon commands and CoDesigner API

## Components

### 1. Objective Types

#### `RetimingObjectiveKind`
- `MinimizeMaxDepth`: Primary goal to reduce critical combinational depth
- `MinimizeDepthWithBudget`: Reduce depth while respecting register/move budgets
- `BalanceStages`: Improve stage balance by spreading depth more evenly

#### `RetimingObjective`
- `kind`: The optimization objective
- Constraints:
  - `max_extra_registers`: Maximum allowed register growth
  - `max_moves`: Maximum number of moves to apply
  - `target_max_depth`: Desired upper bound on critical depth

### 2. Evaluation Metrics

#### `RetimingPlanScore`
- Performance metrics:
  - `estimated_max_depth_before`/`after`: Depth before/after applying plan
- Move breakdown:
  - `applied_move_count`: Total moves that would be applied
  - `safe_move_count`: Number of safe moves
  - `suspicious_move_count`: Number of suspicious moves
  - `forbidden_move_count`: Number of forbidden moves
- Resource metrics:
  - `estimated_register_count_before`/`after`: Register count before/after
- Quality indicators:
  - `respects_cdc_fences`: Whether plan respects clock domain crossing constraints
  - `meets_objective`: Whether plan satisfies constraints
  - `cost`: Scalar cost value for ranking

### 3. Optimization Result

#### `RetimingOptimizationResult`
- `target_id`: Block or subsystem ID being optimized
- `objective`: Original optimization objective
- `plan_scores`: Array of evaluated plans with scores (sorted by cost)
- `best_plan_id`: ID of recommended plan
- `applied`: Whether best plan was auto-applied
- `application_result`: Result of applying the plan (if applicable)

### 4. Optimization Engine

#### `RetimingOptimizer`
- `EvaluateRetimingPlans()`: Evaluate a set of plans without applying them
- `EvaluateAndApplyBestPlanInBranch()`: Evaluate plans and auto-apply the best one

## Cost Model

The cost model is objective-dependent:

### MinimizeMaxDepth
- Base cost: `estimated_max_depth_after`
- Penalty for suspicious moves
- Large penalty for forbidden moves

### MinimizeDepthWithBudget
- Base cost: `estimated_max_depth_after`
- Penalties for violating:
  - Target depth constraint
  - Move budget constraint  
  - Register budget constraint
- Penalty for suspicious moves
- Large penalty for forbidden moves

### BalanceStages
- Base cost: `estimated_max_depth_after`
- Secondary cost factors for stage balance (implementation dependent)
- Penalties for suspicious/forbidden moves

## CLI Commands

### `retime-opt-block`
Optimize retiming for a single block:

```
proto-vm-cli --command retime-opt-block \
  --workspace <path> \
  --session-id <id> \
  --branch <name> \
  --block-id <id> \
  --objective <MinimizeMaxDepth|MinimizeDepthWithBudget|BalanceStages> \
  --target-max-depth <int> \
  --max-moves <int> \
  --max-extra-registers <int> \
  --apply \
  --apply-only-safe \
  --allow-suspicious
```

### `retime-opt-subsystem`
Optimize retiming for a subsystem:

```
proto-vm-cli --command retime-opt-subsystem \
  --workspace <path> \
  --session-id <id> \
  --branch <name> \
  --subsystem-id <id> \
  --block-ids <id1,id2,...> \
  --objective <MinimizeMaxDepth|MinimizeDepthWithBudget|BalanceStages> \
  --target-max-depth <int> \
  --max-moves <int> \
  --max-extra-registers <int> \
  --apply \
  --apply-only-safe \
  --allow-suspicious
```

## CoDesigner Integration

### `designer-retime-opt`
High-level optimization endpoint for AI clients:

```
{
  "command": "designer-retime-opt",
  "payload": {
    "designer_session_id": "cd-1234",
    "target": "block",            // or "subsystem"
    "block_id": "B_PIPE",         // when target="block"
    "subsystem_id": "ALU_PIPE",   // when target="subsystem"
    "block_ids": ["B1", "B2"],    // when target="subsystem"
    "objective": "MinimizeMaxDepth",
    "target_max_depth": 6,
    "max_moves": 10,
    "max_extra_registers": 5,
    "apply": true,
    "apply_only_safe": true,
    "allow_suspicious": false
  }
}
```

## Integration Points

### With Retiming Analysis
- Uses `CircuitFacade::AnalyzeRetimingForBlockInBranch()` to get candidate plans
- Leverages `PipelineMap`, `TimingAnalysis`, and `CdcReport` for accurate scoring

### With Retiming Application  
- Uses `CircuitFacade::ApplyRetimingPlanForBlockInBranch()` for auto-application
- Respects safety constraints during application

### With Existing Infrastructure
- Reuses existing transformation engine
- Integrates with branch-aware operations
- Supports session management and event logging

## Limitations

- **Local Optimization**: Single retiming round per invocation
- **Greedy Algorithm**: Heuristic selection rather than global optimization
- **Within-Branch**: No cross-branch comparison
- **Simple Cost Model**: Basic linear combination of metrics

## Future Extensions

1. **Multi-Objective Optimization**: Support for simultaneous optimization of multiple metrics
2. **Global Search**: More sophisticated search algorithms (beam search, genetic algorithms)
3. **Multi-Round Retiming**: Sequential application of multiple optimization rounds
4. **Advanced Cost Modeling**: More sophisticated resource and timing models
5. **Performance Optimization**: Caching and early termination heuristics

## Implementation Details

The optimization layer implements a three-stage process:

1. **Evaluation**: Score all candidate plans using the cost model
2. **Selection**: Choose best plan based on cost and constraint satisfaction
3. **Application**: Optionally apply the selected plan to the circuit

This design enables ProtoVM to move from "can retime" to "can optimize retiming under constraints", providing AI clients with a powerful tool for automatically improving pipeline depth and timing characteristics with controlled cost.