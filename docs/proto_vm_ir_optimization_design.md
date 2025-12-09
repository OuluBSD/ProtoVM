# ProtoVM IR Optimization Design

## Overview

ProtoVM provides an intermediate representation (IR) optimization layer that operates on the HLS-lite IR. This optimization system takes IR generated from circuit analysis and applies a set of local optimization passes, then bridges these optimizations to the existing transformation system to create concrete refactoring plans.

## IR Optimization Model

### IrOptPassKind

Enumeration of available optimization passes:

- `SimplifyAlgebraic`: Simplifies algebraic expressions (X & X → X, X | X → X, X ^ X → 0, ~~X → X)
- `FoldConstants`: Evaluates constant expressions (X & 1 → X, X & 0 → 0, etc.)
- `SimplifyMux`: Simplifies multiplexer expressions (Mux(SEL, A, A) → A)
- `EliminateTrivialLogic`: Removes trivial logic operations where possible

### IrOptChangeSummary

Structure that summarizes the changes made by a single optimization pass:

```cpp
struct IrOptChangeSummary {
    IrOptPassKind pass_kind;        // Type of pass that was run
    int expr_changes;              // Number of expression changes made
    int reg_changes;               // Number of register assignment changes made
    bool behavior_preserved;       // Whether behavior is heuristically preserved
};
```

### IrOptimizationResult

Result structure containing the optimization outcome:

```cpp
struct IrOptimizationResult {
    IrModule original;             // Original IR before optimization
    IrModule optimized;            // IR after applying optimizations
    std::vector<IrOptChangeSummary> summaries;  // Summary of passes applied
};
```

## Core Components

### IrOptimizer Class

The main optimization engine that applies a sequence of optimization passes to an IR module:

```cpp
class IrOptimizer {
public:
    Result<IrOptimizationResult> OptimizeModule(
        const IrModule& module,
        const std::vector<IrOptPassKind>& passes_to_run
    );
};
```

### Behavior Preservation Verification

A heuristic-based verification system that checks whether IR optimizations preserve the behavioral semantics:

```cpp
Result<bool> VerifyIrOptimizationBehaviorPreserved(
    const BehaviorDescriptor& before_behavior,
    const BehaviorDescriptor& after_behavior
);
```

The system verifies:
- Behavior kind preservation
- Bit width preservation
- Port count and role preservation

### IrToTransformationBridge

A bridge component that converts IR differences into transformation plans:

```cpp
class IrToTransformationBridge {
public:
    static Result<std::vector<TransformationPlan>> PlansFromIrDiff(
        const IrModule& original,
        const IrModule& optimized,
        const IrDiff& ir_diff,
        const std::string& block_id
    );
};
```

This bridge currently recognizes:
- `SimplifyDoubleInversion`: Patterns where double negations (~~X) are simplified
- `SimplifyRedundantGate`: Patterns like X & X or X | X that are simplified to X

## CircuitFacade Integration

### Branch-Aware IR Optimization Methods

The `CircuitFacade` provides branch-aware methods for IR optimization:

#### `OptimizeBlockIrInBranch`

```cpp
Result<IrOptimizationResult> OptimizeBlockIrInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id,
    const std::vector<IrOptPassKind>& passes_to_run
);
```

This method:
1. Gets the IR for the specified block in the given branch
2. Runs the requested optimization passes
3. Applies behavior preservation verification
4. Returns the optimization result

#### `ProposeIrBasedTransformationsForBlock`

```cpp
Result<std::vector<TransformationPlan>> ProposeIrBasedTransformationsForBlock(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id,
    const std::vector<IrOptPassKind>& passes_to_run
);
```

This method:
1. Gets the original IR for the specified block
2. Runs optimization passes
3. Computes the IR difference between original and optimized
4. Converts IR differences to transformation plans via the bridge
5. Returns the transformation plans

## Commands

### CLI Commands

#### `ir-opt-block`

Purpose: Preview IR-level optimization for a block without applying changes.

Usage:
```
proto-vm-cli ir-opt-block --workspace PATH --session-id ID --block-id BLOCK_ID [--branch BRANCH_NAME] [--passes PASS1,PASS2,...]
```

Parameters:
- `--workspace`: Path to workspace directory
- `--session-id`: Session identifier
- `--block-id`: Block identifier to optimize
- `--branch`: Optional branch name (defaults to current branch)
- `--passes`: Optional comma-separated list of passes to run (defaults to all)

Output:
```json
{
  "session_id": 1,
  "branch": "main",
  "block_id": "B3",
  "optimization": {
    "original": { ... IrModule ... },
    "optimized": { ... IrModule ... },
    "summaries": [
      {
        "pass_kind": "SimplifyAlgebraic",
        "expr_changes": 2,
        "reg_changes": 0,
        "behavior_preserved": true
      }
    ]
  }
}
```

#### `ir-opt-refactor-block`

Purpose: Compute IR optimizations and generate transformation plans directly.

Usage:
```
proto-vm-cli ir-opt-refactor-block --workspace PATH --session-id ID --block-id BLOCK_ID [--branch BRANCH_NAME] [--passes PASS1,PASS2,...]
```

Parameters: Same as `ir-opt-block`.

Output:
```json
{
  "session_id": 1,
  "branch": "main",
  "block_id": "B3",
  "plans": [
    {
      "id": "IR_T1",
      "kind": "SimplifyDoubleInversion",
      "target": { "subject_id": "B3", "subject_kind": "Block" },
      "guarantees": ["BehaviorKindPreserved", "IOContractPreserved"],
      "steps": [
        { "description": "Remove redundant NOT-then-NOT around SUM path" }
      ]
    }
  ]
}
```

### Daemon Commands

The daemon supports the same functionality via JSON-RPC:

#### `"ir-opt-block"` command

Request:
```json
{
  "id": "1",
  "command": "ir-opt-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "block_id": "B3",
    "branch": "main",
    "passes": ["SimplifyAlgebraic", "FoldConstants"]
  }
}
```

Response follows the same structure as CLI output.

#### `"ir-opt-refactor-block"` command

Request:
```json
{
  "id": "1",
  "command": "ir-opt-refactor-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "block_id": "B3",
    "branch": "main",
    "passes": ["SimplifyAlgebraic", "FoldConstants"]
  }
}
```

Response follows the same structure as CLI output.

## Usage Examples

### CLI Examples

Basic optimization with all passes:
```
proto-vm-cli ir-opt-block --workspace /path/to/workspace --session-id 1 --block-id B3
```

With specific passes:
```
proto-vm-cli ir-opt-block --workspace /path/to/workspace --session-id 1 --block-id B3 --passes SimplifyAlgebraic,FoldConstants
```

Generate refactoring plans:
```
proto-vm-cli ir-opt-refactor-block --workspace /path/to/workspace --session-id 1 --block-id B3
```

### Daemon Examples

For the daemon, send JSON requests like:
```
{"id": "1", "command": "ir-opt-block", "workspace": "/path/to/workspace", "session_id": 1, "payload": {"block_id": "B3", "branch": "main"}}
```

## Behavior Preservation Guarantees

The IR optimization system uses heuristic checks to determine if behavior is preserved:

1. **Conservative Approach**: When in doubt, optimizations are marked as potentially not behavior-preserving
2. **Structural Checks**: The system verifies that behavior kind, bit width, and port roles remain consistent
3. **Local Optimizations Only**: All implemented optimizations are local transformations that should preserve semantics
4. **Verification Pipeline**: IR differences are checked against behavioral descriptors before marking as safe

The system prioritizes safety over optimization effectiveness - if a transformation cannot be proven safe, it is conservatively marked as potentially unsafe.