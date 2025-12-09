# ProtoVM HLS-lite IR Design

## Overview

ProtoVM provides a high-level, HLS-like intermediate representation (IR) for circuit analysis. This IR serves as a bridge between low-level circuit structures and higher-level, code-like reasoning tools. The IR is designed to capture structured, dataflow/RTL-style descriptions of circuit behavior.

## IR Model

### Core IR Structures

The HLS-lite IR consists of the following core data structures:

#### IrValue
Represents a value reference in the IR (wire, port, literal).
```cpp
struct IrValue {
    std::string name;        // symbolic name (e.g. "A", "B", "SUM", "tmp1")
    int bit_width;           // -1 or 0 if unknown
    bool is_literal;         // true if this is a constant
    uint64_t literal;        // only valid if is_literal == true (for small constants)
};
```

#### IrExprKind
Enumeration of expression kinds:
- `Value` - direct reference
- `Not` - logical inversion
- `And`, `Or`, `Xor` - logical operations
- `Add`, `Sub` - arithmetic operations
- `Mux` - ternary select (sel ? a : b)
- `Eq`, `Neq` - equality comparisons

#### IrExpr
Represents an expression with a target and arguments:
```cpp
struct IrExpr {
    IrExprKind kind;                // type of expression
    IrValue target;                 // left-hand side (e.g. SUM)
    std::vector<IrValue> args;      // right-hand side operands (0,1,2,3 as needed)
};
```

#### IrRegAssign
Represents a simple sequential assignment:
```cpp
struct IrRegAssign {
    IrValue target;                 // register output
    IrExpr  expr;                   // next-state expression
    std::string clock;              // clock signal name (if known)
    std::::string reset;            // reset signal name (optional)
};
```

#### IrModule
Top-level IR for a block/subcircuit:
```cpp
struct IrModule {
    std::string id;                             // e.g. block ID or region ID
    std::vector<IrValue> inputs;                // input ports
    std::vector<IrValue> outputs;               // output ports
    std::vector<IrExpr>  comb_assigns;          // combinational equations
    std::vector<IrRegAssign> reg_assigns;       // sequential updates
};
```

## IR Inference

### Block-level Inference

The IR inference engine operates primarily on `BlockInstance` and their behavior descriptors to generate structured IR:

- For `BehaviorKind::Adder`: identifies operand ports (A, B, CIN) and SUM/COUT outputs, emitting `SUM = A + B (+ CIN)` expressions
- For `BehaviorKind::Mux`: identifies IN0, IN1, SEL, OUT ports, emitting `OUT = Mux(SEL, IN1, IN0)` expressions
- For `BehaviorKind::Comparator`: identifies A, B, EQ ports, emitting `EQ = (A == B)` expressions
- For sequential blocks (Register/Counter): derives simple assignments like `Q_next = D` or `Q_next = Q + 1`

### Node-region Inference

For arbitrary nodes, the system can infer IR for small local regions using functional analysis cones to define input/output boundaries.

## Commands

### CLI Commands

#### `ir-block`
Generate HLS IR for a specific circuit block.

**Usage:**
```
proto-vm-cli ir-block --workspace PATH --session-id ID --block-id BLOCK_ID [--branch BRANCH_NAME]
```

**Output:**
Returns an `IrModule` JSON representation containing inputs, outputs, combinational assignments, and sequential assignments for the block.

#### `ir-node-region`
Generate HLS IR for a small region around a specific node.

**Usage:**
```
proto-vm-cli ir-node-region --workspace PATH --session-id ID --node-id NODE_ID [--node-kind KIND] [--max-depth N] [--branch BRANCH_NAME]
```

**Output:**
Returns an `IrModule` JSON representation for the local region around the specified node.

### Daemon Commands

The daemon supports the same functionality via JSON-RPC:

#### `"ir-block"` command
**Request:**
```json
{
  "id": "1",
  "command": "ir-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "block_id": "B3",
    "branch": "main"
  }
}
```

**Response:**
```json
{
  "id": "1",
  "ok": true,
  "command": "ir-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "ir": {
      "id": "B3",
      "inputs": [
        { "name": "A", "bit_width": 4, "is_literal": false, "literal": null },
        { "name": "B", "bit_width": 4, "is_literal": false, "literal": null },
        { "name": "CIN", "bit_width": 1, "is_literal": false, "literal": null }
      ],
      "outputs": [
        { "name": "SUM", "bit_width": 4, "is_literal": false, "literal": null },
        { "name": "COUT", "bit_width": 1, "is_literal": false, "literal": null }
      ],
      "comb_assigns": [
        {
          "kind": "Add",
          "target": { "name": "SUM", "bit_width": 4, "is_literal": false, "literal": null },
          "args": [
            { "name": "A", "bit_width": 4, "is_literal": false, "literal": null },
            { "name": "B", "bit_width": 4, "is_literal": false, "literal": null }
          ]
        }
      ],
      "reg_assigns": []
    }
  }
}
```

#### `"ir-node-region"` command
Similar to ir-block but operates on a local region around a specified node.

## Branch-aware Operation

All IR commands operate in a branch-aware manner:
- Accept an optional branch name (defaults to current branch)
- Generate IR based on the circuit state in that specific branch
- Results are deterministic and reflect the exact circuit state at the time of query

## Limitations

- IR is approximate for highly complex/generic combinational regions
- Not a full HLS language; it is an analysis/exchange representation
- No heavy symbolic simplification beyond what's reasonable from existing graphs/blocks
- IR-level optimization is not performed in this phase

## Use Cases

1. **Circuit Behavior Explanation**: Generate human-readable descriptions of circuit blocks
2. **Transformation Planning**: Feed IR into reasoning tools for circuit optimizations
3. **Verification**: Compare expected vs. actual circuit behavior using structured IR
4. **AI-assisted Design**: Provide structured representations for AI tools to understand and modify circuits