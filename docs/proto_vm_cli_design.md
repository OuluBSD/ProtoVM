# ProtoVM CLI Design Document

## 1. Purpose & Scope

The ProtoVM CLI is a new command-line interface designed specifically for programmatic use by tools and AI systems. Unlike the existing human-oriented CLI, this interface:

- Executes one command per invocation (stateless)
- Always returns structured JSON output
- Supports session-based workflows with persistent state
- Provides a clean API for circuit simulation and analysis
- Is designed to be easily integrated into automated workflows

## 2. Command Model

All commands follow the pattern: `proto-vm-cli <command> [options]`

### Input Methods
- CLI flags: `--workspace`, `--session-id`, `--ticks`, `--user-id`, etc.
- JSON on stdin: For complex payloads in future extensions

### Global Options
- `--user-id <string>`: Optional user identifier for event logging (default: "anonymous")

### Available Commands

#### 2.1 `init-workspace`
- Purpose: Initialize a workspace directory structure
- Options:
  - `--workspace <path>` (required)
- Output (success):
```json
{
  "ok": true,
  "command": "init-workspace",
  "error_code": null,
  "error": null,
  "data": {
    "workspace": "/path/to/workspace",
    "created": true,
    "version": "0.1"
  }
}
```
- Output (error):
```json
{
  "ok": false,
  "command": "init-workspace",
  "error_code": "INVALID_WORKSPACE",
  "error": "Directory exists but is not a valid ProtoVM workspace (missing workspace.json)",
  "data": null
}
```

#### 2.2 `create-session`
- Purpose: Create a new simulation session and initialize its Machine from circuit file
- Options:
  - `--workspace <path>` (required)
  - `--circuit-file <path>` (required for now)
- Output:
```json
{
  "ok": true,
  "command": "create-session",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "workspace": "/path/to/workspace",
    "circuit_file": "/path/to/circuit.circuit",
    "state": "ready",
    "total_ticks": 0,
    "last_snapshot_file": "/path/to/workspace/sessions/1/snapshots/snapshot_00000001.bin"
  }
}
```

This command also creates initial snapshot files in `workspace/sessions/<id>/snapshots/` and logs the operation to `workspace/sessions/<id>/events.log`.

#### 2.3 `list-sessions`
- Purpose: List existing sessions in workspace
- Options:
  - `--workspace <path>` (required)
- Output:
```json
{
  "ok": true,
  "command": "list-sessions",
  "error_code": null,
  "error": null,
  "data": {
    "sessions": [
      {
        "session_id": 1,
        "state": 1,
        "circuit_file": "/path/to/circuit.circuit",
        "created_at": "2025-01-01T12:34:56Z",
        "last_used_at": "2025-01-01T13:00:00Z",
        "total_ticks": 0
      }
    ],
    "corrupt_sessions": [3, 7]
  }
}
```

#### 2.4 `run-ticks`
- Purpose: Advance simulation for a session by N ticks
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--ticks <N>` (required, default: 1)
- Output:
```json
{
  "ok": true,
  "command": "run-ticks",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "ticks_run": 100,
    "total_ticks": 1234,
    "last_snapshot_file": "/path/to/workspace/sessions/1/snapshots/snapshot_00000002.bin",
    "state": "ready"
  }
}
```

This command loads the machine from the latest snapshot, runs the specified number of ticks, creates a new snapshot, and logs the operation to `workspace/sessions/<id>/events.log`.

#### 2.5 `get-state`
- Purpose: Get current state of a session
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
- Output:
```json
{
  "ok": true,
  "command": "get-state",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "state": 1,
    "circuit_file": "/path/to/circuit.circuit",
    "total_ticks": 1234,
    "created_at": "2025-01-01T12:34:56Z",
    "last_used_at": "2025-01-01T13:00:00Z",
    "breakpoints": [],
    "traces": [],
    "signals": [],
    "last_snapshot_file": "/path/to/workspace/sessions/1/snapshots/snapshot_00000002.bin"
  }
}
```

#### 2.6 `export-netlist`
- Purpose: Export netlist for a PCB in the session
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--pcb-id <id>` (optional, default: 0)
- Output:
```json
{
  "ok": true,
  "command": "export-netlist",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "pcb_id": 0,
    "netlist_file": "/path/to/workspace/sessions/1/netlists/netlist_0.txt"
  }
}
```

#### 2.7 `destroy-session`
- Purpose: Remove a session and its associated files
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
- Output:
```json
{
  "ok": true,
  "command": "destroy-session",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "deleted": true
  }
}
```

## 3. JSON Output Contract

All responses follow a standard envelope pattern with deterministic key ordering:

```json
{
  "ok": true,
  "command": "command-name",
  "error_code": null,
  "error": null,
  "data": { /* command-specific data */ }
}
```

Or in case of error:

```json
{
  "ok": false,
  "command": "command-name",
  "error_code": "ERROR_CODE_STRING",
  "error": "Human-readable error message",
  "data": null
}
```

### JSON Envelope Specification
- Top-level keys must always appear in this order: `ok`, `command`, `error_code`, `error`, `data`
- `command` is always the string name of the command that was invoked
- On success: `ok: true`, `error_code: null`, `error: null`, `data: object with command-specific content`
- On failure: `ok: false`, `error_code: non-null string`, `error: human-readable description`, `data: null`

## 4. Result & Error Handling Model

### 4.1 Error Codes
The system uses a comprehensive set of error codes to provide precise error information:

```cpp
enum class ErrorCode {
    None = 0,
    WorkspaceNotFound,
    InvalidWorkspace,
    WorkspaceCorrupt,
    SessionNotFound,
    SessionCorrupt,
    SessionDeleted,
    SessionIdConflict,
    CircuitFileNotFound,
    CircuitFileUnreadable,
    StorageIoError,
    StorageSchemaMismatch,
    CommandParseError,
    InternalError
    // add more if needed
};
```

### 4.2 Error Code String Mapping
String representations of error codes are:
- `"NONE"` (ErrorCode::None)
- `"WORKSPACE_NOT_FOUND"` (ErrorCode::WorkspaceNotFound)
- `"INVALID_WORKSPACE"` (ErrorCode::InvalidWorkspace)
- `"WORKSPACE_CORRUPT"` (ErrorCode::WorkspaceCorrupt)
- `"SESSION_NOT_FOUND"` (ErrorCode::SessionNotFound)
- `"SESSION_CORRUPT"` (ErrorCode::SessionCorrupt)
- `"SESSION_DELETED"` (ErrorCode::SessionDeleted)
- `"SESSION_ID_CONFLICT"` (ErrorCode::SessionIdConflict)
- `"CIRCUIT_FILE_NOT_FOUND"` (ErrorCode::CircuitFileNotFound)
- `"CIRCUIT_FILE_UNREADABLE"` (ErrorCode::CircuitFileUnreadable)
- `"STORAGE_IO_ERROR"` (ErrorCode::StorageIoError)
- `"STORAGE_SCHEMA_MISMATCH"` (ErrorCode::StorageSchemaMismatch)
- `"COMMAND_PARSE_ERROR"` (ErrorCode::CommandParseError)
- `"INTERNAL_ERROR"` (ErrorCode::InternalError)

### 4.3 Result Type
Generic container for results with error information:

```cpp
template<typename T>
struct Result {
    bool ok;                     // True on success, false on error
    ErrorCode error_code;        // Error code if ok is false
    std::string error_message;   // Human-readable description
    T data;                      // Data if ok is true
};
```

### 4.4 Helper Functions
- `Result<T> MakeOk(const T& data)` - Create success result
- `Result<T> MakeError(ErrorCode code, const std::string& message)` - Create error result

## 5. Workspace & Session Persistence

### 5.1 workspace.json Schema
The workspace root must contain a workspace.json file with the following structure:

```json
{
  "schema_version": 1,
  "created_at": "2025-01-01T12:34:56Z",
  "created_with": "proto-vm-cli/0.1.0",
  "engine_version": "unknown",
  "next_session_id": 1
}
```

Fields:
- `schema_version`: integer, starting from 1
- `created_at`: ISO8601 UTC timestamp string
- `created_with`: string identifying the CLI version
- `engine_version`: placeholder for engine version (currently "unknown")
- `next_session_id`: integer counter for allocating new sessions

### 5.2 session.json Schema
Each session directory workspace/sessions/<id>/ must contain a session.json with:

```json
{
  "schema_version": 1,
  "session_id": 1,
  "state": 1,
  "circuit_file": "/absolute/path/to/circuit.circuit",
  "created_at": "2025-01-01T12:34:56Z",
  "last_used_at": "2025-01-01T13:00:00Z",
  "total_ticks": 0,
  "engine_version": "unknown"
}
```

Fields:
- `schema_version`: integer, starting from 1
- `session_id`: integer, must match the directory name
- `state`: integer representing the state (0=CREATED, 1=READY, 2=RUNNING, 3=ERROR, 4=DELETED)
- `circuit_file`: absolute path to the circuit file
- `created_at` and `last_used_at`: ISO8601 timestamps in UTC
- `total_ticks`: integer for tracking simulation progress
- `engine_version`: placeholder for engine version

### 5.3 Atomic Write Pattern
All critical JSON files (workspace.json, session.json) use atomic writes:
1. Write to a temporary file in the same directory (e.g., workspace.json.tmp)
2. Flush and close the temporary file
3. Use std::filesystem::rename to replace the original file

This prevents corruption during crashes or partial writes.

### 5.4 Session ID Allocation
Session IDs are allocated using the `next_session_id` field from workspace.json:
1. For `create-session`, read the current `next_session_id`
2. Use this ID for the new session
3. Increment `next_session_id` and write it back to workspace.json
4. All modifications to workspace.json are atomic

## 6. Command Semantics

### 6.1 init-workspace
- Validates workspace path is non-existent (create new) or an existing valid workspace
- On success, returns workspace path, whether it was created, and CLI version
- If workspace directory exists but has no workspace.json, returns `ErrorCode::InvalidWorkspace`

### 6.2 create-session
- Validates workspace is valid and circuit_file exists
- Allocates session_id from next_session_id in workspace.json
- Creates session directory and session.json with full schema
- Errors include: `WorkspaceNotFound`, `InvalidWorkspace`, `CircuitFileNotFound`, `StorageIoError`

### 6.3 list-sessions
- Returns usable sessions and lists any corrupt sessions separately
- Uses Option A approach: excludes corrupt sessions from main list but reports them in `corrupt_sessions` array
- Returns both valid sessions and an array of corrupt session IDs

### 6.4 run-ticks
- Reads and updates session.json consistently
- Increments total_ticks by requested amount
- Updates last_used_at timestamp
- Errors include: `SessionNotFound`, `SessionCorrupt`, etc.

### 6.5 get-state
- Loads session.json and provides stable schema for data
- Includes all expected fields even if stubbed for future capabilities
- Maintains consistent field structure for AI client consumption

### 6.6 export-netlist & destroy-session
- Use the centralized Result/JSON envelope
- Update session.json where necessary
- Use appropriate error codes on failures

## 7. Storage Abstraction

### 7.1 Interface Definition
```cpp
class ISessionStore {
public:
    virtual ~ISessionStore() = default;

    virtual Result<int> CreateSession(const SessionCreateInfo& info) = 0;
    virtual Result<SessionMetadata> LoadSession(int session_id) = 0;
    virtual Result<bool> SaveSession(const SessionMetadata& metadata) = 0;

    struct ListSessionsResult {
        std::vector<SessionMetadata> sessions;
        std::vector<int> corrupt_sessions;
    };
    virtual Result<ListSessionsResult> ListSessions() = 0;

    virtual Result<bool> DeleteSession(int session_id) = 0;
    virtual Result<bool> UpdateSessionState(int session_id, SessionState state) = 0;
    virtual Result<bool> UpdateSessionTicks(int session_id, int ticks) = 0;
};
```

### 7.2 Concrete Implementation
- `JsonFilesystemSessionStore`: File-based storage using JSON files with atomic writes and schema validation

## 8. Engine Integration and Snapshot Model

### 8.1 EngineFacade Layer
An `EngineFacade` class provides the integration layer between the CLI and the core simulation engine:
- Handles machine creation from circuit files
- Manages snapshot creation and loading
- Performs simulation operations (ticks, netlist generation)
- Provides a clean abstraction over the `Machine` class

### 8.2 Snapshot Model
- Each session stores its simulation state as binary snapshot files in `workspace/sessions/<id>/snapshots/`
- Snapshot files follow the naming pattern `snapshot_0000000N.bin`
- The `session.json` file tracks `last_snapshot_file`, `last_snapshot_at`, and `total_ticks`
- CLI commands operate on snapshots: load from latest, process, save new snapshot

### 8.3 Core Components Used
- `Machine`: Main simulation orchestrator
- `Pcb`: Circuit board representation
- `ElectricNodeBase`: Base class for all electronic components
- `CircuitSerializer`: For loading/saving circuit files

### 8.4 Key Integration Points
- **Circuit Loading**: Use existing GUI serializers (`CircuitSerializer::LoadCircuit`) to initialize Machine
- **Simulation Execution**: Load machine from snapshot, run ticks on it, then save new snapshot
- **Netlist Generation**: Load machine from snapshot, generate netlist from PCB data
- **State Querying**: Access machine state through snapshot and session metadata

### 8.5 Assumptions and Limitations
- The `Machine` class can be instantiated and used independently from GUI components
- Snapshot I/O provides serialization for the entire simulation state
- Components maintain their state between simulation ticks
- Thread safety is not required for this initial implementation (single-threaded usage)

## 9. Multi-User & Event Log Foundation

For collaborative workflows, the system provides event logging foundations:

### 9.1 User Identification
- Commands accept optional `--user-id <string>` parameter
- Default value is "anonymous" if not provided
- User ID is opaque string with no authentication/permission checking

### 9.2 Event Logging
- Each session maintains an event log at `workspace/sessions/<id>/events.log`
- Events are JSON lines format with structure:
```json
{
  "timestamp": "2025-01-01T12:34:56Z",
  "user_id": "alice",
  "session_id": 1,
  "command": "run-ticks",
  "params": {
    "ticks": 100
  },
  "result": {
    "total_ticks": 1234
  }
}
```

### 9.3 Logged Commands
Events are recorded for all mutating operations:
- `create-session`
- `run-ticks`
- `export-netlist` (if it writes artifacts)
- `destroy-session`
- All `edit-*` commands

This provides a foundation for future multi-user collaboration features.

## 10. Collaborative Circuit Editing & Event-Replay Layer

### 10.1 Circuit Entity IDs

All circuit entities now have stable, unique identifiers that persist across:
- circuit snapshots,
- event-replay,
- simulation runs (as far as the circuit is unchanged).

- Components have `component_id` fields (format: "C<nnnnnnn>")
- Wires have `wire_id` fields (format: "W<nnnnnnn>")
- Pins have `pin_id` fields (format: "P<nnnnnnn>")

These IDs are persisted in the `.circuit` file format and generated using a global counter system that guarantees uniqueness within a session.

### 10.2 Revision Model

The system tracks two related but distinct revision numbers in `session.json`:

- `circuit_revision`: integer
  - Starts at `0` for the initial circuit state (either empty or loaded from file).
  - Incremented by 1 for each successful **circuit editing operation** (edit-commands).

- `sim_revision`: integer
  - The `circuit_revision` that was used when the most recent Machine snapshot was generated.
  - If simulation has not run yet, this is `0`.

When `circuit_revision > sim_revision`:
- The current Machine snapshot is outdated relative to the circuit.
- `run-ticks` will automatically rebuild the Machine from the **current circuit state** before running simulation ticks.

### 10.3 CircuitFacade

`CircuitFacade` is responsible for:
- Loading the **initial circuit** for a session from the original `.circuit` file
- Replaying **circuit editing events** to produce the current, in-memory circuit representation
- Applying new **editing operations** and appending them as events
- Exporting circuit state for clients

### 10.4 Circuit Editing Operations

The system supports the following canonical editing operations:

1. `add_component` - Add a new component to the circuit
2. `remove_component` - Remove an existing component
3. `move_component` - Move a component to a new position
4. `set_component_property` - Modify a component property
5. `connect` - Create a connection between two component pins
6. `disconnect` - Remove an existing connection

Each operation is expressed as a structured type and logged to the event log with schema.

### 10.5 Edit Commands

The system now supports the following CLI edit commands:

1. `edit-add-component` - Add a new component
2. `edit-remove-component` - Remove a component
3. `edit-move-component` - Move a component
4. `edit-set-component-property` - Set a component property
5. `edit-connect` - Create a connection
6. `edit-disconnect` - Remove a connection
7. `edit-get-circuit` - Export the full circuit state

All edit commands accept `--workspace`, `--session-id`, and `--user-id` parameters.

### 10.6 Event Schema for Edit Operations

When logging an edit operation event, the event log entry includes:
* `revision`: the new `circuit_revision` after applying the op (or ops).
* `timestamp`: ISO8601.
* `user_id`: the user performing the edit.
* `session_id`: numeric.
* `op`: string version of the operation type (e.g. `"add_component"`).
* `params`: object containing operation-specific parameters.

Example JSON line for add_component:
```json
{
  "revision": 5,
  "timestamp": "2025-01-01T12:34:56Z",
  "user_id": "user-123",
  "session_id": 1,
  "op": "add_component",
  "params": {
    "component_id": "C42",
    "type": "AND",
    "x": 100,
    "y": 200,
    "props": {
      "label": "U1",
      "family": "74xx"
    }
  }
}
```

### 10.7 Optimistic Concurrency Control

Each edit command implements simple optimistic concurrency by:
1. Checking the current `circuit_revision` in `session.json`
2. Optionally accepting an `expected_revision` parameter
3. Returning `ErrorCode::Conflict` if the expected revision doesn't match current revision
4. Bumping `circuit_revision` by 1 after applying the edit

### 10.8 Simulation Consistency

The system maintains consistency between edited circuits and simulation by:
- Detecting when `circuit_revision != sim_revision`
- Automatically rebuilding the Machine from the current circuit state before running ticks
- Updating `sim_revision = circuit_revision` after running simulation

## 11. Extended Error Codes

New error codes added for collaborative editing:
- `ErrorCode::Conflict` - For revision mismatch errors
- `ErrorCode::InvalidEditOperation` - For invalid edit operations
- `ErrorCode::CircuitStateCorrupt` - For corrupted circuit state errors

## 12. File Directory Structure

New source files are located in:
```
src/
└── ProtoVMCLI/
    ├── CliMain.cpp           # main() entry point
    ├── CommandDispatcher.h   # Command processing logic
    ├── CommandDispatcher.cpp
    ├── JsonIO.h              # JSON serialization utilities
    ├── JsonIO.cpp
    ├── SessionStore.h        # Storage abstraction interface
    ├── JsonFilesystemSessionStore.cpp  # File-based session storage
    ├── EngineFacade.h        # Engine integration layer
    ├── EngineFacade.cpp
    ├── MachineSnapshot.h     # Machine serialization
    ├── MachineSnapshot.cpp
    ├── EventLogger.h         # Event logging utilities
    ├── EventLogger.cpp
    ├── CircuitFacade.h       # Circuit state management layer
    ├── CircuitFacade.cpp
    ├── CircuitOps.h          # Circuit editing operations
    └── CircuitData.h         # Extended circuit data with stable IDs
```

## 13. Build Integration

The new CLI is built as an additional executable target in the existing CMake build system, linking against the core ProtoVM engine components. This preserves existing build functionality while adding the new CLI binary.

## 14. Diagnostics & Linting

### 14.1 Diagnostic Model

The system defines a structured representation for circuit diagnostics:

- `DiagnosticSeverity`: `Info`, `Warning`, `Error`
- `DiagnosticKind`: `FloatingNet`, `ShortCircuit`, `MultipleDrivers`, `UnconnectedPin`, `InvalidFanout`, `ClockDomainConflict`, `GenericIssue`
- `CircuitDiagnosticLocation`: Contains optional `component_id`, `wire_id`, `pin_name`
- `CircuitDiagnostic`: Complete diagnostic with `severity`, `kind`, `location`, `message`, `suggested_fix`

### 14.2 JSON Representation

Diagnostics are serialized to JSON with a stable structure:

```json
{
  "severity": "warning",
  "kind": "FloatingNet",
  "location": {
    "component_id": "C42",
    "wire_id": "W10",
    "pin_name": "OUT"
  },
  "message": "Net W10 has no driver",
  "suggested_fix": "Connect W10 to a valid output pin"
}
```

### 14.3 Lint Circuit Command

`lint-circuit` performs static analysis on the current circuit state and returns diagnostics:

- Purpose: Run static analysis checks on circuit
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--user-id <string>` (optional)
- Output (success):
```json
{
  "ok": true,
  "command": "lint-circuit",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "circuit_revision": 7,
    "diagnostics": [
      {
        "severity": "warning",
        "kind": "FloatingNet",
        "location": {
          "component_id": "C42",
          "wire_id": "W10",
          "pin_name": "OUT"
        },
        "message": "Net W10 has no driver",
        "suggested_fix": "Connect W10 to a valid output pin"
      }
    ]
  }
}
```

### 14.4 Analyze Circuit Command

`analyze-circuit` performs static analysis and returns diagnostics with summary statistics:

- Purpose: Run detailed static analysis on circuit with statistics
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--user-id <string>` (optional)
- Output (success):
```json
{
  "ok": true,
  "command": "analyze-circuit",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "circuit_revision": 7,
    "summary": {
      "component_count": 123,
      "net_count": 456
    },
    "diagnostics": [ ... ]
  }
}
```

## 15. Daemon Architecture

### 15.1 Overview

The ProtoVM daemon (`proto-vm-daemon`) is a long-running process that maintains circuit and machine state in memory and exposes functionality through a JSON request/response interface over stdin/stdout.

### 15.2 JSON Request/Response Protocol

The daemon uses a JSON-RPC style protocol with the following schema:

Request:
```json
{
  "id": "req-123",           // caller-provided correlation ID
  "command": "edit-add-component",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "alice",
  "payload": {
    "...": "command-specific arguments"
  }
}
```

Response:
```json
{
  "id": "req-123",
  "ok": true,
  "command": "edit-add-component",
  "error_code": null,
  "error": null,
  "data": {
    "...": "command-specific result"
  }
}
```

### 15.3 Session Management

The daemon maintains in-memory state for each session:
- Circuit data (via `CircuitFacade`)
- Simulation/Machine state (via `EngineFacade`)
- Dirty flag indicating unsaved changes

### 15.4 Event Broadcasting

The daemon emits broadcast events to stdout using a different schema (without `id` field):
```json
{
  "event": "session-updated",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "circuit_revision": 8,
  "sim_revision": 8
}
```

### 15.5 Supported Commands

All existing CLI commands are supported in the daemon:
- Session management: `init-workspace`, `create-session`, `list-sessions`, `destroy-session`
- Simulation: `run-ticks`, `get-state`, `export-netlist`
- Circuit editing: All `edit-*` commands
- Analysis: `lint-circuit`, `analyze-circuit`

## 16. Phase 5: Collaborative Editing (CRDT-lite / Event-Ordered Multiuser Mode)

### 16.1 Overview

Phase 5 introduces collaborative editing semantics on top of the existing event-sourced circuit model. This includes concurrency-safe editing with deterministic resolution (revision-based merges) and a suite of collaboration utilities.

### 16.2 Deterministic Event Ordering

The daemon becomes the single point of serialization for all circuit editing events:
- Event #100 always precedes event #101 in the canonical order
- All clients receive events in this authoritative order via broadcast
- Clients apply operations locally using the same replay algorithm

### 16.3 Optimistic Concurrency Control

Clients can send an `expected_revision` parameter with edit commands:
- If `expected_revision == current_revision`: apply operations normally
- If mismatch: daemon attempts deterministic auto-merge
  - If merge succeeds: apply operations + return merged result
  - If merge fails: return `CONFLICT` error

The `allow_merge` parameter (boolean) can be used to control whether auto-merge is attempted.

### 16.4 Auto-Merge Semantics (CRDT-lite)

Simple, deterministic merge rules are implemented in `CircuitMerge`:

- **AddComponent**: If target ID already exists → generate a new derived ID: `<original>_userX_n`
- **RemoveComponent**: If the component was already removed → no-op, success
- **MoveComponent**: If multiple users moved the same component concurrently → last-writer-wins based on daemon event order
- **SetComponentProperty**: If two users change the same property → last-writer-wins
- **Connect/Disconnect**: If wire or component was changed by another event → daemon re-evaluates targeted entity IDs via replay; if connection is now impossible → return CONFLICT

### 16.5 Collaboration Commands

#### 16.5.1 `circuit-diff`
- Purpose: Generate diff between two circuit revisions
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--from-revision <int>` (required)
  - `--to-revision <int>` (required)
- Output:
```json
{
  "ok": true,
  "command": "circuit-diff",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "from_revision": 5,
    "to_revision": 8,
    "diff": [
      {
        "type": 0,
        "component_id": "C42_user_bob_1",
        "x": 100,
        "y": 200
      }
    ]
  }
}
```

#### 16.5.2 `circuit-patch`
- Purpose: Apply a diff between two revisions
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--diff <JSON array>` (required)
  - `--expected-revision <int>` (optional)
- Output:
```json
{
  "ok": true,
  "command": "circuit-patch",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "circuit_revision": 9,
    "applied": true
  }
}
```

#### 16.5.3 `circuit-replay`
- Purpose: Export circuit state at a specific revision
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--revision <int>` (required)
- Output:
```json
{
  "ok": true,
  "command": "circuit-replay",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "revision": 7,
    "circuit_data": "{\"name\":\"circuit\",\"components\":[...],\"wires\":[...]}"
  }
}
```

#### 16.5.4 `circuit-history`
- Purpose: Get metadata for all revisions
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
- Output:
```json
{
  "ok": true,
  "command": "circuit-history",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "current_revision": 12,
    "total_operations": 12,
    "history": [
      {
        "revision": 0,
        "timestamp": "2025-01-01T12:00:00Z",
        "user": "system",
        "operation": "initial_state"
      },
      {
        "revision": 1,
        "timestamp": "2025-01-01T12:01:00Z",
        "user": "alice",
        "operation": "add_component"
      }
    ]
  }
}
```

### 16.6 Enhanced Daemon Protocol

#### 16.6.1 Request Schema Extensions

Edit commands now support additional parameters in the `payload`:

```json
{
  "id": "req-123",
  "command": "edit-add-component",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "alice",
  "payload": {
    "type": "AND",
    "x": 100,
    "y": 200,
    "expected_revision": 7,
    "allow_merge": true
  }
}
```

#### 16.6.2 Response Schema Extensions

Responses now include merge information:

```json
{
  "id": "req-123",
  "ok": true,
  "command": "edit-add-component",
  "error_code": null,
  "error": null,
  "data": {
    "...": "command-specific result",
    "merge": {
      "merged": true,
      "conflict": false,
      "reason": null
    }
  }
}
```

In case of conflict:

```json
{
  "id": "req-123",
  "ok": false,
  "command": "edit-add-component",
  "error_code": "CONFLICT",
  "error": "Component C42 removed by another user",
  "data": {
    "merge": {
      "merged": false,
      "conflict": true,
      "reason": "Component C42 removed by another user"
    }
  }
}
```

#### 16.6.3 Broadcast Events

New broadcast event for merged operations:

```json
{
  "event": "circuit-merged",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "revision": 123,
  "merged_ops": [
    {
      "type": 0,
      "component_id": "C42_user_alice_1",
      "x": 100,
      "y": 200
    }
  ]
}
```

### 16.7 Merge Result Structure

The `MergeResult` structure includes:

```cpp
struct MergeResult {
    bool merged;               // true if auto-merge was applied
    bool conflict;             // true if merge failed
    std::string conflict_reason;    // detailed reason
    std::vector<EditOperation> transformed_ops; // final ops to apply
};
```

### 16.8 Collaboration Utilities

- **Diff/Export**: `circuit-diff` command outputs JSON array of `EditOperation` representing changes between revisions
- **Patch/Import**: `circuit-patch` command applies a diff, performing same checks as other edit commands
- **Time Travel**: `circuit-replay` command outputs circuit state at specific revision
- **History**: `circuit-history` command outputs revision metadata (number, user, op type, timestamp)

### 16.9 Integration Points

- **CircuitMerge Module**: Contains merge resolution logic with `ResolveConcurrentEdits` function
- **CircuitFacade Integration**: Enhanced with merge logic when applying operations with revision mismatches
- **Daemon Extensions**: Request/response schemas updated with collaboration parameters
- **Event Logging**: Enhanced to support collaboration metadata
- **Command Dispatcher**: New collaboration commands implemented

## 17. Graph-Based Circuit Query & Semantic Analysis (Phase 7)

### 17.1 Overview

Phase 7 introduces a graph-based semantic analysis engine that models circuits as graphs with nodes for components, pins, and nets, and edges for connectivity and signal flow. This enables structural and signal-flow queries on digital circuits.

### 17.2 Graph Model

The graph model consists of three node types:
- **Component**: Electronic components (AND, OR, NOT gates, etc.)
- **Pin**: Component input/output pins
- **Net**: Wires connecting pins together

Edge types:
- **Connectivity**: Bidirectional connections between pins and nets
- **SignalFlow**: Directed connections from output pins to input pins

### 17.3 Graph Commands

#### 17.3.1 `graph-export`
- Purpose: Export the entire circuit graph as JSON
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
- Output (success):
```json
{
  "ok": true,
  "command": "graph-export",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "graph": {
      "nodes": [
        { "kind": "Component", "id": "C1" },
        { "kind": "Pin", "id": "C1:OUT" },
        { "kind": "Net", "id": "N10" }
      ],
      "edges": [
        {
          "from": { "kind": "Pin", "id": "C1:OUT" },
          "to": { "kind": "Net", "id": "N10" },
          "kind": "Connectivity"
        },
        {
          "from": { "kind": "Pin", "id": "C1:OUT" },
          "to": { "kind": "Pin", "id": "C2:IN" },
          "kind": "SignalFlow"
        }
      ]
    }
  }
}
```

#### 17.3.2 `graph-paths`
- Purpose: Find signal paths from a source node to a target node
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
  - `--graph-source-kind <Component|Pin|Net>` (required for CLI)
  - `--graph-source-id <id>` (required for CLI)
  - `--graph-target-kind <Component|Pin|Net>` (required for CLI)
  - `--graph-target-id <id>` (required for CLI)
  - `--graph-max-depth <int>` (optional, default: 128)
- Output (success):
```json
{
  "ok": true,
  "command": "graph-paths",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "paths": [
      {
        "nodes": [
          { "kind": "Pin", "id": "C1:OUT" },
          { "kind": "Net", "id": "N10" },
          { "kind": "Pin", "id": "C2:IN" }
        ]
      }
    ]
  }
}
```

#### 17.3.3 `graph-fanin`
- Purpose: Find upstream connectivity from a given node
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
  - `--graph-node-kind <Component|Pin|Net>` (required for CLI)
  - `--graph-node-id <id>` (required for CLI)
  - `--graph-max-depth <int>` (optional, default: 128)
- Output (success):
```json
{
  "ok": true,
  "command": "graph-fanin",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "node": { "kind": "Pin", "id": "C2:IN" },
    "endpoints": [
      { "kind": "Pin", "id": "C1:OUT" },
      { "kind": "Pin", "id": "C0:CLK" }
    ]
  }
}
```

#### 17.3.4 `graph-fanout`
- Purpose: Find downstream connectivity from a given node
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
  - `--graph-node-kind <Component|Pin|Net>` (required for CLI)
  - `--graph-node-id <id>` (required for CLI)
  - `--graph-max-depth <int>` (optional, default: 128)
- Output (success):
```json
{
  "ok": true,
  "command": "graph-fanout",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "node": { "kind": "Pin", "id": "C1:OUT" },
    "endpoints": [
      { "kind": "Pin", "id": "C2:IN" },
      { "kind": "Pin", "id": "C3:IN" }
    ]
  }
}
```

#### 17.3.5 `graph-stats`
- Purpose: Get statistics about the circuit graph
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
- Output (success):
```json
{
  "ok": true,
  "command": "graph-stats",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "node_count": 512,
    "edge_count": 1024
  }
}
```

### 17.4 Daemon Protocol Extensions

The daemon protocol supports the same graph commands as JSON-RPC requests. The payload parameters follow the same schema as the CLI options but use the JSON map structure:

```json
{
  "id": "req-42",
  "command": "graph-paths",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "alice",
  "payload": {
    "branch": "feature",
    "source": {
      "kind": "Pin",
      "id": "C1:OUT"
    },
    "target": {
      "kind": "Pin",
      "id": "C2:IN"
    },
    "max_depth": 64
  }
}
```

### 17.5 Integration Points

- **CircuitGraph Module**: Contains graph construction logic in `CircuitGraphBuilder`
- **CircuitGraphQueries Module**: Implements pathfinding and fan analysis algorithms
- **CircuitFacade Integration**: Enhanced with `BuildGraphForBranch` method
- **Command Dispatcher**: New graph commands implemented
- **SessionServer**: Graph commands supported in daemon protocol
- **CLI Integration**: Graph commands available as CLI commands

## 18. Timing Analysis Engine (Phase 8)

### 18.1 Overview

Phase 8 introduces a timing analysis engine that builds on top of the graph model to provide static timing analysis capabilities. This includes critical path analysis, cycle detection, and hazard identification.

### 18.2 Timing Commands

#### 18.2.1 `timing-summary`
- Purpose: Compute global timing summary for circuit
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
- Output (success):
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

#### 18.2.2 `timing-critical-paths`
- Purpose: Find the deepest timing paths in the circuit
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
  - `--max-paths <N>` (optional, default: 5)
  - `--max-depth <D>` (optional, default: 1024)
- Output (success):
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

#### 18.2.3 `timing-loops`
- Purpose: Detect combinational loops in the circuit
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
- Output (success):
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

#### 18.2.4 `timing-hazards`
- Purpose: Detect potential hazard-prone structures (reconvergent fanout)
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--branch <name>` (optional, defaults to current branch)
  - `--max-results <N>` (optional, default: 64)
- Output (success):
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

### 18.3 Daemon Protocol Extensions

The daemon protocol supports all timing commands as JSON-RPC requests. The payload parameters follow the same schema as the CLI options:

```json
{
  "id": "req-42",
  "command": "timing-critical-paths",
  "workspace": "/path/to/ws",
  "session_id": 1,
  "user_id": "alice",
  "payload": {
    "branch": "feature",
    "max_paths": 10,
    "max_depth": 512
  }
}
```

### 18.4 Integration Points

- **TimingAnalysis Module**: Contains timing analysis algorithms
- **TimingGraphBuilder Module**: Converts circuit graph to timing representation
- **CircuitFacade Integration**: Enhanced with `BuildTimingGraphForBranch` method
- **Command Dispatcher**: New timing commands implemented
- **SessionServer**: Timing commands supported in daemon protocol
- **JsonIO Integration**: Serialization methods for timing structures

## 19. Phase 9: Functional Dependency Analysis & Automatic Dependency Extraction

### 19.1 Overview

Phase 9 introduces functional cone analysis and automatic dependency extraction, creating a dependency analysis engine that can compute "who influences what" and "what is affected if this changes" queries over digital circuits.

### 19.2 Functional Dependency Model

- **Backward Cone (Cone of Influence)**: All upstream nodes whose value can affect a given root node
- **Forward Cone (Cone of Impact)**: All downstream nodes whose value can be affected by a given root node
- **Functional Dependency**: A relation `A -> B` meaning B is functionally dependent on A's value

Node ID conventions:
- **Pin**: `"ComponentId:PinName"` (e.g., "C1:OUT")
- **Component**: `"ComponentId"` (e.g., "C1")
- **Net**: `"NetId"` (e.g., "N10")

### 19.3 New Commands

#### 19.3.1 `deps-summary`
Get dependency summary (upstream/downstream counts) for a node.

CLI usage:
```
proto-vm-cli deps-summary --workspace <workspace> --session-id <session_id> --node-id <node_id> --node-kind <Pin|Component|Net> --max-depth <depth>
```

Daemon request:
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

#### 19.3.2 `deps-backward`
Compute backward cone (upstream influences) for a node.

CLI usage:
```
proto-vm-cli deps-backward --workspace <workspace> --session-id <session_id> --node-id <node_id> --node-kind <Pin|Component|Net> --max-depth <depth>
```

Daemon request:
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

#### 19.3.3 `deps-forward`
Compute forward cone (downstream impacts) for a node.

CLI usage:
```
proto-vm-cli deps-forward --workspace <workspace> --session-id <session_id> --node-id <node_id> --node-kind <Pin|Component|Net> --max-depth <depth>
```

Daemon request:
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

#### 19.3.4 `deps-both`
Compute both forward and backward cones plus summary in a single call.

CLI usage:
```
proto-vm-cli deps-both --workspace <workspace> --session-id <session_id> --node-id <node_id> --node-kind <Pin|Component|Net> --max-depth <depth>
```

Daemon request:
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

### 19.4 Implementation Details

- **FunctionalAnalysis Module**: Core algorithms for cone computation
- **CircuitFacade Integration**: Enhanced with `BuildBackwardConeForBranch`, `BuildForwardConeForBranch`, and `BuildDependencySummaryForBranch` methods
- **JsonIO Integration**: Serialization methods for functional analysis structures
- **Node Resolution**: Utilities to map user-provided identifiers to functional/graph nodes

### 19.5 Relationship to Other Analysis Types

- **Graph Analysis** (`graph-*`): Structural/topological queries
- **Timing Analysis** (`timing-*`): Temporal and delay-based queries
- **Functional Analysis** (`deps-*`): Functional dependency queries following signal flow direction

## 20. Phase 10: Block Analysis & Semantic Subcircuit Discovery

### 20.1 Overview

Phase 10 introduces a semantic subcircuit detection and block analysis system that identifies meaningful higher-level functional units within low-level circuit descriptions. The system discovers common digital logic patterns (adders, multiplexers, decoders, etc.) and provides tools to query and inspect them.

### 20.2 Block Analysis Model

- **BlockKind**: Semantic classification of blocks (`GenericComb`, `Adder`, `Mux`, `Decoder`, `Comparator`, `Register`, `Counter`, `Latch`, etc.)
- **BlockInstance**: Specific occurrence of a block with components, nets, and interface ports
- **BlockPort**: Logical interface with name, direction ("in", "out", "inout"), and underlying pin IDs
- **BlockGraph**: Collection of discovered blocks with optional inter-block connectivity

### 20.3 Block Commands

#### 20.3.1 `blocks-list`
List all discovered blocks in the circuit.

CLI usage:
```
proto-vm-cli blocks-list --workspace <workspace> --session-id <session_id> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "blocks-list",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main"
  }
}
```

Response:
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

#### 20.3.2 `blocks-export`
Export a block-level view of the circuit.

CLI usage:
```
proto-vm-cli blocks-export --workspace <workspace> --session-id <session_id> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "blocks-export",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main"
  }
}
```

Response:
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

#### 20.3.3 `block-inspect`
Inspect a specific block by ID.

CLI usage:
```
proto-vm-cli block-inspect --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "block-inspect",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B1"
  }
}
```

Response:
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

### 20.4 Implementation Details

- **BlockAnalysis Module**: Pattern detection algorithms for identifying semantic blocks
- **CircuitFacade Integration**: Enhanced with `BuildBlockGraphForBranch` method
- **JsonIO Integration**: Serialization methods for block analysis structures
- **Pattern Detection**: Heuristic-based identification of common digital logic patterns
- **Block Interface Inference**: Automatic determination of block inputs/outputs based on external connections

## 21. Phase 11: Behavioral Analysis & Semantic Behavioral Inference (This Phase)

### 21.1 Overview

Phase 11 introduces a behavioral inference and semantic understanding system that identifies high-level behavioral descriptions of digital circuits. The system provides structured, machine-readable behavioral descriptors like "4-bit ripple-carry adder", "2:1 multiplexer", "N-bit equality comparator", etc.

### 21.2 Behavioral Model

The behavioral model consists of:

- **BehaviorKind**: Semantic classification of behavior (`Unknown`, `Adder`, `Mux`, `Register`, etc.)
- **BehaviorPortRole**: Semantic role for a port (`data_in`, `data_out`, `select`, `clock`, etc.)
- **BehaviorDescriptor**: Structured summary of circuit behavior with ports, bit width, and description

### 21.3 Behavioral Analysis Commands

#### 21.3.1 `behavior-block`
Infer behavior for a specific block.

CLI usage:
```
proto-vm-cli behavior-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "behavior-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B1"
  }
}
```

Response:
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
      "subject_id": "B1",
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

#### 21.3.2 `behavior-node`
Infer behavior for an arbitrary node (pin/component/net).

CLI usage:
```
proto-vm-cli behavior-node --workspace <workspace> --session-id <session_id> --node-id <node_id> [--node-kind <Pin|Component|Net>] [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "behavior-node",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "node_id": "C10:OUT",
    "node_kind": "Pin"
  }
}
```

Response:
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

#### 21.3.3 `ir-block`
Generate HLS IR for a specific circuit block.

CLI usage:
```
proto-vm-cli ir-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "ir-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B3"
  }
}
```

Response:
```json
{
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

#### 21.3.4 `ir-node-region`
Generate HLS IR for a small region around a specific node.

CLI usage:
```
proto-vm-cli ir-node-region --workspace <workspace> --session-id <session_id> --node-id <node_id> [--node-kind <Pin|Component|Net>] [--max-depth <int>] [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "ir-node-region",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "node_id": "C10:OUT",
    "node_kind": "Pin",
    "max_depth": 4
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "ir-node-region",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "ir": {
      "id": "C10:OUT_region",
      "inputs": [
        { "name": "A", "bit_width": 1, "is_literal": false, "literal": null },
        { "name": "B", "bit_width": 1, "is_literal": false, "literal": null }
      ],
      "outputs": [
        { "name": "OUT", "bit_width": 1, "is_literal": false, "literal": null }
      ],
      "comb_assigns": [
        {
          "kind": "And",
          "target": { "name": "OUT", "bit_width": 1, "is_literal": false, "literal": null },
          "args": [
            { "name": "A", "bit_width": 1, "is_literal": false, "literal": null },
            { "name": "B", "bit_width": 1, "is_literal": false, "literal": null }
          ]
        }
      ],
      "reg_assigns": []
    }
  }
}
```

#### 21.3.5 `ir-opt-block`
Preview IR-level optimization for a block without applying changes.

CLI usage:
```
proto-vm-cli ir-opt-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>] [--passes <pass1,pass2,...>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "ir-opt-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B3",
    "passes": ["SimplifyAlgebraic", "FoldConstants"]
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "ir-opt-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "B3",
    "optimization": {
      "original": { ... IrModule ... },
      "optimized": { ... IrModule ... },
      "summaries": [
        {
          "pass_kind": "SimplifyAlgebraic",
          "expr_changes": 2,
          "reg_changes": 0,
          "behavior_preserved": true
        }
      ]
    }
  }
}
```

#### 21.3.6 `ir-opt-refactor-block`
Compute IR optimizations and generate transformation plans directly.

CLI usage:
```
proto-vm-cli ir-opt-refactor-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>] [--passes <pass1,pass2,...>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "ir-opt-refactor-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B3",
    "passes": ["SimplifyAlgebraic", "FoldConstants"]
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "ir-opt-refactor-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "B3",
    "plans": [
      {
        "id": "IR_T1",
        "kind": "SimplifyDoubleInversion",
        "target": { "subject_id": "B3", "subject_kind": "Block" },
        "guarantees": ["BehaviorKindPreserved", "IOContractPreserved"],
        "steps": [
          { "description": "Remove redundant NOT-then-NOT around SUM path" }
        ]
      }
    ]
  }
}
```

#### 21.3.7 `schedule-block`
Generate scheduled (pipelined) HLS IR for a specific circuit block with configurable pipeline stages.

CLI usage:
```
proto-vm-cli schedule-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>] --strategy <SingleStage|DepthBalancedStages|FixedStageCount> [--stages <int>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "schedule-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B3",
    "strategy": "DepthBalancedStages",
    "stages": 3
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "schedule-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "B3",
    "scheduled_ir": {
      "id": "B3",
      "num_stages": 3,
      "inputs": [
        { "name": "A", "bit_width": 4, "is_literal": false, "literal": null },
        { "name": "B", "bit_width": 4, "is_literal": false, "literal": null },
        { "name": "CIN", "bit_width": 1, "is_literal": false, "literal": null }
      ],
      "outputs": [
        { "name": "SUM", "bit_width": 4, "is_literal": false, "literal": null },
        { "name": "COUT", "bit_width": 1, "is_literal": false, "literal": null }
      ],
      "comb_ops": [
        {
          "stage": 0,
          "expr": {
            "kind": "Add",
            "target": { "name": "SUM", "bit_width": 4, "is_literal": false, "literal": null },
            "args": [
              { "name": "A", "bit_width": 4, "is_literal": false, "literal": null },
              { "name": "B", "bit_width": 4, "is_literal": false, "literal": null }
            ]
          }
        }
      ],
      "reg_ops": [
        {
          "stage": 2,
          "reg_assign": {
            "target": { "name": "Q", "bit_width": 4, "is_literal": false, "literal": null },
            "expr": { ... },
            "clock": "CLK",
            "reset": "RST"
          }
        }
      ]
    }
  }
}
```

#### 21.3.8 `schedule-node-region`
Generate scheduled (pipelined) HLS IR for a small region around a specific node with configurable pipeline stages.

CLI usage:
```
proto-vm-cli schedule-node-region --workspace <workspace> --session-id <session_id> --node-id <node_id> [--node-kind <Pin|Component|Net>] [--max-depth <int>] [--branch <branch_name>] --strategy <SingleStage|DepthBalancedStages|FixedStageCount> [--stages <int>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "schedule-node-region",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "node_id": "C10:OUT",
    "node_kind": "Pin",
    "max_depth": 4,
    "strategy": "DepthBalancedStages",
    "stages": 2
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "schedule-node-region",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "node_id": "C10:OUT",
    "scheduled_ir": {
      "id": "C10:OUT_region",
      "num_stages": 2,
      "inputs": [
        { "name": "A", "bit_width": 1, "is_literal": false, "literal": null },
        { "name": "B", "bit_width": 1, "is_literal": false, "literal": null }
      ],
      "outputs": [
        { "name": "OUT", "bit_width": 1, "is_literal": false, "literal": null }
      ],
      "comb_ops": [
        {
          "stage": 0,
          "expr": {
            "kind": "And",
            "target": { "name": "TMP1", "bit_width": 1, "is_literal": false, "literal": null },
            "args": [
              { "name": "A", "bit_width": 1, "is_literal": false, "literal": null },
              { "name": "B", "bit_width": 1, "is_literal": false, "literal": null }
            ]
          }
        },
        {
          "stage": 1,
          "expr": {
            "kind": "Not",
            "target": { "name": "OUT", "bit_width": 1, "is_literal": false, "literal": null },
            "args": [
              { "name": "TMP1", "bit_width": 1, "is_literal": false, "literal": null }
            ]
          }
        }
      ],
      "reg_ops": []
    }
  }
}
```

#### 21.3.9 `pipeline-block`
Analyze a circuit block to identify clock domains, pipeline stages, and register-to-register paths.

CLI usage:
```
proto-vm-cli pipeline-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "pipeline-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B_PIPE_STAGE1"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "pipeline-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "B_PIPE_STAGE1",
    "pipeline_map": {
      "id": "B_PIPE_STAGE1",
      "clock_domains": [
        { "signal_name": "CLK", "domain_id": 0 }
      ],
      "registers": [
        { "reg_id": "R1", "name": "REG_A", "clock_signal": "CLK", "domain_id": 0, "reset_signal": "RST" },
        { "reg_id": "R2", "name": "REG_B", "clock_signal": "CLK", "domain_id": 0, "reset_signal": "RST" }
      ],
      "stages": [
        {
          "stage_index": 0,
          "domain_id": 0,
          "registers_in": ["R1"],
          "registers_out": ["R2"],
          "comb_depth_estimate": 5
        }
      ],
      "reg_paths": [
        {
          "src_reg_id": "R1",
          "dst_reg_id": "R2",
          "domain_id": 0,
          "comb_depth_estimate": 5,
          "stage_span": 0,
          "crosses_clock_domain": false
        }
      ]
    }
  }
}
```

#### 21.3.10 `pipeline-subsystem`
Analyze a subsystem composed of multiple circuit blocks to identify clock domains, pipeline stages, and register-to-register paths across the subsystem.

CLI usage:
```
proto-vm-cli pipeline-subsystem --workspace <workspace> --session-id <session_id> --subsystem-id <subsystem_id> --block-ids <comma_separated_block_ids> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "pipeline-subsystem",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "subsystem_id": "ALU_PIPE",
    "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"]
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "pipeline-subsystem",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "subsystem_id": "ALU_PIPE",
    "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"],
    "pipeline_map": {
      "id": "ALU_PIPE",
      "clock_domains": [
        { "signal_name": "CLK", "domain_id": 0 },
        { "signal_name": "CLK_FAST", "domain_id": 1 }
      ],
      "registers": [
        { "reg_id": "R1", "name": "REG_A", "clock_signal": "CLK", "domain_id": 0, "reset_signal": "RST" },
        { "reg_id": "R2", "name": "REG_B", "clock_signal": "CLK_FAST", "domain_id": 1, "reset_signal": "RST" }
      ],
      "stages": [
        {
          "stage_index": 0,
          "domain_id": 0,
          "registers_in": ["R1"],
          "registers_out": ["R2"],
          "comb_depth_estimate": 7
        }
      ],
      "reg_paths": [
        {
          "src_reg_id": "R1",
          "dst_reg_id": "R2",
          "domain_id": -1,
          "comb_depth_estimate": 7,
          "stage_span": 1,
          "crosses_clock_domain": true
        }
      ]
    }
  }
}
```

#### 21.3.11 `cdc-block`
Analyze a circuit block to identify and classify clock-domain crossings (CDC) and report potential hazards.

CLI usage:
```
proto-vm-cli cdc-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "cdc-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B_PIPE_STAGE1"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "cdc-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "B_PIPE_STAGE1",
    "cdc_report": {
      "id": "B_PIPE_STAGE1",
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

#### 21.3.12 `cdc-subsystem`
Analyze a subsystem composed of multiple circuit blocks to identify and classify clock-domain crossings across the subsystem.

CLI usage:
```
proto-vm-cli cdc-subsystem --workspace <workspace> --session-id <session_id> --subsystem-id <subsystem_id> --block-ids <comma_separated_block_ids> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "cdc-subsystem",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "subsystem_id": "ALU_PIPE",
    "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"]
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "cdc-subsystem",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "subsystem_id": "ALU_PIPE",
    "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"],
    "cdc_report": {
      "id": "ALU_PIPE",
      "clock_domains": [
        { "signal_name": "CLK_A", "domain_id": 0 },
        { "signal_name": "CLK_B", "domain_id": 1 },
        { "signal_name": "CLK_FAST", "domain_id": 2 }
      ],
      "crossings": [
        {
          "id": "CDCC_0001",
          "src": { "reg_id": "R1", "clock_signal": "CLK_A", "domain_id": 0 },
          "dst": { "reg_id": "R2", "clock_signal": "CLK_B", "domain_id": 1 },
          "kind": "SingleBitSyncCandidate",
          "is_single_bit": true,
          "bit_width": 1,
          "crosses_reset_boundary": false
        }
      ],
      "issues": [
        {
          "id": "CDCISS_0001",
          "severity": "Warning",
          "summary": "Single-bit CDC from CLK_A to CLK_B.",
          "detail": "Single-bit control signal crossing clock domains. This is typically safe with a 2-flop synchronizer."
        }
      ]
    }
  }
}
```

#### 21.3.13 `retime-block`
Analyze a circuit block for safe intra-clock-domain retiming opportunities and return suggested register movement plans.

CLI usage:
```
proto-vm-cli retime-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>] [--min-depth <N>] [--max-plans <M>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "retime-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B_PIPE_STAGE1",
    "min_depth": 5,
    "max_plans": 10
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "retime-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "B_PIPE_STAGE1",
    "min_depth": 5,
    "max_plans": 10,
    "retiming_plans": [
      {
        "id": "RTP_B_PIPE_1",
        "target_id": "B_PIPE_STAGE1",
        "description": "Balance long path between REG_A and REG_B",
        "moves": [
          {
            "move_id": "RTM_0001",
            "src_reg_id": "REG_A",
            "dst_reg_id": "REG_B",
            "direction": "Forward",
            "domain_id": 0,
            "src_stage_index": 0,
            "dst_stage_index": 1,
            "before_comb_depth": 10,
            "after_comb_depth_est": 5,
            "safety": "SafeIntraDomain",
            "safety_reason": "Intra-domain, no CDC crossings, internal path",
            "affected_ops": ["ADD_1", "MUX_2"]
          }
        ],
        "estimated_max_depth_before": 10,
        "estimated_max_depth_after": 6,
        "respects_cdc_fences": true
      }
    ]
  }
}
```

#### 21.3.14 `retime-subsystem`
Analyze a multi-block subsystem for safe intra-clock-domain retiming opportunities and return suggested register movement plans.

CLI usage:
```
proto-vm-cli retime-subsystem --workspace <workspace> --session-id <session_id> --subsystem-id <subsystem_id> --block-ids <comma_separated_block_ids> [--branch <branch_name>] [--min-depth <N>] [--max-plans <M>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "retime-subsystem",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "subsystem_id": "ALU_PIPE",
    "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"],
    "min_depth": 6,
    "max_plans": 20
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "retime-subsystem",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "subsystem_id": "ALU_PIPE",
    "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"],
    "min_depth": 6,
    "max_plans": 20,
    "retiming_plans": [
      {
        "id": "RTP_ALU_1",
        "target_id": "ALU_PIPE",
        "description": "Reduce critical path in ALU pipeline",
        "moves": [
          {
            "move_id": "RTM_FWD_1",
            "src_reg_id": "REG_IN",
            "dst_reg_id": "REG_OUT",
            "direction": "Forward",
            "domain_id": 0,
            "src_stage_index": 0,
            "dst_stage_index": 2,
            "before_comb_depth": 15,
            "after_comb_depth_est": 7,
            "safety": "SafeIntraDomain",
            "safety_reason": "Intra-domain, no CDC crossings, internal path",
            "affected_ops": ["ADD_OP", "SHIFT_OP"]
          }
        ],
        "estimated_max_depth_before": 15,
        "estimated_max_depth_after": 8,
        "respects_cdc_fences": true
      }
    ]
  }
}
```

### 21.4 Implementation Details

- **RetimingModel Module**: Core data structures for retiming analysis (RetimingMoveDirection, RetimingMoveSafety, RetimingMove, RetimingPlan)
- **RetimingAnalysis Module**: Analysis engine that identifies register movement opportunities within single clock domains
- **CircuitFacade Integration**: Enhanced with `AnalyzeRetimingForBlockInBranch` and `AnalyzeRetimingForSubsystemInBranch` methods
- **JsonIO Integration**: Serialization methods for retiming structures (RetimingMoveDirectionToJson, RetimingMoveToValueMap, etc.)
- **Integration with PipelineMap**: Uses clock domain information and register paths to identify safe retiming opportunities
- **CDC Safety**: Respects CDC boundaries from `CdcReport` as hard fences preventing unsafe register movements
- **Planning Only**: This phase generates retiming *plans* but does not apply them; transformations happen in a later phase

## 22. Phase 12: Behavior-Preserving Transformations / Refactor Engine

### 22.1 Overview

The refactoring engine provides behavior-preserving structural transformations for circuits using semantic analysis. It identifies local optimization opportunities and applies them while ensuring high-level circuit behavior is preserved.

### 22.2 Transformation Model

The system defines a structured transformation model:
- `TransformationKind`: Types of transformations (SimplifyDoubleInversion, SimplifyRedundantGate, ReplaceWithKnownBlock, etc.)
- `PreservationLevel`: Levels of behavior preservation (BehaviorKindPreserved, IOContractPreserved, DependencyPatternPreserved)
- `TransformationPlan`: A structured plan with target, guarantees, and steps

### 22.3 CLI Commands

#### 22.3.1 `refactor-suggest`
Propose behavior-preserving transformations for the entire branch.

CLI usage:
```
proto-vm-cli refactor-suggest --workspace <workspace> --session-id <session_id> [--branch <branch_name>] [--max-plans <n>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "refactor-suggest",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "max_plans": "10"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "refactor-suggest",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "plans": [
      {
        "id": "TRANS_1",
        "kind": "SimplifyDoubleInversion",
        "target": {
          "subject_id": "C10",
          "subject_kind": "Component"
        },
        "guarantees": ["BehaviorKindPreserved", "IOContractPreserved"],
        "steps": [
          {
            "description": "Remove double inverter chain between C5:OUT and C10:IN"
          }
        ]
      }
    ]
  }
}
```

#### 22.3.2 `refactor-suggest-block`
Propose behavior-preserving transformations for a specific block.

CLI usage:
```
proto-vm-cli refactor-suggest-block --workspace <workspace> --session-id <session_id> --block-id <block_id> [--branch <branch_name>] [--max-plans <n>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "refactor-suggest-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "block_id": "B1",
    "max_plans": "5"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "refactor-suggest-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "B1",
    "plans": [
      {
        "id": "TRANS_2",
        "kind": "ReplaceWithKnownBlock",
        "target": {
          "subject_id": "B1",
          "subject_kind": "Block"
        },
        "guarantees": ["BehaviorKindPreserved", "IOContractPreserved", "DependencyPatternPreserved"],
        "steps": [
          {
            "description": "Replace generic combinational block with canonical Adder block"
          }
        ]
      }
    ]
  }
}
```

#### 22.3.3 `refactor-apply`
Apply a selected transformation plan.

CLI usage:
```
proto-vm-cli refactor-apply --workspace <workspace> --session_id <session_id> --plan-id <plan_id> [--branch <branch_name>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "refactor-apply",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch": "main",
    "plan_id": "TRANS_1"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "refactor-apply",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "applied_plan_id": "TRANS_1",
    "new_circuit_revision": 42
  }
}
```

### 22.4 Implementation Details

- **TransformationEngine Module**: Core transformation proposal and materialization algorithms
- **Behavior Preservation Verification**: Heuristic checks to ensure transformations maintain behavioral semantics
- **CircuitFacade Integration**: Enhanced with transformation-related methods
- **JsonIO Integration**: Serialization methods for transformation structures
- **Integration with Analysis Modules**: Uses CircuitGraph, BlockAnalysis, BehavioralAnalysis, FunctionalAnalysis for pattern detection
- **Session/Revision System Integration**: Properly integrates with existing session, branch, and revision mechanisms

### 22.5 Transformation Patterns

The engine currently supports:
- Double inversion simplification (A → NOT → NOT → B becomes A → B)
- Redundant gate simplification (X AND X = X, X OR X = X)
- Known block replacement (generic blocks reclassified as canonical types)
- Additional patterns can be added through the extensible architecture

## 23. Phase 14: Behavioral & IR Diff Engine

### 23.1 Overview

Phase 14 introduces a regression analysis layer that compares behavioral information and HLS-lite IR modules across different branches and revisions. This engine enables understanding how circuit behavior changes across branches and revisions, turning ProtoVM into a regression-awareness engine.

### 23.2 Diff Model

The diff engine provides structured comparison of:
- High-level behavioral information (BehaviorDescriptor, BlockKind, BehaviorKind)
- HLS-lite IR modules (IrModule)

### 23.3 Diff Commands

#### 23.3.1 `behavior-diff-block`
Compare the behavioral description of a specific block between two branches.

CLI usage:
```
proto-vm-cli behavior-diff-block --workspace <workspace> --session-id <session_id> --branch-before <branch_name> --branch-after <branch_name> --block-id <block_id>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "behavior-diff-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "block_id": "B3"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "behavior-diff-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "behavior_diff": {
      "subject_id": "B3",
      "subject_kind": "Block",
      "change_kind": "BehaviorKindChanged",
      "before_behavior": {
        "subject_id": "B3",
        "subject_kind": "Block",
        "behavior_kind": "Adder",
        "bit_width": 4,
        "ports": [ ... ],
        "description": "4-bit ripple-carry adder with carry in/out"
      },
      "after_behavior": {
        "subject_id": "B3",
        "subject_kind": "Block",
        "behavior_kind": "GenericComb",
        "bit_width": 4,
        "ports": [ ... ],
        "description": "Generic combinational logic"
      },
      "port_changes": [
        {
          "port_name": "SUM",
          "before_role": "data_out",
          "after_role": "data_out",
          "before_width": 4,
          "after_width": 4
        }
      ]
    }
  }
}
```

#### 23.3.2 `ir-diff-block`
Compare the HLS IR representation of a specific block between two branches.

CLI usage:
```
proto-vm-cli ir-diff-block --workspace <workspace> --session-id <session_id> --branch-before <branch_name> --branch-after <branch_name> --block-id <block_id>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "ir-diff-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "block_id": "B3"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "ir-diff-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "ir_diff": {
      "module_id": "B3",
      "change_kind": "CombLogicChanged",
      "iface_changes": {
        "added_inputs": [],
        "removed_inputs": [],
        "added_outputs": [],
        "removed_outputs": []
      },
      "comb_changes": [
        {
          "target_name": "SUM",
          "before_expr_repr": "SUM = A + B",
          "after_expr_repr": "SUM = A + B + 1"
        }
      ],
      "reg_changes": []
    }
  }
}
```

#### 23.3.3 `ir-diff-node-region` (Optional)
Compare the HLS IR representation of a node region between two branches.

CLI usage:
```
proto-vm-cli ir-diff-node-region --workspace <workspace> --session-id <session_id> --branch-before <branch_name> --branch-after <branch_name> --node-id <node_id> [--node-kind <Pin|Component|Net>] [--max-depth <int>]
```

Daemon request:
```json
{
  "id": "req1",
  "command": "ir-diff-node-region",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "node_id": "C10:OUT",
    "node_kind": "Pin",
    "max_depth": 4
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "ir-diff-node-region",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch_before": "main",
    "branch_after": "experiment-alu",
    "ir_diff": {
      "module_id": "C10:OUT_region",
      "change_kind": "InterfaceChanged",
      "iface_changes": {
        "added_inputs": [
          { "name": "CLOCK", "bit_width": 1, "is_literal": false }
        ],
        "removed_inputs": [],
        "added_outputs": [],
        "removed_outputs": []
      },
      "comb_changes": [],
      "reg_changes": []
    }
  }
}
```

### 23.4 Change Classification

The diff engine classifies changes into meaningful categories:

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

### 23.5 Implementation Details

- **DiffAnalysis Module**: Core diff comparison algorithms
- **CircuitFacade Integration**: Enhanced with branch-aware diff helpers
- **JsonIO Integration**: Serialization methods for diff structures
- **Branch-Aware Operations**: All diffs compare the same entity across different branches
- **Regression Detection**: Enables detection of behavioral changes across development branches
- **Read-Only Operations**: All diff operations are read-only and don't modify circuit state

## 24. Phase 16: AI Co-Designer / Interactive Design Session API Commands

Phase 16 introduces the Co-Designer API, providing a structured interface for AI clients to interact with circuit designs. These commands create an interactive design session layer on top of existing ProtoVM capabilities.

### 24.1 Co-Designer Session Model

CoDesignerSession wraps an existing ProtoVM session and branch, maintaining focus on a specific block or node region. Sessions are in-memory only and not persisted beyond daemon lifetime.

### 24.2 Co-Designer Commands

#### 24.2.1 `designer-create-session`
Create a new CoDesignerSession bound to an existing ProtoVM session/branch.

CLI usage:
```
proto-vm-cli designer-create-session --workspace <workspace> --proto-session-id <session_id> --branch <branch_name>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-create-session",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "proto_session_id": 1,
    "branch": "main"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-create-session",
  "error_code": null,
  "error": null,
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

#### 24.2.2 `designer-set-focus`
Set the current focus for the CoDesignerSession.

CLI usage:
```
proto-vm-cli designer-set-focus --workspace <workspace> --designer-session-id <session_id> --block-id <block_id> --node-id <node_id> --node-kind <Pin|Component|Net> --use-optimized-ir <true|false>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-set-focus",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "block_id": "B3",
    "node_id": "C10:OUT",
    "node_kind": "Pin",
    "use_optimized_ir": true
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-set-focus",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": {
      "designer_session_id": "cd-1234",
      "proto_session_id": 1,
      "branch": "main",
      "current_block_id": "B3",
      "current_node_id": "C10:OUT",
      "current_node_kind": "Pin",
      "use_optimized_ir": true
    }
  }
}
```

#### 24.2.3 `designer-get-context`
Retrieve current context for the AI to orient itself.

CLI usage:
```
proto-vm-cli designer-get-context --workspace <workspace> --designer-session-id <session_id>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-get-context",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234"
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-get-context",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "block_behavior": { ... BehaviorDescriptor ... },  // if current_block_id is set
    "node_behavior": { ... BehaviorDescriptor ... }    // if current_node_id is set
  }
}
```

#### 24.2.4 `designer-analyze`
Provide a bundle of analysis results based on current focus.

CLI usage:
```
proto-vm-cli designer-analyze --workspace <workspace> --designer-session-id <session_id> --include-behavior <true|false> --include-ir <true|false> --include-graph-stats <true|false> --include-timing <true|false>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-analyze",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "include_behavior": true,
    "include_ir": true,
    "include_graph_stats": false,
    "include_timing": false
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-analyze",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "block": {
      "block_id": "B3",
      "behavior": { ... BehaviorDescriptor ... },
      "ir": { ... IrModule ... }
    },
    "node": {
      "node_id": "C10:OUT",
      "behavior": { ... BehaviorDescriptor ... },
      "ir": { ... IrModule ... }
    }
  }
}
```

#### 24.2.5 `designer-optimize`
Run IR optimizations for the focused block or node-region.

CLI usage:
```
proto-vm-cli designer-optimize --workspace <workspace> --designer-session-id <session_id> --target <block|node> --passes <pass1,pass2,...>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-optimize",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "target": "block",
    "passes": ["SimplifyAlgebraic", "FoldConstants"]
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-optimize",
  "error_code": null,
  "error": null,
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

#### 24.2.6 `designer-propose-refactors`
Propose transformation plans based on IR optimizations.

CLI usage:
```
proto-vm-cli designer-propose-refactors --workspace <workspace> --designer-session-id <session_id> --target <block|node> --passes <pass1,pass2,...>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-propose-refactors",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "target": "block",
    "passes": ["SimplifyAlgebraic", "FoldConstants"]
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-propose-refactors",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "plans": [
      { "id": "IR_T1", "kind": "SimplifyDoubleInversion", ... },
      { "id": "IR_T2", "kind": "SimplifyRedundantGate", ... }
    ]
  }
}
```

#### 24.2.7 `designer-apply-refactors`
Apply one or more transformation plans to the underlying branch.

CLI usage:
```
proto-vm-cli designer-apply-refactors --workspace <workspace> --designer-session-id <session_id> --user-id <user_id> --allow-unverified <true|false>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-apply-refactors",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "plans": [
      { ... full TransformationPlan object ... }
    ],
    "user_id": "ai-agent-1",
    "allow_unverified": false
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-apply-refactors",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... updated ... },
    "applied_plan_ids": ["IR_T1"],
    "new_circuit_revision": 42
  }
}
```

#### 24.2.8 `designer-diff`
Get before/after insight after changes, based on current focus.

CLI usage:
```
proto-vm-cli designer-diff --workspace <workspace> --designer-session-id <session_id> --compare-branch <branch_name> --include-behavior-diff <true|false> --include-ir-diff <true|false>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-diff",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "compare_branch": "main",
    "include_behavior_diff": true,
    "include_ir_diff": true
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-diff",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "behavior_diff": { ... BehaviorDiff ... },
    "ir_diff": { ... IrDiff ... }
  }
}
```

#### 24.2.9 `designer-codegen`
Generate human-readable code for the focus point.

CLI usage:
```
proto-vm-cli designer-codegen --workspace <workspace> --designer-session-id <session_id> --target <block|node> --flavor <PseudoVerilog|...> --use-optimized-ir <true|false>
```

Daemon request:
```json
{
  "id": "req1",
  "command": "designer-codegen",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "target": "block",
    "flavor": "PseudoVerilog",
    "use_optimized_ir": true
  }
}
```

Response:
```json
{
  "ok": true,
  "command": "designer-codegen",
  "error_code": null,
  "error": null,
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

### 24.3 Implementation Details

- **CoDesigner Module**: Central orchestration layer that coordinates existing ProtoVM analysis, optimization, transformation, diff, and codegen capabilities
- **Session Management**: In-memory session tracking without persistence beyond daemon lifetime
- **Integration with Existing Infrastructure**: Leverages CircuitFacade, BehavioralAnalysis, HlsIrInference, IrOptimization, Transformations, DiffAnalysis, and Codegen modules
- **CLI Wrappers**: Optional debugging commands available for manual testing
- **Daemon Protocol**: Full JSON-RPC support for AI client integration

### 24.4 Autonomous Playbooks (Phase 17)

The CoDesigner API is now extended with autonomous playbook capabilities that orchestrate multi-step design workflows as single high-level commands.

#### 24.4.1 Playbook Commands

##### designer-run-playbook

Execute a structured, multi-step design workflow in a single command:

**Request**:
```json
{
  "id": "req1",
  "command": "designer-run-playbook",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "designer_session_id": "cd-1234",
    "playbook_kind": "OptimizeAndApplySafeRefactors",
    "target": "block",
    "block_id": "B3",
    "baseline_branch": "main",
    "passes": ["SimplifyAlgebraic", "FoldConstants"],
    "use_optimized_ir": true,
    "apply_refactors": true
  }
}
```

**Response**:
```json
{
  "id": "req1",
  "ok": true,
  "command": "designer-run-playbook",
  "error_code": null,
  "error": null,
  "data": {
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
}
```