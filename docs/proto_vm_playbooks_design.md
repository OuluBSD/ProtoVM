# ProtoVM Playbooks Design Document

## 1. Overview

ProtoVM Playbooks provide a structured way to define and execute multi-step design workflows as single high-level commands. A playbook orchestrates existing ProtoVM capabilities to perform complex operations like:

- analyze → optimize → propose refactors → (optionally) apply → diff → codegen
- All in a single, deterministic, branch-aware operation
- Safe by default: does not auto-apply behavior-breaking changes
- Extensible to support new workflow patterns

Playbooks turn ProtoVM into an autonomous co-design executor, allowing AI systems to trigger rich, multi-step design optimization and inspection workflows with a single high-level command.

## 2. Playbook Concept

### 2.1 High-Level Idea

A **playbook** is a predefined, parameterized workflow that consists of several steps such as:

- Set focus to a block
- Analyze behavior + IR
- Run IR optimization passes
- Propose transformation plans
- (Optionally) apply selected plans
- Compute behavior/IR diffs against a baseline branch
- Generate code (pseudo-Verilog) for the final state

Each playbook:
- Is declarative: describes what to do, not how to call each low-level command
- Is executed by a single new daemon command that wraps multiple `designer-*` commands internally
- Maintains deterministic and branch-aware behavior
- Is safe: does not auto-apply behavior-changing transformations without explicit permission

### 2.2 Example Use-Cases

Two concrete playbooks are currently implemented:

1. **"OptimizeBlockAndReport"**:
   - For a given block: analyze → optimize IR → propose refactors (do not apply) → diff vs baseline → codegen
   - Returns a structured report

2. **"OptimizeAndApplySafeRefactors"**:
   - For a given block: analyze → optimize IR → propose refactors
   - Apply only refactors that are heuristically verified as behavior-preserving
   - Diff vs baseline → codegen
   - Returns summary of applied changes + updated views

## 3. Data Structures

### 3.1 PlaybookKind

Enumeration of available playbook types:

```cpp
enum class PlaybookKind {
    OptimizeBlockAndReport,
    OptimizeAndApplySafeRefactors,
    // System-level playbooks (Phase 18)
    SystemOptimizeAndReport,
    SystemOptimizeAndApplySafeRefactors,
    // extendable in the future
};
```

### 3.2 PlaybookConfig

Configuration struct used to parameterize a playbook run:

```cpp
struct PlaybookConfig {
    PlaybookKind kind;

    String designer_session_id;    // existing CoDesigner session
    String target;                 // "block" (existing) or "system" (new for Phase 18)
    String block_id;               // required when target == "block"

    // System-level parameters (Phase 18):
    Vector<String> block_ids;      // explicit list for system-level playbooks
    String name_prefix;            // optional prefix filter for system-level playbooks

    String baseline_branch;        // branch name for diff comparison
    Vector<IrOptPassKind> passes;  // IR optimization passes to run

    bool use_optimized_ir;         // hint if analysis/codegen should use optimized IR
    bool apply_refactors;          // whether to actually apply suggested refactors
};
```

### 3.3 BlockPlaybookResult

Structure used for per-block results in system-level playbooks:

```cpp
struct BlockPlaybookResult {
    String block_id;
    BehaviorDescriptor initial_behavior;
    BehaviorDescriptor final_behavior;
    IrModule initial_ir;
    IrModule final_ir;
    IrOptimizationResult optimization;
    Vector<TransformationPlan> proposed_plans;
    Vector<String> applied_plan_ids;
    int new_circuit_revision = -1;
    BehaviorDiff behavior_diff;
    IrDiff ir_diff;
    CodegenModule codegen;
};
```

### 3.4 PlaybookResult

Structure returned by a playbook execution:

```cpp
struct PlaybookResult {
    PlaybookKind kind;
    PlaybookConfig config;

    // Snapshots of context
    CoDesignerSessionState designer_session;

    // Optional analysis outputs (for block-level playbooks)
    BehaviorDescriptor initial_behavior;
    BehaviorDescriptor final_behavior;

    IrModule initial_ir;
    IrModule final_ir;

    // Optimization summary (for block-level playbooks)
    IrOptimizationResult optimization; // may be empty/default if no optimization

    // Refactor plans and applied changes (for block-level playbooks)
    Vector<TransformationPlan> proposed_plans;
    Vector<String> applied_plan_ids;
    int new_circuit_revision = -1; // -1 if no changes applied

    // Diffs against baseline (for block-level playbooks)
    BehaviorDiff behavior_diff;
    IrDiff ir_diff;

    // Codegen result for final state (for block-level playbooks)
    CodegenModule codegen;

    // System-level results (for system-level playbooks)
    Vector<BlockPlaybookResult> system_block_results;

    // Aggregated system-level metrics
    int total_blocks = 0;
    int blocks_with_changes = 0;
    int total_applied_plans = 0;
};
```

## 4. Playbook Engine

The `PlaybookEngine` orchestrates the flow of operations for each playbook.

### 4.1 Execution Flow (OptimizeBlockAndReport)

For `PlaybookKind::OptimizeBlockAndReport`:

1. Lookup the `CoDesignerSessionState` from `CoDesignerManager` using `designer_session_id`.
   - Ensure `current_block_id` or `config.block_id` is set; if not, error.

2. Set or confirm focus:
   - If `config.block_id` is non-empty and differs from `current_block_id`, update focus (equivalent to `designer-set-focus`).

3. Initial analysis:
   - Use existing helpers to get:
     - `initial_behavior` for the block.
     - `initial_ir` (via `BuildIrForBlockInBranch` or via `designer-analyze` equivalent logic).

4. IR optimization:
   - Run `IrOptimizer::OptimizeModule` with `config.passes` on `initial_ir`.
   - Store `optimization` and `final_ir` in `PlaybookResult`.

5. Refactor proposal (no application in this playbook):
   - Call `ProposeIrBasedTransformationsForBlock` (via CircuitFacade/CoDesigner).
   - Store `proposed_plans`.

6. Diff vs baseline:
   - Use `DiffBlockBehaviorBetweenBranches` and `DiffBlockIrBetweenBranches` comparing:
     - current branch (after hypothetical changes? or current head) vs `config.baseline_branch`.
   - For this playbook, do not apply any transformations; diffs may show "zero change" if baseline == current branch.

7. Codegen:
   - Use `GenerateCodeForBlockInBranch` to get `codegen` for the block.
   - Honor `config.use_optimized_ir` if possible (e.g., codegen from `final_ir` if it reflects a safe, non-applied optimization view).

8. Assemble `PlaybookResult`:
   - Fill in all fields above.
   - `applied_plan_ids` remains empty.
   - `new_circuit_revision` remains -1.

### 4.2 Execution Flow (OptimizeAndApplySafeRefactors)

For `PlaybookKind::OptimizeAndApplySafeRefactors`:

1. Steps 1–5 as above.

2. Behavior/IR verification & application:
   - For each proposed `TransformationPlan`:
     - Reuse existing verification logic (from Phase 12/15).
     - If the plan is behavior-preserving and `config.apply_refactors` is true:
       - Apply using `ApplyTransformationPlan` on the underlying ProtoVM session/branch.
       - Record `applied_plan_ids`.
   - Refresh block state after application:
     - Update `final_behavior`.
     - Update `final_ir` (rebuild IR for the block).
     - Store updated `designer_session` and `new_circuit_revision`.

3. Diff vs baseline:
   - Same as above, but now you expect potentially non-trivial diffs vs `config.baseline_branch`.

4. Codegen:
   - Generate code for the **final** block state (after applied refactors).
   - Use optimized IR if `config.use_optimized_ir` or if it matches `final_ir`.

### 4.3 Execution Flow (SystemOptimizeAndReport)

For `PlaybookKind::SystemOptimizeAndReport`:

1. Validate configuration:
   - Ensure `target` is "system"
   - Either `block_ids` or `name_prefix` must be specified

2. Resolve block set:
   - If `block_ids` non-empty, use it.
   - Otherwise, use `name_prefix` to query all blocks whose IDs/names start with that prefix:
     - Use CircuitFacade / BlockAnalysis to enumerate blocks.

3. For each block in the resolved set:
   - Run the equivalent of `OptimizeBlockAndReport`:
     - Analyze initial behavior + IR.
     - Run IR optimization (no refactor apply).
     - Propose refactors (do not apply).
     - Optionally compute diffs vs `baseline_branch` (these may be no-op if no structural difference vs baseline).
     - Generate codegen for the block (current or optimized view; honoring `use_optimized_ir`).

4. Populate a `BlockPlaybookResult` instance for each block.

5. Aggregation:
   - `total_blocks`: number of blocks in the resolved set.
   - `blocks_with_changes`: number of blocks where:
     - optimization summaries report non-zero changes, or
     - diffs indicate changes vs baseline.
   - `total_applied_plans` = 0 (no apply in this playbook).
   - `system_block_results` = vector of all block results.

### 4.4 Execution Flow (SystemOptimizeAndApplySafeRefactors)

For `PlaybookKind::SystemOptimizeAndApplySafeRefactors`:

1. Validate configuration:
   - Ensure `target` is "system"
   - Either `block_ids` or `name_prefix` must be specified

2. Resolve block set (as above).

3. For each block:
   - Analyze initial behavior + IR.
   - Run IR optimization.
   - Propose refactors.
   - Use existing verification logic to decide which plans are safe (behavior-preserving):
     - If `apply_refactors` is true:
       - Apply safe plans for that block.
       - Track `applied_plan_ids` and per-block `new_circuit_revision` (the revision after last applied plan for that block).
     - Else:
       - Do not apply; treat as "dry run".
   - After possible application:
     - Recompute `final_behavior` and `final_ir`.
     - Compute diffs vs `baseline_branch`.
     - Generate codegen for the **final** state of that block.

4. Populate a `BlockPlaybookResult` instance for each block.

5. Aggregation:
   - `total_blocks`: number of blocks in the resolved set.
   - `blocks_with_changes`: blocks where either:
     - `applied_plan_ids` non-empty
     - or diffs vs baseline are non-empty.
   - `total_applied_plans`: sum of `applied_plan_ids.size()` across all blocks.

## 5. System-Level Block Selection

System-level playbooks support two modes of block selection:

1. **Explicit list** (`block_ids`): Specify an exact list of block IDs to operate on
2. **Name prefix match** (`name_prefix`): Find all blocks whose names start with a given prefix

When both are provided, `block_ids` takes precedence. When neither is specified for a system-level playbook, an error is returned.

## 6. Daemon Commands

### 6.1 designer-run-playbook

Execute a playbook workflow with the specified parameters:

**Request JSON (payload) for block-level playbooks**:
```json
{
  "designer_session_id": "cd-1234",
  "playbook_kind": "OptimizeAndApplySafeRefactors",
  "target": "block",
  "block_id": "B3",
  "baseline_branch": "main",
  "passes": ["SimplifyAlgebraic", "FoldConstants"],
  "use_optimized_ir": true,
  "apply_refactors": true
}
```

**Request JSON (payload) for system-level playbooks**:
```json
{
  "designer_session_id": "cd-1234",
  "playbook_kind": "SystemOptimizeAndApplySafeRefactors",
  "target": "system",
  "block_ids": ["ALU_ADD", "ALU_MUX"],
  "name_prefix": "ALU_",  // optional prefix filter (if block_ids not provided)
  "baseline_branch": "main",
  "passes": ["SimplifyAlgebraic", "FoldConstants"],
  "use_optimized_ir": true,
  "apply_refactors": true
}
```

**Response for system-level playbooks**:
```json
{
  "ok": true,
  "command": "designer-run-playbook",
  "error_code": null,
  "error": null,
  "data": {
    "playbook_result": {
      "kind": "SystemOptimizeAndApplySafeRefactors",
      "config": { ... },
      "designer_session": { ... },
      "initial_behavior": { ... }, // may be unused for system-level
      "final_behavior": { ... },   // may be unused for system-level
      "initial_ir": { ... },       // may be unused for system-level
      "final_ir": { ... },         // may be unused for system-level
      "optimization": { ... IrOptimizationResult ... }, // may be unused for system-level
      "proposed_plans": [ ... ],   // may be unused for system-level
      "applied_plan_ids": [ ... ], // may be unused for system-level
      "new_circuit_revision": -1,  // -1 for system-level since it's an aggregate
      "behavior_diff": { ... },    // may be unused for system-level
      "ir_diff": { ... },          // may be unused for system-level
      "codegen": { ... },          // may be unused for system-level
      "system_block_results": [
        {
          "block_id": "ALU_ADD",
          "initial_behavior": { ... },
          "final_behavior": { ... },
          "initial_ir": { ... },
          "final_ir": { ... },
          "optimization": { ... },
          "proposed_plans": [ ... ],
          "applied_plan_ids": ["IR_T_ADD_1"],
          "new_circuit_revision": 43,
          "behavior_diff": { ... },
          "ir_diff": { ... },
          "codegen": { ... }
        },
        {
          "block_id": "ALU_MUX",
          "initial_behavior": { ... },
          "final_behavior": { ... },
          "initial_ir": { ... },
          "final_ir": { ... },
          "optimization": { ... },
          "proposed_plans": [ ... ],
          "applied_plan_ids": [],
          "codegen": { ... }
        }
      ],
      "total_blocks": 2,
      "blocks_with_changes": 1,
      "total_applied_plans": 1
    }
  }
}
```

### 5.2 designer-list-playbooks (Optional)

List available playbooks and their descriptions (not implemented in this phase).

## 6. Integration Points

The playbook system integrates with:

- **CoDesignerManager**: Orchestration of existing CoDesigner sessions
- **SessionStore**: Storage management for state persistence
- **CircuitFacade**: Core analysis, optimization, transformation, diff, and codegen capabilities
- **JsonIO**: Serialization of playbook types for daemon communication
- **SessionServer**: Daemon command handling

## 7. Safety & Verification

### 7.1 Behavior Preservation

Playbooks implement conservative behavior preservation checks:

- Use heuristic-based verification for transformation plans
- Only apply transformations marked as behavior-preserving
- Provide detailed diff analysis to show changes before/after

### 7.2 Branch Isolation

- All operations are branch-aware and isolated
- Changes only affect the specified branch
- Baseline comparisons use separate reference branches

## 8. Extensibility

The playbook system is designed for extension:

- New `PlaybookKind` values can be added
- Additional configuration parameters can be introduced
- New execution flows can be implemented in `PlaybookEngine::RunPlaybook`

## 9. Examples

### 9.1 Optimize a Block Without Applying Changes

```json
{
  "id": "req1",
  "command": "designer-run-playbook",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "playbook_kind": "OptimizeBlockAndReport",
    "target": "block",
    "block_id": "B5",
    "baseline_branch": "main",
    "passes": ["SimplifyAlgebraic", "FoldConstants"],
    "use_optimized_ir": true,
    "apply_refactors": false
  }
}
```

### 9.2 Optimize and Apply Safe Refactors

```json
{
  "id": "req2",
  "command": "designer-run-playbook",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-5678",
    "playbook_kind": "OptimizeAndApplySafeRefactors",
    "target": "block",
    "block_id": "B7",
    "baseline_branch": "main",
    "passes": ["SimplifyAlgebraic", "SimplifyMux"],
    "use_optimized_ir": true,
    "apply_refactors": true
  }
}
```