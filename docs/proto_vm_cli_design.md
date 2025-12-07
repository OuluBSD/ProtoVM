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

This provides a foundation for future multi-user collaboration features.

## 10. File Directory Structure

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
    └── EventLogger.cpp
```

## 11. Build Integration

The new CLI is built as an additional executable target in the existing CMake build system, linking against the core ProtoVM engine components. This preserves existing build functionality while adding the new CLI binary.