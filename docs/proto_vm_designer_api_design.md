# ProtoVM CoDesigner API Design Document

## 1. Purpose & Scope

The CoDesigner API is a high-level API designed for AI-powered design assistants. It provides semantic operations and analyses to support design exploration, optimization, and transformation workflows.

## 2. API Model

The CoDesigner API is a stateful session-based API where clients:
- Create a high-level designer session linked to a ProtoVM session
- Set focus to specific blocks, components, or subcircuits for analysis
- Request semantic operations and analyses
- Receive structured responses with design insights

## 3. Session Management

### 3.1 `designer-create-session`

Create a new CoDesigner session linked to a ProtoVM session.

**Request Parameters**:
```json
{
  "command": "designer-create-session",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "branch": "main"
  }
}
```

**Response**:
```json
{
  "ok": true,
  "command": "designer-create-session",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": {
      "designer_session_id": "cds-1234",
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

### 3.2 `designer-set-focus`

Set the focus to a specific block or node for analysis.

**Request Parameters**:
```json
{
  "command": "designer-set-focus",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "block_id": "BLOCK1",
    "node_id": "NODE1",
    "node_kind": "Component",
    "use_optimized_ir": false
  }
}
```

**Response**:
```json
{
  "ok": true,
  "command": "designer-set-focus",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": {
      "designer_session_id": "cds-1234",
      "proto_session_id": 1,
      "branch": "main",
      "current_block_id": "BLOCK1",
      "current_node_id": "NODE1",
      "current_node_kind": "Component",
      "use_optimized_ir": false
    }
  }
}
```

## 4. Analysis Commands

### 4.1 `designer-get-context`

Get detailed information about the currently focused design elements.

**Request Parameters**:
```json
{
  "command": "designer-get-context",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234"
  }
}
```

**Response**:
```json
{
  "ok": true,
  "command": "designer-get-context",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "block_behavior": { ... },
    "node_behavior": { ... }
  }
}
```

### 4.2 `designer-analyze`

Perform comprehensive analysis of the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-analyze",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "include_behavior": true,
    "include_ir": true,
    "include_graph_stats": true,
    "include_timing": false
  }
}
```

### 4.3 `designer-optimize`

Apply IR-level optimizations to the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-optimize",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "passes": ["simplify-double-inversion", "simplify-redundant-gate"]
  }
}
```

## 5. Transformation Commands

### 5.1 `designer-propose-refactors`

Suggest refactorings for the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-propose-refactors",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "passes": ["simplify-double-inversion", "simplify-redundant-gate"]
  }
}
```

### 5.2 `designer-apply-refactors`

Apply a set of refactorings to the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-apply-refactors",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "plans": [...],
    "user_id": "assistant-123",
    "allow_unverified": false
  }
}
```

## 6. Change Detection and Diff

### 6.1 `designer-diff`

Compare two branches or revisions of the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-diff",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "compare_branch": "feature-branch",
    "include_behavior_diff": true,
    "include_ir_diff": true
  }
}
```

## 7. Code Generation Commands

### 7.1 `designer-codegen`

Generate code for the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-codegen",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "flavor": "PseudoVerilog",
    "use_optimized_ir": true
  }
}
```

### 7.2 `designer-codegen-block-c`

Generate C/C++ code for the focused block.

**Request Parameters**:
```json
{
  "command": "designer-codegen-block-c",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "block_id": "BLOCK1",
    "lang": "cpp",
    "emit_state_struct": true,
    "state_struct_name": "BlockState",
    "function_name": "Step"
  }
}
```

### 7.3 `designer-codegen-block-audio-demo`

Generate an audio demo for an oscillator-like block.

**Request Parameters**:
```json
{
  "command": "designer-codegen-block-audio-demo",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "block_id": "OSC1",
    "lang": "cpp",
    "state_struct_name": "OscillatorState",
    "step_function_name": "OscillatorStep",
    "render_function_name": "OscillatorRender"
  }
}
```

## 8. Retiming Commands

### 8.1 `designer-retiming`

Analyze retiming possibilities for the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-retiming",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "block_id": "BLOCK1",
    "min_depth": 2,
    "max_plans": 5
  }
}
```

### 8.2 `designer-retiming-apply`

Apply a retiming plan to the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-retiming-apply",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "plan_id": "plan-123",
    "apply_only_safe": true
  }
}
```

### 8.3 `designer-retiming-opt`

Optimize retiming for the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-retiming-opt",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "block_id": "BLOCK1",
    "objective": {
      "kind": "CriticalPathReduction",
      "target_reduction": 0.1
    },
    "apply": false
  }
}
```

## 9. Global Pipelining Commands

### 9.1 `designer-global-pipeline`

Analyze global pipelining for a subsystem.

**Request Parameters**:
```json
{
  "command": "designer-global-pipeline",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "subsystem",
    "subsystem_id": "SUB1",
    "block_ids": ["BLOCK1", "BLOCK2", "BLOCK3"],
    "analyze_only": true
  }
}
```

### 9.2 `designer-global-pipeline-opt`

Optimize global pipelining for a subsystem.

**Request Parameters**:
```json
{
  "command": "designer-global-pipeline-opt",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "subsystem",
    "subsystem_id": "SUB1",
    "block_ids": ["BLOCK1", "BLOCK2", "BLOCK3"],
    "objective": {
      "kind": "ThroughputMaximization",
      "target_ii": 1
    },
    "apply": false
  }
}
```

### 9.3 `designer-global-pipeline-apply`

Apply a global pipelining plan to a subsystem.

**Request Parameters**:
```json
{
  "command": "designer-global-pipeline-apply",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "plan_id": "gpp-123",
    "apply_only_safe": true
  }
}
```

## 10. Structural Synthesis Commands

### 10.1 `designer-struct-analyze`

Analyze structural patterns in the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-struct-analyze",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "block_id": "BLOCK1"
  }
}
```

### 10.2 `designer-struct-apply`

Apply structural changes to the focused design element.

**Request Parameters**:
```json
{
  "command": "designer-struct-apply",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "plan_id": "struct-plan-123",
    "apply_only_safe": true
  }
}
```

## 11. DSP Graph Commands

### 11.1 `designer-dsp-graph-inspect`

Inspect the DSP graph for an oscillator-like block.

**Request Parameters**:
```json
{
  "command": "designer-dsp-graph-inspect",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "block_id": "OSC1",
    "freq_hz": 440.0,
    "pan_lfo_hz": 0.25,
    "sample_rate": 48000.0,
    "duration_sec": 3.0
  }
}
```

### 11.2 `designer-dsp-render-osc`

Render audio from an oscillator-like block.

**Request Parameters**:
```json
{
  "command": "designer-dsp-render-osc",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "block_id": "OSC1",
    "freq_hz": 440.0,
    "pan_lfo_hz": 0.25,
    "sample_rate": 48000.0,
    "duration_sec": 3.0
  }
}
```

## 12. Analog Model Commands

### 12.1 `designer-analog-model-inspect`

Inspect the analog model extracted from an analog circuit block.

**Request Parameters**:
```json
{
  "command": "designer-analog-model-inspect",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "block_id": "ANALOG_OSC1"
  }
}
```

**Response**:
```json
{
  "ok": true,
  "command": "designer-analog-model-inspect",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "analog_model": {
      "id": "ANALOG_OSC1",
      "block_id": "ANALOG_OSC1",
      "kind": "RcOscillator",
      "state": [
        { "name": "v_out", "kind": "Voltage", "value": 0.0 }
      ],
      "params": [
        { "name": "R", "value": 10000.0 },
        { "name": "C", "value": 1e-7 }
      ],
      "output_state_name": "v_out",
      "estimated_freq_hz": 159.15
    }
  }
}
```

### 12.2 `designer-analog-render-osc`

Render analog oscillator output to stereo audio with panning.

**Request Parameters**:
```json
{
  "command": "designer-analog-render-osc",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "assistant-123",
  "payload": {
    "designer_session_id": "cds-1234",
    "target": "block",
    "block_id": "ANALOG_OSC1",
    "sample_rate_hz": 48000.0,
    "duration_sec": 3.0,
    "pan_lfo_hz": 0.25
  }
}
```

**Response**:
```json
{
  "ok": true,
  "command": "designer-analog-render-osc",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "left_samples": [0.0, 0.01, 0.02, ...],
    "right_samples": [0.0, 0.01, 0.02, ...],
    "render_stats": {
      "sample_rate_hz": 48000.0,
      "duration_sec": 3.0,
      "estimated_freq_hz": 159.15,
      "pan_lfo_hz": 0.25,
      "left_rms": 0.707,
      "right_rms": 0.707,
      "left_min": -0.999,
      "left_max": 0.999,
      "right_min": -0.999,
      "right_max": 0.999,
      "total_samples": 144000
    }
  }
}
```