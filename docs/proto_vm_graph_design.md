# ProtoVM Graph-Based Circuit Query & Semantic Analysis Engine

## 1. Overview

This document describes the graph-based semantic analysis engine implemented in ProtoVM. The system creates a graph representation of digital circuits with nodes for components, pins, and nets, enabling sophisticated structural and signal-flow queries.

## 2. Graph Model

### 2.1 Node Types

The circuit graph consists of three primary node types:

- **Component Nodes** (`GraphNodeKind::Component`)
  - Represent electronic components (gates, flip-flops, memory, etc.)
  - ID: component identifier (e.g., "C42")

- **Pin Nodes** (`GraphNodeKind::Pin`)
  - Represent component input/output pins
  - ID: component_id:pin_name format (e.g., "C42:OUT", "U5:CLK")

- **Net Nodes** (`GraphNodeKind::Net`)
  - Represent electrical connections between pins
  - ID: wire/net identifier (e.g., "N10", "W15")

### 2.2 Edge Types

The graph contains two types of edges:

- **Connectivity Edges** (`GraphEdgeKind::Connectivity`)
  - Bidirectional connections between pins and nets
  - Represent physical connectivity in the circuit
  - Example: Pin "C1:OUT" ↔ Net "N5"

- **SignalFlow Edges** (`GraphEdgeKind::SignalFlow`)
  - Directed connections from output pins to input pins
  - Represent logical signal flow direction
  - Example: Pin "C1:OUT" → Pin "C2:IN"

### 2.3 Graph Construction

The `CircuitGraphBuilder` creates the graph from `CircuitData`:

1. Creates component nodes for each circuit component
2. Creates pin nodes for each component input/output
3. Creates net nodes for each wire/connection
4. Establishes connectivity edges between pins and nets
5. Establishes signal flow edges based on pin directions (output → input)

## 3. Query Engine

The `CircuitGraphQueries` class provides several query methods:

### 3.1 Path Finding
- `FindSignalPaths()`: Find signal paths from source to target nodes
- Uses depth-limited DFS traversal following SignalFlow edges
- Returns ordered paths of nodes

### 3.2 Fan Analysis
- `FindFanIn()`: Find upstream signal sources for a given node
- `FindFanOut()`: Find downstream signal destinations for a given node
- Both use depth-limited DFS traversal in respective directions

### 3.3 Statistics
- `ComputeGraphStats()`: Count nodes and edges in the graph

## 4. CLI Commands

### 4.1 `graph-export`
Export the entire circuit graph as JSON.

**CLI Usage:**
```
proto-vm-cli graph-export --workspace <path> --session-id <id> [--branch <name>]
```

**Daemon Usage:**
```json
{
  "id": "req-1",
  "command": "graph-export",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main"
  }
}
```

### 4.2 `graph-paths`
Find signal paths from a source node to a target node.

**CLI Usage:**
```
proto-vm-cli graph-paths --workspace <path> --session-id <id> --graph-source-kind <kind> --graph-source-id <id> --graph-target-kind <kind> --graph-target-id <id> [--branch <name>] [--graph-max-depth <N>]
```

**Daemon Usage:**
```json
{
  "id": "req-2",
  "command": "graph-paths",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main",
    "source": {
      "kind": "Pin",
      "id": "C1:OUT"
    },
    "target": {
      "kind": "Pin",
      "id": "C2:IN"
    },
    "max_depth": 128
  }
}
```

### 4.3 `graph-fanin`
Find upstream signal sources for a given node.

**CLI Usage:**
```
proto-vm-cli graph-fanin --workspace <path> --session-id <id> --graph-node-kind <kind> --graph-node-id <id> [--branch <name>] [--graph-max-depth <N>]
```

**Daemon Usage:**
```json
{
  "id": "req-3",
  "command": "graph-fanin",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main",
    "node": {
      "kind": "Pin",
      "id": "C2:IN"
    },
    "max_depth": 128
  }
}
```

### 4.4 `graph-fanout`
Find downstream signal destinations for a given node.

**CLI Usage:**
```
proto-vm-cli graph-fanout --workspace <path> --session-id <id> --graph-node-kind <kind> --graph-node-id <id> [--branch <name>] [--graph-max-depth <N>]
```

**Daemon Usage:**
```json
{
  "id": "req-4",
  "command": "graph-fanout",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main",
    "node": {
      "kind": "Pin",
      "id": "C1:OUT"
    },
    "max_depth": 128
  }
}
```

### 4.5 `graph-stats`
Get statistics about the circuit graph.

**CLI Usage:**
```
proto-vm-cli graph-stats --workspace <path> --session-id <id> [--branch <name>]
```

**Daemon Usage:**
```json
{
  "id": "req-5",
  "command": "graph-stats",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main"
  }
}
```

## 5. Implementation Details

### 5.1 Data Structures

- `CircuitGraph`: Container for nodes and edges with adjacency lists
- `GraphNodeId`: Identifier for graph nodes with type and ID
- `GraphEdge`: Connection between two nodes with type
- `PathQueryResult`: Result of path finding queries
- `FanQueryResult`: Result of fan analysis queries

### 5.2 Integration Points

- **CircuitFacade**: Enhanced with `BuildGraphForBranch()` method
- **CommandDispatcher**: New command handlers for graph commands
- **SessionServer**: Daemon support for graph commands
- **Branch Support**: All graph commands are branch-aware

### 5.3 Performance Considerations

- Graph construction from circuit data is O(N + E) where N is nodes and E is edges
- Pathfinding uses DFS with depth limits to prevent runaway searches
- Adjacency lists enable O(1) neighbor lookup for traversal operations

## 6. Use Cases

The graph engine enables several powerful analysis capabilities:

- **Signal Tracing**: Find paths from signal sources to destinations
- **Fan Analysis**: Identify source/sink count for timing and loading analysis
- **Connectivity Verification**: Verify expected connections exist
- **Circuit Structure Analysis**: Understand circuit organization and hierarchy
- **Automated Testing**: Generate test vectors based on signal paths
- **Diagnostics**: Identify structural issues like floating nets or short circuits

## 7. Future Enhancements

- Advanced path analysis (critical paths, timing analysis)
- Subgraph matching for pattern detection
- Graph-based linting and design rule checking
- Visualization tools for graph display
- Performance optimizations for large circuits