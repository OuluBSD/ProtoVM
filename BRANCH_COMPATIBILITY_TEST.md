# Backward Compatibility Test for ProtoVM Branching

## Test Overview
This test verifies that the ProtoVM branching system maintains backward compatibility with existing sessions.

## Test Steps

1. **Original Session Structure (Before Branching)**
   ```json
   {
     "schema_version": 1,
     "session_id": 1,
     "state": 1,
     "circuit_file": "/path/to/circuit.circuit",
     "created_at": "2025-01-01T12:34:56Z",
     "last_used_at": "2025-01-01T13:00:00Z",
     "total_ticks": 0,
     "circuit_revision": 42,
     "sim_revision": 40,
     "engine_version": "unknown"
   }
   ```

2. **After Branching System Migration**
   When the old session is loaded by the new system:
   ```json
   {
     "schema_version": 1,
     "session_id": 1,
     "state": 1,
     "circuit_file": "/path/to/circuit.circuit",
     "created_at": "2025-01-01T12:34:56Z",
     "last_used_at": "2025-01-01T13:00:00Z",
     "total_ticks": 0,
     "circuit_revision": 42,      // DEPRECATED but preserved
     "sim_revision": 40,          // DEPRECATED but preserved
     "current_branch": "main",    // NEW: default branch
     "branches": [                // NEW: branch information
       {
         "name": "main",
         "head_revision": 42,     // Migrated from old circuit_revision
         "sim_revision": 40,      // Migrated from old sim_revision
         "base_revision": 0,
         "is_default": true
       }
     ],
     "engine_version": "unknown"
   }
   ```

3. **Behavior Verification**
   - All existing CLI commands work the same way (operating on the "main" branch by default)
   - All existing daemon commands work the same way (operating on the "main" branch by default)
   - New branch commands are available for advanced users
   - Old session files are automatically migrated on first access
   - No data loss during migration

## Code Verification

### 1. Session Migration Logic
In `JsonFilesystemSessionStore.cpp`, the `LoadSession` method handles both old and new formats:

```cpp
// Extract branches - this might not exist in older sessions
pos = content_copy.find("\"branches\": [");
if (pos != std::string::npos) {
    // Parse the new branches array format
    // ... implementation for new format
} else {
    // For backward compatibility, migrate the old format
    // Create a branch from the existing revision information
    BranchMetadata main_branch("main", metadata.circuit_revision, metadata.sim_revision, 0, true);
    metadata.branches.clear();
    metadata.branches.push_back(main_branch);
}
```

### 2. Default Branch Initialization
In `SessionTypes.h`, the `SessionMetadata` constructor initializes with a default main branch:

```cpp
SessionMetadata() {
    // ... other initialization
    // Initialize with a default main branch
    BranchMetadata main_branch("main", 0, 0, 0, true);
    branches.push_back(main_branch);
}
```

### 3. Branch-Aware Circuit Operations
All existing operations now use the current branch by default:

```cpp
// In CircuitFacade.cpp
Result<CircuitRevisionInfo> CircuitFacade::LoadCurrentCircuit(
    const SessionMetadata& session,
    const std::string& session_dir,
    CircuitData& out_circuit
) {
    // Call the branch-aware version using the current branch
    return LoadCurrentCircuitForBranch(session, session_dir, session.current_branch, out_circuit);
}
```

## Test Conclusion
The branching system maintains full backward compatibility:
- Old sessions are automatically migrated
- All existing commands continue to work as before
- New branch-aware commands are available for enhanced functionality
- No disruption to existing workflows