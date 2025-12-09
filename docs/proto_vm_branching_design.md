# ProtoVM Branching Model Design Document

## 1. Overview

This document describes the branching and multi-timeline architecture implemented in ProtoVM. The system now supports multiple named branches per session, allowing for parallel development timelines of circuits with the ability to create, switch, merge, and delete branches.

## 2. Core Concepts

### 2.1 Branch
- A named timeline within a session
- Each branch has its own circuit revision history
- Shares initial history with other branches where possible

### 2.2 Branch Metadata
Each branch contains the following metadata:
- `name`: The branch name (e.g., "main", "feature-alu", "bugfix-timing")
- `head_revision`: Latest circuit revision on this branch
- `sim_revision`: Latest sim revision for this branch (latest simulation snapshot based on circuit state)
- `base_revision`: Revision from which this branch originally forked
- `is_default`: True for the default branch (typically "main")

### 2.3 Sessions with Branches
- Each session now contains a list of all branches
- A `current_branch` field tracks which branch is active
- Each branch maintains independent revision history

## 3. Data Model Changes

### 3.1 Session Metadata Structure
The `session.json` schema has been extended:

```json
{
  "schema_version": 1,
  "session_id": 1,
  "state": 1,
  "circuit_file": "/path/to/circuit.circuit",
  "created_at": "2025-01-01T12:34:56Z",
  "last_used_at": "2025-01-01T13:00:00Z",
  "total_ticks": 0,
  "circuit_revision": 0,  // DEPRECATED: use branches info
  "sim_revision": 0,      // DEPRECATED: use branches info  
  "current_branch": "main",
  "branches": [
    {
      "name": "main",
      "head_revision": 42,
      "sim_revision": 40,
      "base_revision": 0,
      "is_default": true
    },
    {
      "name": "experiment-alu",
      "head_revision": 10,
      "sim_revision": 10,
      "base_revision": 35,
      "is_default": false
    }
  ],
  "engine_version": "unknown"
}
```

### 3.2 Event Log Format
The event log format now includes a `branch` field:

```json
{
  "revision": 12,
  "branch": "alu-experiment", // NEW: indicates which branch this event belongs to
  "timestamp": "2025-01-01T12:34:56Z",
  "user_id": "user-1",
  "session_id": 1,
  "command": "add_component",
  "params": { ... },
  "result": { ... }
}
```

## 4. Branch Operations

### 4.1 List Branches (`branch-list`)
**CLI:** `proto-vm-cli branch-list --workspace <path> --session-id <id>`
**Daemon:** `{"command": "branch-list", "workspace": "...", "session_id": 1}`

Output:
```json
{
  "ok": true,
  "command": "branch-list",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "current_branch": "alu-experiment",
    "branches": [
      {
        "name": "main",
        "head_revision": 42,
        "sim_revision": 40,
        "base_revision": 0,
        "is_default": true
      },
      {
        "name": "alu-experiment",
        "head_revision": 10,
        "sim_revision": 10,
        "base_revision": 35,
        "is_default": false
      }
    ]
  }
}
```

### 4.2 Create Branch (`branch-create`)
**CLI:** `proto-vm-cli branch-create --workspace <path> --session-id <id> --branch-name <name> [--from-branch <branch>]`
**Daemon:** `{"command": "branch-create", "workspace": "...", "session_id": 1, "payload": {"branch_name": "new-feature", "from_branch": "main"}}`

### 4.3 Switch Branch (`branch-switch`)
**CLI:** `proto-vm-cli branch-switch --workspace <path> --session-id <id> --branch-name <name>`
**Daemon:** `{"command": "branch-switch", "workspace": "...", "session_id": 1, "payload": {"branch_name": "feature-branch"}}`

### 4.4 Delete Branch (`branch-delete`)
**CLI:** `proto-vm-cli branch-delete --workspace <path> --session-id <id> --branch-name <name>`
**Daemon:** `{"command": "branch-delete", "workspace": "...", "session_id": 1, "payload": {"branch_name": "old-experiment"}}`

### 4.5 Merge Branch (`branch-merge`)
**CLI:** `proto-vm-cli branch-merge --workspace <path> --session-id <id> --from-branch <source> --to-branch <target>`
**Daemon:** `{"command": "branch-merge", "workspace": "...", "session_id": 1, "payload": {"source_branch": "feature", "target_branch": "main"}}`

## 5. Branch-Aware Commands

### 5.1 Optional Branch Parameter
All existing commands now accept an optional `--branch <name>` parameter:
- If omitted, defaults to `current_branch`
- When specified, operates on the named branch

**CLI Example:**
```bash
# Operate on current branch
proto-vm-cli edit-add-component --workspace /ws --session-id 1 --type AND --x 10 --y 20

# Operate on specific branch
proto-vm-cli edit-add-component --workspace /ws --session-id 1 --branch feature --type AND --x 10 --y 20
```

**Daemon Example:**
```json
{
  "id": "req-42",
  "command": "edit-add-component", 
  "workspace": "/ws",
  "session_id": 1,
  "user_id": "bob",
  "payload": {
    "branch": "feature-branch",
    "type": "AND",
    "x": 10,
    "y": 20
  }
}
```

## 6. Simulation Integration

### 6.1 Per-Branch Revisions
- `circuit_revision` and `sim_revision` are now tracked per branch in `BranchMetadata`
- When `branch.head_revision != branch.sim_revision`, simulation is automatically rebuilt from circuit state
- Each branch maintains independent simulation state

### 6.2 Simulation Commands
Commands like `run-ticks` and `get-state` operate on the current branch by default or specified branch:

```json
{
  "ok": true,
  "command": "get-state",
  "data": {
    "session_id": 1,
    "branch": "alu-experiment",        // NEW: indicates which branch
    "circuit_revision": 10,            // From the specified branch
    "sim_revision": 10,                // From the specified branch
    "total_ticks": 1234
  }
}
```

## 7. Backward Compatibility

### 7.1 Migration
- Existing sessions without branches are automatically migrated
- A single "main" branch is created containing the existing circuit revision history
- `current_branch` is set to "main"
- `branches[0]` contains the original `circuit_revision` and `sim_revision`

### 7.2 Default Behavior
- Commands without `--branch` parameter use `current_branch`
- Default branch behavior matches pre-branching system exactly
- All existing functionality preserved

## 8. Implementation Details

### 8.1 CircuitFacade Updates
- `LoadCurrentCircuitForBranch()` and `ApplyEditOperationsToBranch()` methods added
- Existing methods now call branch-aware versions using `current_branch`

### 8.2 Event Processing
- `ReplayCircuitEventsForBranch()` filters events by branch field
- Only processes events with matching branch name

### 8.3 Branch Operations
- Implemented in `BranchOperations` class
- Handles validation, creation, switching, deletion, and merging
- Maintains branch integrity and prevents invalid operations

## 9. Future Enhancements

### 9.1 Advanced Merging
- Three-way merge algorithm with conflict detection
- Visual merge tools in GUI
- Manual conflict resolution

### 9.2 Branch Management
- Branch protection rules
- Remote branch synchronization
- Branch lifecycle management