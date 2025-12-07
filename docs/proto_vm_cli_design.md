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
- CLI flags: `--workspace`, `--session-id`, `--ticks`, etc.
- JSON on stdin: For complex payloads in future extensions

### Available Commands

#### 2.1 `init-workspace`
- Purpose: Initialize a workspace directory structure
- Options:
  - `--workspace <path>` (required)
- Output:
  - Success: `{"ok": true, "workspace": "path", "created": true, "version": "0.1"}`
  - Error: `{"ok": false, "error": "message", "error_code": "CODE"}`

#### 2.2 `create-session`
- Purpose: Create a new simulation session
- Options:
  - `--workspace <path>` (required)
  - `--circuit-file <path>` (required for now)
- Output:
  - Success: `{"ok": true, "session_id": 1, "workspace": "path", "circuit_file": "path"}`

#### 2.3 `list-sessions`
- Purpose: List existing sessions in workspace
- Options:
  - `--workspace <path>` (required)
- Output:
  - Success: `{"ok": true, "sessions": [{"session_id": 1, "state": "ready", "circuit_file": "path", "created_at": "...", "last_used_at": "..."}]}`

#### 2.4 `run-ticks`
- Purpose: Advance simulation for a session by N ticks
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--ticks <N>` (required, default: 1)
- Output:
  - Success: `{"ok": true, "session_id": 1, "ticks_run": 100, "total_ticks": 1234}`

#### 2.5 `get-state`
- Purpose: Get current state of a session
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
- Output:
  - Success: `{"ok": true, "session_id": 1, "circuit_name": "...", "total_ticks": 1234, "breakpoints_hit": [], "traces": []}`

#### 2.6 `export-netlist`
- Purpose: Export netlist for a PCB in the session
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
  - `--pcb-id <id>` (optional, default: 0)
- Output:
  - Success: `{"ok": true, "session_id": 1, "pcb_id": 0, "netlist_file": "path/to/netlist.txt"}`

#### 2.7 `destroy-session`
- Purpose: Remove a session and its associated files
- Options:
  - `--workspace <path>` (required)
  - `--session-id <id>` (required)
- Output:
  - Success: `{"ok": true, "session_id": 1, "deleted": true}`

## 3. JSON Output Contract

All responses follow a consistent envelope pattern:

```json
{
  "ok": true,
  "data": { /* command-specific data */ }
}
```

Or in case of error:

```json
{
  "ok": false,
  "error": "Human-readable error message",
  "error_code": "MACHINE_PARSE_ERROR"
}
```

### Common Error Codes
- `WORKSPACE_NOT_FOUND`
- `INVALID_WORKSPACE`
- `SESSION_NOT_FOUND`
- `INVALID_SESSION_ID`
- `CIRCUIT_PARSE_ERROR`
- `MACHINE_INITIALIZATION_ERROR`
- `INVALID_ARGUMENT`

## 4. Session Model

### Conceptual Model
A session represents a running simulation instance with:
- One `Machine` object loaded with a circuit
- Persistent state (current tick count, signal traces, etc.)
- Associated artifacts (netlists, logs, etc.)

### On-disk Layout
```
workspace/
├── workspace.json                 # Workspace metadata
├── sessions/
│   └── <session_id>/
│       ├── session.json           # Session metadata
│       ├── circuit.circuit        # Copy of circuit file
│       ├── netlists/              # Exported netlists
│       ├── traces/                # Signal traces
│       └── logs/                  # Simulation logs
└── logs/                          # CLI logs
```

### Session Metadata Structure (session.json)
```json
{
  "session_id": 1,
  "created_at": "2025-01-01T12:34:56Z",
  "last_used_at": "2025-01-01T13:00:00Z",
  "circuit_file": "path/to/circuit.circuit",
  "state": "ready",                # created|ready|running|error
  "total_ticks": 0,
  "workspace": "/path/to/workspace"
}
```

## 5. Storage Abstraction

### Interface Definition
```cpp
class ISessionStore {
public:
    virtual ~ISessionStore() = default;
    
    virtual Result<int> CreateSession(const SessionCreateInfo& info) = 0;
    virtual Result<SessionMetadata> LoadSession(int session_id) = 0;
    virtual Result<bool> SaveSession(const SessionMetadata& metadata) = 0;
    virtual Result<std::vector<SessionMetadata>> ListSessions() = 0;
    virtual Result<bool> DeleteSession(int session_id) = 0;
    virtual Result<bool> UpdateSessionState(int session_id, const SessionState& state) = 0;
};
```

### Concrete Implementation
- `JsonFilesystemSessionStore`: File-based storage using JSON files
- `SqlSessionStore`: (Future) SQL-based storage (interface only for now)

## 6. Integration with Existing Engine

### Core Components Used
- `Machine`: Main simulation orchestrator
- `Pcb`: Circuit board representation
- `ElectricNodeBase`: Base class for all electronic components
- `CircuitSerializer`: For loading/saving circuit files

### Key Integration Points
- **Circuit Loading**: Use existing GUI serializers (`CircuitSerializer::LoadCircuit`) or core methods
- **Simulation Execution**: Call `Machine::Tick()` repeatedly for `run-ticks` command
- **Netlist Generation**: Use `Machine::GenerateNetlist()` for `export-netlist`
- **State Querying**: Access `Machine` and component state through public methods

### Assumptions and Limitations
- The `Machine` class can be instantiated and used independently from GUI components
- Circuit files in `.circuit` format can be loaded into a `Machine` instance
- Components maintain their state between simulation ticks
- Thread safety is not required for this initial implementation (single-threaded usage)

## 7. File Directory Structure

New source files will be added to:
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
    └── SessionTypes.h        # Type definitions for sessions
```

## 8. Build Integration

The new CLI will be built as an additional executable target in the existing CMake build system, linking against the core ProtoVM engine components. This preserves existing build functionality while adding the new CLI binary.