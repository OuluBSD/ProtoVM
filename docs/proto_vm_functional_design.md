# ProtoVM Functional Dependency Analysis Design

## Overview

This document describes the functional cone analysis and automatic dependency extraction system in ProtoVM. This system provides structural analysis of signal flow dependencies in digital circuits, enabling queries about which signals/components influence or are affected by other signals/components.

## Functional Dependency Model

### Definitions

- **Functional Cone**: A set of nodes in the circuit graph that are either influenced by (forward cone) or influence (backward cone) a given root node.
- **Backward Cone (Cone of Influence)**: All upstream nodes whose value can affect the root node, following signal flow backwards.
- **Forward Cone (Cone of Impact)**: All downstream nodes whose value can be affected by the root node, following signal flow forwards.
- **Functional Dependency**: A relation `A -> B` meaning: B is (directly or transitively) a function of A's value in the combinational structure.

### Node ID Conventions

Nodes in the dependency analysis use the following conventions:
- **Pin**: `"ComponentId:PinName"` (e.g., "C1:OUT", "AND1:IN1")
- **Component**: `"ComponentId"` (e.g., "C1", "AND1")
- **Net**: `"NetId"` (e.g., "N10", "DATA_BUS_0")

### Depth Semantics

- Depth 0: The root node itself (not included in cone results, but is the starting point)
- Depth 1: Directly connected upstream/downstream nodes
- Depth N: N signal propagation steps away from the root

## Commands

### `deps-summary`
Returns upstream/downstream dependency sizes for a given node.

**CLI Usage:**
```
proto-vm-cli deps-summary --workspace <workspace> --session-id <session_id> --node-id <node_id> --node-kind <Pin|Component|Net> --max-depth <depth>
```

**Daemon Request:**
```json
{
  "id": "req1",
  "command": "deps-summary",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "node_id": "C5:OUT",
    "node_kind": "Pin",
    "max_depth": 64
  }
}
```

**Response:**
```json
{
  "id": "req1",
  "ok": true,
  "command": "deps-summary",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "root": { "id": "C5:OUT", "kind": "Pin" },
    "upstream_count": 12,
    "downstream_count": 37
  }
}
```

### `deps-backward` (or `cone-backward`)
Returns backward cone (influences) for a given node.

**CLI Usage:**
```
proto-vm-cli deps-backward --workspace <workspace> --session-id <session_id> --node-id <node_id> --node-kind <Pin|Component|Net> --max-depth <depth>
```

**Daemon Request:**
```json
{
  "id": "req1",
  "command": "deps-backward",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "node_id": "C5:OUT",
    "node_kind": "Pin",
    "max_depth": 64
  }
}
```

**Response:**
```json
{
  "id": "req1",
  "ok": true,
  "command": "deps-backward",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "cone": {
      "root": { "id": "C5:OUT", "kind": "Pin" },
      "nodes": [
        { "node": { "id": "C3:OUT", "kind": "Pin" }, "depth": 1 },
        { "node": { "id": "C1:IN", "kind": "Pin" }, "depth": 2 }
      ]
    }
  }
}
```

### `deps-forward` (or `cone-forward`)
Returns forward cone (impacts) for a given node.

**CLI Usage:**
```
proto-vm-cli deps-forward --workspace <workspace> --session-id <session_id> --node-id <node_id> --node-kind <Pin|Component|Net> --max-depth <depth>
```

**Daemon Request:**
```json
{
  "id": "req1",
  "command": "deps-forward",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "node_id": "C5:OUT",
    "node_kind": "Pin",
    "max_depth": 64
  }
}
```

**Response:**
```json
{
  "id": "req1",
  "ok": true,
  "command": "deps-forward",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "cone": {
      "root": { "id": "C5:OUT", "kind": "Pin" },
      "nodes": [
        { "node": { "id": "C7:IN", "kind": "Pin" }, "depth": 1 },
        { "node": { "id": "C8:OUT", "kind": "Pin" }, "depth": 2 }
      ]
    }
  }
}
```

### `deps-both` (optional convenience command)
Returns both forward and backward cones plus summary in a single call.

**CLI Usage:**
```
proto-vm-cli deps-both --workspace <workspace> --session-id <session_id> --node-id <node_id> --node-kind <Pin|Component|Net> --max-depth <depth>
```

**Daemon Request:**
```json
{
  "id": "req1",
  "command": "deps-both",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "node_id": "C5:OUT",
    "node_kind": "Pin",
    "max_depth": 64
  }
}
```

**Response:**
```json
{
  "id": "req1",
  "ok": true,
  "command": "deps-both",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "root": { "id": "C5:OUT", "kind": "Pin" },
    "backward": {
      "nodes": [
        { "node": { "id": "C3:OUT", "kind": "Pin" }, "depth": 1 },
        { "node": { "id": "C1:IN", "kind": "Pin" }, "depth": 2 }
      ]
    },
    "forward": {
      "nodes": [
        { "node": { "id": "C7:IN", "kind": "Pin" }, "depth": 1 },
        { "node": { "id": "C8:OUT", "kind": "Pin" }, "depth": 2 }
      ]
    },
    "summary": {
      "upstream_count": 12,
      "downstream_count": 37
    }
  }
}
```

## Relationship to Graph & Timing

- **Graph Queries** (`graph-fanin`, `graph-fanout`): Structural/topological queries that follow connectivity regardless of signal direction
- **Timing Analysis** (`timing-*`): Considers propagation delays, critical paths, and temporal properties
- **Functional Analysis** (`deps-*`): Logically oriented queries that follow signal flow direction to determine functional dependencies

## Implementation Details

The functional analysis system is built on top of the existing CircuitGraph infrastructure and follows signal flow semantics to determine dependencies. It uses depth-first search algorithms to traverse the graph both upstream (backward cone) and downstream (forward cone) from a given root node.

The system respects branch boundaries and can analyze dependencies within a specific branch of the circuit. All analysis is read-only and does not modify the circuit state.

## Error Handling

- `InvalidArgument`: When node ID or kind is invalid or node not found in circuit
- `InternalError`: When internal errors occur during analysis
- `InvalidEditOperation`: When graph node doesn't exist