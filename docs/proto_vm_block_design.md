# ProtoVM Block Analysis & Semantic Subcircuit Discovery

## 1. Overview

This document describes the block analysis and semantic subcircuit discovery system implemented in ProtoVM. The system provides structural analysis of digital circuits to identify meaningful subcircuits (e.g., adders, multiplexers, decoders, registers) within low-level circuit descriptions.

## 2. Block Model & Semantics

### 2.1 Definitions

- **Block**: A semantically meaningful subcircuit composed of one or more components and nets. Has well-defined inputs, outputs, and optionally internal state.

- **BlockInstance**: A specific occurrence of a Block in a given circuit/branch. Has a unique ID, a block kind (e.g., "Adder", "Mux", "Decoder", "GenericComb", etc.), a set of member components and nets, and a defined interface (input pins, output pins).

### 2.2 BlockKind Enumeration

- **GenericComb**: Generic combinational cluster
- **Adder**: Addition logic (half/full adders, multi-bit adders)
- **Comparator**: Equality or magnitude comparison logic
- **Mux**: Multiplexer structures
- **Decoder**: Binary to one-hot decoders
- **Encoder**: One-hot to binary encoders
- **Register**: Storage elements with clock
- **Counter**: Counting logic
- **Latch**: Level-sensitive storage elements

### 2.3 Non-goals

Block detection is heuristic and structural, not a full behavioral inference engine. It identifies common patterns based on component types and connectivity, but does not perform formal verification that identified blocks functionally match ideal models.

## 3. Block-level Commands

### 3.1 `blocks-list`

Lists all discovered blocks in a branch.

**CLI Usage:**
```
proto-vm-cli blocks-list --workspace <path> --session-id <id> [--branch <name>]
```

**Daemon Usage:**
```json
{
  "id": "req-1",
  "command": "blocks-list",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main"
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "blocks-list",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "blocks": [
      {
        "id": "B1",
        "kind": "Adder",
        "components": ["C10", "C11", "C12"],
        "ports": [
          { "name": "A", "direction": "in", "pins": ["C10:A", "C11:A"] },
          { "name": "B", "direction": "in", "pins": ["C10:B", "C11:B"] },
          { "name": "SUM", "direction": "out", "pins": ["C12:SUM"] }
        ]
      }
    ]
  }
}
```

### 3.2 `blocks-export`

Exports a block-level view of the circuit.

**CLI Usage:**
```
proto-vm-cli blocks-export --workspace <path> --session-id <id> [--branch <name>]
```

**Daemon Usage:**
```json
{
  "id": "req-2",
  "command": "blocks-export",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main"
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "blocks-export",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_graph": {
      "blocks": [ ... ],
      "edges": [
        {
          "from_block": "B1",
          "to_block": "B2",
          "from_port": "SUM",
          "to_port": "IN"
        }
      ]
    }
  }
}
```

### 3.3 `block-inspect`

Inspects a specific block by ID.

**CLI Usage:**
```
proto-vm-cli block-inspect --workspace <path> --session-id <id> --block-id <id> [--branch <name>]
```

**Daemon Usage:**
```json
{
  "id": "req-3",
  "command": "block-inspect",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main",
    "block_id": "B1"
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "block-inspect",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block": {
      "id": "B1",
      "kind": "Adder",
      "components": ["C10", "C11", "C12"],
      "nets": [],
      "ports": [
        { "name": "A", "direction": "in", "pins": ["C10:A", "C11:A"] },
        { "name": "B", "direction": "in", "pins": ["C10:B", "C11:B"] },
        { "name": "SUM", "direction": "out", "pins": ["C12:SUM"] }
      ]
    }
  }
}
```

## 4. Implementation Details

### 4.1 Detection Strategy

The block detection system uses a pattern-based approach with heuristics:

1. **Generic combinational clusters (GenericComb)**: Finds connected regions of combinational components and groups them if they exceed a size threshold.

2. **Semantic patterns**: Detects specific topologies for:
   - Adders: XOR + AND combinations for sum and carry logic
   - Multiplexers: AND/OR/INV patterns around select signals
   - Comparators: XOR/XNOR chains converging to detection logic
   - Decoders: AND gate patterns with input inversions

### 4.2 Block Boundaries

Block interfaces are determined by:
- **Inputs**: Pins whose drivers are outside the block
- **Outputs**: Pins that drive nodes outside the block
- **Internal**: Pins connecting components entirely within the block

### 4.3 Branch Awareness

All block commands are branch-aware and operate on the specified branch (or current branch if not specified). The system maintains separate block analysis results per branch.

### 4.4 Read-Only Operations

All block commands are read-only and do not modify session state, circuit revisions, or simulation state.

## 5. Relation to Existing Layers

### 5.1 Dependencies

Block detection uses:
- **CircuitGraph**: Provides structural topology information
- **CircuitData**: Provides component type information for pattern matching

### 5.2 Distinction

- **CircuitGraph**: Low-level structural representation (components, pins, nets)
- **Functional Analysis**: Dependency relationships between nodes
- **Timing Analysis**: Depth and timing properties
- **Block Analysis**: Semantic clustering of low-level components into meaningful units

## 6. Use Cases

The block analysis system enables several capabilities:

- **Circuit Understanding**: Identify functional units within complex circuits
- **Design Review**: Verify expected block structures exist
- **Documentation**: Generate block-level circuit documentation
- **Verification**: Check for specific block combinations
- **Optimization**: Guide manual or automated circuit optimization
- **AI Tooling**: Provide structured semantic information for automated analysis