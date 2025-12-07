#ifndef _ProtoVM_EngineFacade_h_
#define _ProtoVM_EngineFacade_h_

#include "SessionTypes.h"
#include "ProtoVM.h"
#include <memory>
#include <string>

namespace ProtoVMCLI {

struct EngineSnapshotInfo {
    int64 total_ticks;
    std::string snapshot_file;       // path to the latest snapshot
    std::string timestamp;           // ISO 8601 timestamp
};

struct EngineStateSummary {
    int64 total_ticks;
    std::string circuit_name;
    std::vector<std::string> breakpoints; // can be empty
    std::vector<std::string> traces;      // can be empty
    // optional: basic signals/stats
};

class Machine;  // Forward declaration

class EngineFacade {
public:
    // Initialize Machine for a new session from a circuit file,
    // create initial snapshot.
    Result<EngineSnapshotInfo> InitializeNewSession(
        SessionMetadata& session,
        const std::string& circuit_file,
        const std::string& session_dir
    );

    // Load a Machine from the latest snapshot for an existing session.
    Result<EngineSnapshotInfo> LoadFromLatestSnapshot(
        SessionMetadata& session,
        const std::string& session_dir,
        std::unique_ptr<Machine>& out_machine
    );

    // Run N ticks, update Machine and return new snapshot info.
    Result<EngineSnapshotInfo> RunTicksAndSnapshot(
        SessionMetadata& session,
        std::unique_ptr<Machine>& machine,
        int ticks,
        const std::string& session_dir
    );

    // Export netlist for a given PCB; may or may not need a live Machine
    // depending on architecture.
    Result<std::string> ExportNetlist(
        SessionMetadata& session,
        std::unique_ptr<Machine>& machine,
        int pcb_id
    );

    // Query summarized state from a live Machine.
    Result<EngineStateSummary> QueryState(
        SessionMetadata& session,
        std::unique_ptr<Machine>& machine
    );
    
private:
    // Helper method to create a Machine from a circuit file
    Result<std::unique_ptr<Machine>> CreateMachineFromCircuit(const std::string& circuit_file);

    // Helper method to save a snapshot of a Machine
    Result<bool> SaveSnapshot(const Machine& machine, const std::string& snapshot_path);

    // Helper method to load a Machine from a snapshot
    Result<std::unique_ptr<Machine>> LoadFromSnapshot(const std::string& snapshot_path);

    // Helper method to get the latest snapshot file path (internal use)
    std::string GetLatestSnapshotFileInternal(const std::string& session_dir);

    // Helper method to create a new snapshot file path
    std::string CreateNewSnapshotPath(const std::string& session_dir, int64 tick_count);

public:
    // Public method to get the latest snapshot file path (needed for queries)
    std::string GetLatestSnapshotFile(const std::string& session_dir) const;
};

} // namespace ProtoVMCLI

#endif