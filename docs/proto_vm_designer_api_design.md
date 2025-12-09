# ProtoVM Co-Designer API Design Document

## 1. Purpose & Scope

The ProtoVM Co-Designer API introduces an AI-friendly design session layer that orchestrates existing ProtoVM capabilities for interactive circuit design, analysis, and optimization. This API provides a structured, session-oriented protocol for AI clients to perform tasks like:

- Circuit analysis and behavioral inference
- High-level optimization and refactoring
- Transformation plan generation and application
- Diff analysis between circuit versions
- Code generation for human-readable representations

## 2. Co-Designer Session Model

### 2.1 High-Level Concept

A CoDesignerSession wraps an existing ProtoVM session and branch, maintaining focus on a specific block or node region. The session provides:

- **Focus state**: Current block, node, or circuit region of interest
- **Analysis preferences**: IR optimization settings, etc.
- **State management**: Track AI client position in design workflow

### 2.2 CoDesignerSession State Structure

```cpp
struct CoDesignerSessionState {
    String designer_session_id;  // unique ID for this co-designer session
    int    proto_session_id;     // underlying ProtoVM session
    String branch;               // branch name
    String current_block_id;     // optional: active block focus
    String current_node_id;      // optional: active node region focus
    String current_node_kind;    // "Pin" / "Component" / "Net" / empty
    bool   use_optimized_ir;     // default false
};
```

**Lifetime**: CoDesigner sessions exist only in daemon memory during the daemon lifetime and are not persisted to disk.

## 3. Co-Designer Commands

All commands follow the standard ProtoVM CLI envelope:

```json
{
  "ok": true,
  "command": "designer-command-name",
  "error_code": null,
  "error": null,
  "data": { /* command-specific data */ }
}
```

### 3.1 designer-create-session

**Purpose**: Create a new CoDesignerSession bound to an existing ProtoVM session/branch.

**Request**:
```json
{
  "proto_session_id": 1,
  "branch": "main"
}
```

**Response**:
```json
{
  "data": {
    "designer_session": {
      "designer_session_id": "cd-1234",
      "proto_session_id": 1,
      "branch": "main",
      "current_block_id": "",
      "current_node_id": "",
      "current_node_kind": "",
      "use_optimized_ir": false
    }
  }
}
```

### 3.2 designer-set-focus

**Purpose**: Set the current focus for the CoDesignerSession.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "block_id": "B3",
  "node_id": "C10:OUT",
  "node_kind": "Pin",
  "use_optimized_ir": true
}
```

**Response**: Updated `CoDesignerSessionState`.

### 3.3 designer-get-context

**Purpose**: Retrieve current context (for AI to orient itself).

**Response**:
```json
{
  "data": {
    "designer_session": { ... },
    "block_behavior": { ... },  // if current_block_id is set
    "node_behavior": { ... }    // if current_node_id is set
  }
}
```

### 3.4 designer-analyze

**Purpose**: Provide a bundle of analysis results in a single call, based on current focus.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "include_behavior": true,
  "include_ir": true,
  "include_graph_stats": false,
  "include_timing": false
}
```

**Response**:
```json
{
  "data": {
    "designer_session": { ... },
    "block": {
      "block_id": "B3",
      "behavior": { ... BehaviorDescriptor ... },
      "ir": { ... IrModule ... }
    },
    "node": {
      "node_id": "C10:OUT",
      "behavior": { ... },
      "ir": { ... }
    }
  }
}
```

### 3.5 designer-optimize

**Purpose**: Run IR optimizations for the focused block or node-region and show results.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "target": "block",          // "block" or "node"
  "passes": ["SimplifyAlgebraic", "FoldConstants"]
}
```

**Response**:
```json
{
  "data": {
    "designer_session": { ... },
    "optimization": {
      "original": { ... IrModule ... },
      "optimized": { ... IrModule ... },
      "summaries": [ ... IrOptChangeSummary ... ]
    }
  }
}
```

### 3.6 designer-propose-refactors

**Purpose**: Propose transformation plans based on IR optimizations and known patterns.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "target": "block",
  "passes": ["SimplifyAlgebraic", "FoldConstants"]
}
```

**Response**:
```json
{
  "data": {
    "designer_session": { ... },
    "plans": [
      { "id": "IR_T1", "kind": "SimplifyDoubleInversion", ... },
      { "id": "IR_T2", "kind": "SimplifyRedundantGate", ... }
    ]
  }
}
```

### 3.7 designer-apply-refactors

**Purpose**: Apply one or more transformation plans to the underlying branch, with behavior safety.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "plans": [
    { ... full TransformationPlan object ... }
  ],
  "user_id": "ai-agent-1",
  "allow_unverified": false
}
```

**Response**:
```json
{
  "data": {
    "designer_session": { ... updated ... },
    "applied_plan_ids": ["IR_T1"],
    "new_circuit_revision": 42
  }
}
```

### 3.8 designer-diff

**Purpose**: Get before/after insight after changes, based on current focus.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "compare_branch": "main",  // or other reference branch
  "include_behavior_diff": true,
  "include_ir_diff": true
}
```

**Response**:
```json
{
  "data": {
    "designer_session": { ... },
    "behavior_diff": { ... BehaviorDiff ... },
    "ir_diff": { ... IrDiff ... }
  }
}
```

### 3.9 designer-codegen

**Purpose**: Generate human-readable code for the focus point.

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "target": "block",
  "flavor": "PseudoVerilog",
  "use_optimized_ir": true
}
```

**Response**:
```json
{
  "data": {
    "designer_session": { ... },
    "codegen": {
      "id": "B3",
      "name": "B3_Adder",
      "flavor": "PseudoVerilog",
      "code": "module B3_Adder(...);\n  ...\nendmodule\n"
    }
  }
}
```

## 4. Integration with Existing Infrastructure

The Co-Designer API is built on top of existing ProtoVM infrastructure:

- **CircuitFacade**: Provides access to circuit state and analysis operations
- **BehavioralAnalysis**: Behavioral inference for blocks and nodes
- **HlsIrInference**: IR generation for blocks and node regions
- **IrOptimization**: IR-level optimization passes and transformation plans
- **Transformations**: Refactoring operations
- **DiffAnalysis**: Comparison between circuit versions
- **Codegen**: Human-readable code generation

## 5. Example Workflows

### 5.1 Optimize and Refactor an ALU Block with AI

1. Create co-designer session for session 1 on main branch
2. Set focus to ALU block B5
3. Call analyze to get current behavior and IR
4. Call optimize with ["SimplifyAlgebraic", "FoldConstants"] passes
5. Call propose-refactors to get transformation plans
6. Call apply-refactors for selected plans
7. Call diff to see changes between before/after
8. Call codegen to get optimized Verilog representation

### 5.2 Compare Experimental Branch vs Main for a Given Block

1. Create co-designer session for session 1 on experimental branch
2. Set focus to target block B3
3. Call designer-diff with compare_branch="main"
4. Review behavior and IR differences

## 6. Limitations

- Co-designer sessions are in-memory only (not persisted beyond daemon lifetime)
- No natural language processing (this is a structured API)
- All operations are orchestrations over existing capabilities (no new functionality)
- Session IDs are UUID-like strings but not full UUIDs

## 7. Autonomous Playbook Integration (Phase 17)

The Co-Designer API now supports autonomous playbooks that orchestrate multi-step design workflows as single high-level commands. The main playbook command is:

### 7.1 designer-run-playbook

Execute a structured, multi-step design workflow in a single command:

**Request**:
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

**Response**:
```json
{
  "playbook_result": {
    "kind": "OptimizeAndApplySafeRefactors",
    "config": { ... },
    "designer_session": { ... },
    "initial_behavior": { ... },
    "final_behavior": { ... },
    "initial_ir": { ... },
    "final_ir": { ... },
    "optimization": { ... IrOptimizationResult ... },
    "proposed_plans": [ ... ],
    "applied_plan_ids": ["IR_T1", "IR_T2"],
    "new_circuit_revision": 42,
    "behavior_diff": { ... },
    "ir_diff": { ... },
    "codegen": { ... CodegenModule ... }
  }
}
```

### 7.2 designer-cdc (Optional)

The Co-Designer API optionally supports CDC analysis that orchestrates the CDC analysis commands for the focused block or subsystem:

**Request**:
```json
{
  "designer_session_id": "cd-1234",
  "target": "block",          // "block" or "system/subsystem"
  "subsystem_id": "ALU_PIPE",
  "block_ids": ["ALU_STAGE1", "ALU_STAGE2"]
}
```

**Response**:
```json
{
  "data": {
    "designer_session": { ... },
    "cdc_report": {
      "id": "ALU_PIPE",
      "clock_domains": [
        { "signal_name": "CLK_A", "domain_id": 0 },
        { "signal_name": "CLK_B", "domain_id": 1 }
      ],
      "crossings": [
        {
          "id": "CDCC_0001",
          "src": { "reg_id": "R1", "clock_signal": "CLK_A", "domain_id": 0 },
          "dst": { "reg_id": "R2", "clock_signal": "CLK_B", "domain_id": 1 },
          "kind": "MultiBitBundle",
          "is_single_bit": false,
          "bit_width": 8,
          "crosses_reset_boundary": false
        }
      ],
      "issues": [
        {
          "id": "CDCISS_0001",
          "severity": "Error",
          "summary": "Multi-bit CDC bundle from CLK_A to CLK_B.",
          "detail": "8-bit register crossing clock domains without recognized safe structure. Consider async FIFO or Gray code encoding."
        }
      ]
    }
  }
}
```

## 8. Security & Authorization

- Co-designer commands follow the same authorization model as other ProtoVM commands
- Requires valid session ID with appropriate read/write permissions
- User ID tracking follows the same pattern as other commands