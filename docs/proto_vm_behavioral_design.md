# ProtoVM Behavioral Inference Engine

## 1. Overview

This document describes the behavioral inference engine implemented in ProtoVM. The system provides high-level behavioral analysis and semantic understanding of digital circuits by inferring what specific subcircuits do based on their structure and connectivity patterns.

The behavioral inference engine operates on top of existing analysis infrastructure:
- **CircuitGraph** (Phase 7): Structural representation of circuits
- **TimingAnalysis** (Phase 8): Timing and depth analysis
- **FunctionalAnalysis** (Phase 9): Dependency analysis
- **BlockAnalysis** (Phase 10): Semantic subcircuit detection

The system infers high-level behavioral descriptions like "4-bit ripple-carry adder", "2:1 multiplexer", "N-bit equality comparator" and provides structured, machine-readable behavior descriptors.

## 2. Behavioral Model

### 2.1 BehaviorKind

The `BehaviorKind` enumeration represents coarse semantic classifications of behavior:

- `Unknown`: No specific behavior recognized
- `CombinationalLogic`: Generic combinational logic
- `Adder`: Addition logic (half/full adders, multi-bit adders)
- `Subtractor`: Subtraction logic
- `Comparator`: Magnitude comparison logic
- `EqualityComparator`: Equality detection logic
- `InequalityComparator`: Inequality detection logic
- `Mux`: Multiplexer structures
- `Decoder`: Binary to one-hot decoders
- `Encoder`: One-hot to binary encoders
- `Register`: Storage elements with clock
- `Counter`: Counting logic
- `StateMachine`: State machine logic

### 2.2 BehaviorPortRole

The `BehaviorPortRole` struct provides semantic roles for ports in behavioral context:

- `port_name`: Logical name (e.g. "A", "B", "SEL", "CLK", "RESET")
- `role`: Semantic role (e.g. "data_in", "data_out", "select", "clock", "reset", "carry_in", "carry_out")

### 2.3 BehaviorDescriptor

The `BehaviorDescriptor` struct provides a structured, machine-readable summary of what a block/subcircuit does:

- `subject_id`: ID of the analyzed entity (block ID, node ID, etc.)
- `subject_kind`: Type of entity ("Block", "Pin", "Component", "Net")
- `behavior_kind`: Inferred behavior kind
- `ports`: Array of port roles
- `bit_width`: Inferred bit width if applicable
- `description`: Human-readable description

## 3. Behavioral Inference Strategy

### 3.1 Block-based Inference

The primary inference strategy operates on `BlockInstance` objects from BlockAnalysis:

1. **BlockKind Mapping**: Use the `BlockKind` from BlockAnalysis as a strong hint for behavioral classification
   - `BlockKind::Adder` → `BehaviorKind::Adder`
   - `BlockKind::Mux` → `BehaviorKind::Mux`
   - `BlockKind::Register` or `Counter` → `BehaviorKind::Register`/`Counter`
   - `BlockKind::GenericComb` → `BehaviorKind::CombinationalLogic`

2. **Port Role Analysis**: Examine port names and connectivity patterns to determine semantic roles
   - Recognize common naming patterns: "A", "B" → data inputs, "CLK" → clock, "SEL" → select
   - Infer roles like "carry_in", "carry_out", "enable", "reset" from structure

3. **Width Inference**: Determine bit width from port definitions
   - Count number of pins in multi-bit ports
   - Use largest port width as the block's bit width

4. **Description Generation**: Create human-readable descriptions combining kind, width, and roles

### 3.2 Node-based Inference

For individual nodes (pins/components/nets):

1. **Block Mapping**: If node belongs to a known block, inherit block behavior
2. **Cone Analysis**: Use functional analysis cones to determine if node is part of known patterns
3. **Fallback**: Return "Unknown" with generic descriptions

## 4. Command Interface

### 4.1 `behavior-block`

Infers behavior for a specific block:

**CLI Usage:**
```
proto-vm-cli behavior-block --workspace <path> --session-id <id> --block-id <block_id> [--branch <name>]
```

**Daemon Request:**
```json
{
  "id": "req-1",
  "command": "behavior-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main",
    "block_id": "B3"
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "behavior-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "behavior": {
      "subject_id": "B3",
      "subject_kind": "Block",
      "behavior_kind": "Adder",
      "bit_width": 4,
      "ports": [
        { "port_name": "A", "role": "data_in" },
        { "port_name": "B", "role": "data_in" },
        { "port_name": "SUM", "role": "data_out" },
        { "port_name": "CIN", "role": "carry_in" },
        { "port_name": "COUT", "role": "carry_out" }
      ],
      "description": "4-bit ripple-carry adder with carry in/out"
    }
  }
}
```

### 4.2 `behavior-node`

Infers behavior for an arbitrary node:

**CLI Usage:**
```
proto-vm-cli behavior-node --workspace <path> --session-id <id> --node-id <node_id> [--node-kind <Pin|Component|Net>] [--branch <name>]
```

**Daemon Request:**
```json
{
  "id": "req-2",
  "command": "behavior-node",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main",
    "node_id": "C10:OUT",
    "node_kind": "Pin"
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "behavior-node",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "behavior": {
      "subject_id": "C10:OUT",
      "subject_kind": "Pin",
      "behavior_kind": "Mux",
      "bit_width": 1,
      "ports": [
        { "port_name": "IN0", "role": "data_in" },
        { "port_name": "IN1", "role": "data_in" },
        { "port_name": "OUT", "role": "data_out" },
        { "port_name": "SEL", "role": "select" }
      ],
      "description": "Output of a 2:1 multiplexer"
    }
  }
}
```

## 5. Implementation Details

### 5.1 BehavioralAnalysis Class

The core behavioral analysis is implemented in the `BehavioralAnalysis` class which provides:

- `InferBehaviorForBlock()`: Infers behavior for a single block instance
- `InferBehaviorForNode()`: Infers behavior for an arbitrary node

### 5.2 Branch Awareness

All behavioral commands are branch-aware and operate on the specified branch (or current branch if not specified). The system maintains separate behavioral analysis results per branch.

### 5.3 Read-Only Operations

All behavioral commands are read-only and do not modify session state, circuit revisions, or simulation state.

## 6. Integration Points

The behavioral analysis integrates with existing infrastructure:

- **CircuitFacade**: Provides branch-aware helpers to run behavioral inference
- **CommandDispatcher**: Handles CLI command execution
- **SessionServer**: Handles daemon protocol requests
- **JsonIO**: Handles serialization of behavioral analysis structures
- **BlockAnalysis**: Provides semantic blocks as input to behavioral inference
- **FunctionalAnalysis**: Provides cone analysis for node-based inference

## 7. Scope & Limitations

### 7.1 Scope
- High-level semantic recognition of common digital logic patterns
- Structured, machine-readable behavioral descriptors
- Human-readable behavioral descriptions
- Branch-aware behavioral analysis
- Read-only, deterministic analysis

### 7.2 Limitations
- **Heuristic-based**: Pattern matching is based on structural analysis, not formal verification
- **Pattern-limited**: Only recognizes known patterns from BlockAnalysis
- **No formal equivalence**: Does not perform complete functional equivalence checking vs ideal models
- **No SAT/SMT**: Purely structural/semantic inference without external solvers

## 8. Use Cases

The behavioral analysis engine enables several capabilities:

- **Circuit Understanding**: Identify high-level functional units within complex circuits
- **Design Review**: Verify expected high-level structures exist
- **Documentation**: Generate behavioral circuit documentation
- **Verification**: Check for specific behavioral combinations
- **Optimization**: Guide manual or automated behavioral-level optimization
- **AI Tooling**: Provide structured semantic information for automated analysis
- **Debugging**: Understand what parts of a circuit do at a behavioral level