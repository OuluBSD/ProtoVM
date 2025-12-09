# ProtoVM Timing & Hazard Analysis Engine

## 1. Overview

This document describes the timing and hazard analysis engine implemented in ProtoVM. The system provides static timing analysis capabilities on top of the existing circuit graph model, enabling critical path analysis, cycle detection, and hazard identification.

## 2. Timing Model

### 2.1 Definitions

- **TimingNode**: A node in timing analysis, corresponding to component pins in the circuit graph
- **TimingEdge**: A directed edge representing signal propagation between pins through components
- **Path Depth**: Integer representing the number of logic levels (or edges) from a source to a sink
- **Critical Path**: The longest combinational path in terms of logic levels between sources and sinks

### 2.2 Timing Model Scope

The timing model focuses on:
- **Combinational depth computation** (logical levels, not physical delays)
- **Critical path estimation** for performance analysis
- **Cycle detection** for identifying combinational loops
- **Hazard detection** for identifying potential structural hazards

**Note**: This is a logical-level timing model, not a physical one. It does not model absolute time (ns) or electrical characteristics, but focuses on logical depth and structural analysis.

## 3. Data Structures

### 3.1 TimingNodeId
```cpp
struct TimingNodeId {
    std::string id;     // e.g. "C1:OUT", "C2:IN"
};
```

### 3.2 TimingPath
```cpp
struct TimingPathPoint {
    TimingNodeId node;
    int depth;  // cumulative depth at this node
};

struct TimingPath {
    std::vector<TimingPathPoint> points;  // ordered from source to sink
    int total_depth;                      // depth at sink
};
```

### 3.3 TimingSummary
```cpp
struct TimingSummary {
    int max_depth;
    int path_count;
};
```

### 3.4 HazardCandidate
```cpp
struct HazardCandidate {
    std::vector<TimingNodeId> reconvergent_points;  // where signals reconverge
    std::vector<TimingNodeId> sources;              // upstream sources
    std::string description;                        // human-readable description
};
```

## 4. CLI Commands

All timing commands accept the following common parameters:
- `--workspace` (required)
- `--session-id` (required)
- `--branch` (optional, defaults to current branch)

### 4.1 `timing-summary`

Computes global timing summary for the current branch revision.

**CLI Usage:**
```
proto-vm-cli timing-summary --workspace <path> --session-id <id> [--branch <name>]
```

**Daemon Usage:**
```json
{
  "id": "req-1",
  "command": "timing-summary",
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
  "command": "timing-summary",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "max_depth": 23,
    "path_count": 157
  }
}
```

### 4.2 `timing-critical-paths`

Returns the top N deepest timing paths.

**CLI Usage:**
```
proto-vm-cli timing-critical-paths --workspace <path> --session-id <id> [--branch <name>] [--max-paths N] [--max-depth M]
```

**Daemon Usage:**
```json
{
  "id": "req-2",
  "command": "timing-critical-paths",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main",
    "max_paths": 5,
    "max_depth": 1024
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "timing-critical-paths",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "paths": [
      {
        "total_depth": 23,
        "points": [
          { "node": { "id": "C1:OUT" }, "depth": 0 },
          { "node": { "id": "C2:IN" }, "depth": 1 },
          { "node": { "id": "C2:OUT" }, "depth": 2 },
          { "node": { "id": "C3:IN" }, "depth": 3 }
        ]
      }
    ]
  }
}
```

### 4.3 `timing-loops`

Reports combinational loops, if any.

**CLI Usage:**
```
proto-vm-cli timing-loops --workspace <path> --session-id <id> [--branch <name>]
```

**Daemon Usage:**
```json
{
  "id": "req-3",
  "command": "timing-loops",
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
  "command": "timing-loops",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "loops": [
      {
        "nodes": [
          { "id": "C10:OUT" },
          { "id": "C11:IN" },
          { "id": "C11:OUT" },
          { "id": "C10:IN" }
        ]
      }
    ]
  }
}
```

If no loops exist, returns an empty array.

### 4.4 `timing-hazards`

Reports reconvergent fanout hazard candidates.

**CLI Usage:**
```
proto-vm-cli timing-hazards --workspace <path> --session-id <id> [--branch <name>] [--max-results N]
```

**Daemon Usage:**
```json
{
  "id": "req-4",
  "command": "timing-hazards",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "user",
  "payload": {
    "branch": "main",
    "max_results": 64
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "timing-hazards",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "hazards": [
      {
        "description": "Potential glitch due to reconvergent fanout from C5:OUT to C9:IN",
        "sources": [
          { "id": "C5:OUT" }
        ],
        "reconvergent_points": [
          { "id": "C9:IN" }
        ]
      }
    ]
  }
}
```

These are **heuristic warnings**, not hard errors, identifying potential hazard hotspots.

## 5. Implementation Details

### 5.1 Timing Graph Construction

The timing analysis operates on a timing-specific graph derived from the circuit graph. The `TimingGraphBuilder`:

1. Filters the circuit graph to focus on `SignalFlow` edges (representing actual signal propagation)
2. Creates timing nodes for component pins only
3. Creates timing edges representing signal flow between pins

### 5.2 Critical Path Algorithm

The critical path algorithm uses BFS/DFS traversal to find the longest paths between sources and sinks:

- Sources are nodes with no incoming signal flow edges
- Sinks are nodes with no outgoing signal flow edges
- Path depth is incremented for each logic level (edge in the signal flow graph)

### 5.3 Cycle Detection

Cycle detection uses DFS to identify combinational loops in the signal flow graph. This helps identify potentially problematic feedback in combinational circuits.

### 5.4 Hazard Detection

Hazard detection focuses on identifying potential reconvergent fanout patterns:

- A reconvergent fanout occurs when a signal splits and later reconverges at a later point
- This can create timing hazards where signal changes may cause glitches
- The implementation uses heuristics to identify common hazard-prone patterns

## 6. Integration Points

### 6.1 Branch Awareness

All timing commands are branch-aware and operate on the specified branch (or current branch if not specified). The system maintains separate timing analysis results per branch.

### 6.2 Read-Only Operations

All timing commands are read-only and do not modify session state, circuit revisions, or simulation state.

## 7. Performance Considerations

- Timing analysis is performed on-demand and does not affect simulation performance
- Maximum depth and result limits prevent excessive computation
- Graph construction happens once per analysis operation

## 8. Scope & Limitations

### 8.1 Scope
- Logic-level timing analysis (combinational depth)
- Critical path identification
- Cycle detection
- Basic hazard heuristics
- Branch-aware analysis

### 8.2 Limitations
- **Not a physical timing model**: No ns delays, slew/transition/RC modeling
- **No clock-domain analysis**: Single timing domain only
- **Heuristic-based hazards**: May report false positives
- **Logical-level only**: No electrical or physical modeling

## 9. Use Cases

The timing engine enables several analysis capabilities:

- **Performance Analysis**: Identify critical paths for optimization
- **Design Validation**: Detect combinational loops in intended sequential designs
- **Hazard Identification**: Find potential glitch-prone regions
- **Timing Optimization**: Guide manual or automated design improvements
- **AI Tooling**: Provide structured timing data for automated analysis