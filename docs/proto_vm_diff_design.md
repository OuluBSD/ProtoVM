# ProtoVM Diff Engine Design

## Overview

The ProtoVM Diff Engine provides a regression analysis layer that compares behavioral information and HLS-lite IR modules across different branches and revisions. This engine enables understanding how circuit behavior changes across branches and revisions, turning ProtoVM into a regression-awareness engine.

## Core Concepts

### 1. Diff Types

The diff engine supports two main types of comparisons:

#### Behavioral Diff
- Compares two `BehaviorDescriptor` objects between different branches/revisions
- Reports changes in:
  - Behavior kind (e.g., Adder → GenericComb)
  - Bit widths
  - Port roles and changes
  - Descriptive text

#### IR Diff
- Compares two `IrModule` instances between different branches/revisions
- Reports changes in:
  - Interface (inputs/outputs)
  - Combinational logic expressions
  - Sequential register assignments

### 2. Change Classification

Changes are classified into distinct categories to allow for meaningful analysis:

#### BehaviorChangeKind
- `None`: No changes detected
- `BehaviorKindChanged`: The behavior classification changed
- `BitWidthChanged`: Bit width changed
- `PortsChanged`: Port roles or definitions changed
- `DescriptionChanged`: Descriptive text changed
- `MultipleChanges`: Multiple types of changes detected

#### IrChangeKind
- `None`: No changes detected
- `InterfaceChanged`: Input/output interface modified
- `CombLogicChanged`: Combinational expressions modified
- `RegLogicChanged`: Register assignments modified
- `MultipleChanges`: Multiple types of changes detected

## Data Structures

### PortChange
Records detailed changes for individual ports:
```cpp
struct PortChange {
    std::string port_name;
    std::string before_role;    // e.g. "data_in"
    std::string after_role;
    int before_width;          // -1 if unknown
    int after_width;
};
```

### BehaviorDiff
Encapsulates behavioral differences:
```cpp
struct BehaviorDiff {
    std::string subject_id;             // block or node id
    std::string subject_kind;           // "Block", "Pin", "Component", "Net"
    BehaviorChangeKind change_kind;
    BehaviorDescriptor before_behavior;
    BehaviorDescriptor after_behavior;
    std::vector<PortChange> port_changes; // detailed port-level changes
};
```

### IrDiff
Encapsulates IR-level differences:
```cpp
struct IrDiff {
    std::string module_id;         // id of IrModule (block id, region id)
    IrChangeKind change_kind;
    IrInterfaceChange iface_changes;
    std::vector<IrExprChange> comb_changes;
    std::vector<IrRegChange> reg_changes;
};
```

## API Commands

### 1. `behavior-diff-block`
Compares the behavioral description of a specific block between two branches.

**CLI Usage:**
```
proto-vm-cli behavior-diff-block --workspace PATH --session-id ID --branch-before BRANCH --branch-after BRANCH --block-id BLOCK_ID
```

**Daemon Request:**
```json
{
  "id": "req-1",
  "command": "behavior-diff-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "block_id": "B3"
  }
}
```

### 2. `ir-diff-block`
Compares the HLS IR representation of a specific block between two branches.

**CLI Usage:**
```
proto-vm-cli ir-diff-block --workspace PATH --session-id ID --branch-before BRANCH --branch-after BRANCH --block-id BLOCK_ID
```

**Daemon Request:**
```json
{
  "id": "req-2",
  "command": "ir-diff-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "block_id": "B3"
  }
}
```

### 3. `ir-diff-node-region` (Optional)
Compares the HLS IR representation of a node region between two branches.

**CLI Usage:**
```
proto-vm-cli ir-diff-node-region --workspace PATH --session-id ID --branch-before BRANCH --branch-after BRANCH --node-id NODE_ID --node-kind KIND --max-depth N
```

**Daemon Request:**
```json
{
  "id": "req-3",
  "command": "ir-diff-node-region",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "node_id": "C10:OUT",
    "node_kind": "Pin",
    "max_depth": 4
  }
}
```

## Implementation Details

### Branch-Aware Operations

All diff operations are branch-aware and compare the same entity across different branches. The system ensures:

- Read-only operations that don't modify circuit state
- Deterministic results for the same comparison
- Proper handling of branch metadata and revisions

### Diff Algorithms

The diff engine implements efficient comparison algorithms:

- Behavioral comparison: Structural comparison of behavior descriptors
- IR comparison: Expression-by-expression comparison with semantic understanding
- Port mapping: Intelligent matching of ports by name across versions

## Limitations

- **Heuristic-based**: Not a full formal equivalence checker (no SAT/SMT solvers)
- **Circuit-level**: Focuses on block/IR-level changes rather than bit-level waveforms
- **Current-head**: Primarily supports comparisons between current branch heads (not historical revisions)

## Use Cases

### 1. Regression Detection
Identify when changes to one branch affect the behavior of specific circuit blocks.

### 2. Refactoring Validation
Verify that behavioral refactoring transformations preserve intended functionality.

### 3. Design Evolution Tracking
Track how circuit behavior evolves across branches and development cycles.

### 4. AI Tool Integration
Provide structured diff information for AI tools to understand circuit changes.