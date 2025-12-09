#include "EngineFacade.h"
#include "Machine.h"  // Include the main Machine class
#include "CircuitSerializer.h"  // Include the circuit serialization
#include "CircuitData.h"  // Include circuit data structures
#include "MachineSnapshot.h"  // Include the machine snapshot functionality
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace ProtoVMCLI {

// Helper function to generate ISO 8601 timestamp
std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::gmtime(&time_t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
}

// Implementation of CreateMachineFromCircuit
Result<std::unique_ptr<Machine>> EngineFacade::CreateMachineFromCircuit(const std::string& circuit_file) {
    try {
        // Use the CircuitSerializer to load circuit data from the file
        CircuitData circuitData;
        wxString wx_circuit_file(circuit_file.c_str());
        
        if (!CircuitSerializer::LoadCircuit(wx_circuit_file, circuitData)) {
            return Result<std::unique_ptr<Machine>>::MakeError(
                ErrorCode::CircuitFileUnreadable,
                "Failed to load circuit file: " + circuit_file
            );
        }

        // Create a new Machine and build it from the circuit data
        auto machine = std::make_unique<Machine>();
        
        // For now, we'll create a simplified approach to convert CircuitData to Machine
        // This requires more detailed implementation in the real system
        // For now, just return a basic machine - this is where we'd need to implement
        // proper conversion from CircuitData to machine components
        
        // Since the CircuitData to Machine conversion is complex and requires more code,
        // we'll implement a simplified approach for now
        // In practice, this would involve parsing the components and wires from CircuitData
        // and creating corresponding ElectricNodeBase objects in the Machine
        
        // For the purpose of this implementation, return a new machine with basic setup
        // until proper circuit loading is implemented
        machine->Init();  // Initialize the machine after loading circuit
        
        return Result<std::unique_ptr<Machine>>::MakeOk(std::move(machine));
    } catch (const std::exception& e) {
        return Result<std::unique_ptr<Machine>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in CreateMachineFromCircuit: ") + e.what()
        );
    }
}

// Implementation of SaveSnapshot
Result<bool> EngineFacade::SaveSnapshot(const Machine& machine, const std::string& snapshot_path) {
    try {
        // Create the snapshot directory if it doesn't exist
        fs::path snap_path(snapshot_path);
        fs::create_directories(snap_path.parent_path());

        // Use the MachineSnapshot class to serialize the machine state
        bool success = MachineSnapshot::SerializeToFile(machine, snapshot_path);

        if (!success) {
            return Result<bool>::MakeError(
                ErrorCode::StorageIoError,
                "Failed to serialize machine state to snapshot file: " + snapshot_path
            );
        }

        // Verify the file was written successfully
        if (!fs::exists(snapshot_path)) {
            return Result<bool>::MakeError(
                ErrorCode::StorageIoError,
                "Snapshot file was not created successfully: " + snapshot_path
            );
        }

        return Result<bool>::MakeOk(true);
    } catch (const std::exception& e) {
        return Result<bool>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in SaveSnapshot: ") + e.what()
        );
    }
}

// Implementation of LoadFromSnapshot
Result<std::unique_ptr<Machine>> EngineFacade::LoadFromSnapshot(const std::string& snapshot_path) {
    try {
        // Create a new Machine instance
        auto machine = std::make_unique<Machine>();

        // Use the MachineSnapshot class to deserialize the machine state
        bool success = MachineSnapshot::DeserializeFromFile(*machine, snapshot_path);

        if (!success) {
            return Result<std::unique_ptr<Machine>>::MakeError(
                ErrorCode::StorageIoError,
                "Failed to deserialize machine state from snapshot file: " + snapshot_path
            );
        }

        return Result<std::unique_ptr<Machine>>::MakeOk(std::move(machine));
    } catch (const std::exception& e) {
        return Result<std::unique_ptr<Machine>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in LoadFromSnapshot: ") + e.what()
        );
    }
}

// Implementation of GetLatestSnapshotFileInternal
std::string EngineFacade::GetLatestSnapshotFileInternal(const std::string& session_dir) {
    std::string snapshots_dir = session_dir + "/snapshots";
    
    // Create snapshots directory if it doesn't exist
    fs::create_directories(snapshots_dir);
    
    // Find the snapshot files with the highest sequence number
    std::vector<std::string> snapshot_files;
    
    for (const auto& entry : fs::directory_iterator(snapshots_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".bin") {
            snapshot_files.push_back(entry.path().string());
        }
    }
    
    // Sort snapshot files by their sequence number
    std::sort(snapshot_files.begin(), snapshot_files.end());
    
    if (!snapshot_files.empty()) {
        return snapshot_files.back();  // Return the last (highest numbered) snapshot
    }
    
    // No snapshots found
    return "";
}

// Implementation of public GetLatestSnapshotFile
std::string EngineFacade::GetLatestSnapshotFile(const std::string& session_dir) const {
    std::string snapshots_dir = session_dir + "/snapshots";

    // Create snapshots directory if it doesn't exist
    fs::create_directories(snapshots_dir);

    // Find the snapshot files with the highest sequence number
    std::vector<std::string> snapshot_files;

    for (const auto& entry : fs::directory_iterator(snapshots_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".bin") {
            snapshot_files.push_back(entry.path().string());
        }
    }

    // Sort snapshot files by their sequence number
    std::sort(snapshot_files.begin(), snapshot_files.end());

    if (!snapshot_files.empty()) {
        return snapshot_files.back();  // Return the last (highest numbered) snapshot
    }

    // No snapshots found
    return "";
}

// Implementation of CreateNewSnapshotPath
std::string EngineFacade::CreateNewSnapshotPath(const std::string& session_dir, int64 tick_count) {
    std::string snapshots_dir = session_dir + "/snapshots";
    
    // Create snapshots directory if it doesn't exist
    fs::create_directories(snapshots_dir);
    
    // Find the next available sequence number
    int next_seq = 1;
    std::string next_path = snapshots_dir + "/snapshot_" + 
                           std::string(8, '0').substr(0, 8 - std::to_string(next_seq).length()) + 
                           std::to_string(next_seq) + ".bin";
    
    // Find the next available snapshot file name
    while (fs::exists(next_path)) {
        next_seq++;
        next_path = snapshots_dir + "/snapshot_" + 
                   std::string(8, '0').substr(0, 8 - std::to_string(next_seq).length()) + 
                   std::to_string(next_seq) + ".bin";
    }
    
    return next_path;
}

// Implementation of InitializeNewSession
Result<EngineSnapshotInfo> EngineFacade::InitializeNewSession(
    SessionMetadata& session,
    const std::string& circuit_file,
    const std::string& session_dir
) {
    try {
        // Create a new Machine from the circuit file
        auto machine_result = CreateMachineFromCircuit(circuit_file);
        if (!machine_result.ok) {
            return Result<EngineSnapshotInfo>::MakeError(
                machine_result.error_code,
                machine_result.error_message
            );
        }
        
        auto machine = std::move(machine_result.data);
        
        // Create an initial snapshot for the machine
        std::string snapshot_path = session_dir + "/snapshots/snapshot_00000001.bin";
        auto save_result = SaveSnapshot(*machine, snapshot_path);
        
        if (!save_result.ok) {
            return Result<EngineSnapshotInfo>::MakeError(
                save_result.error_code,
                save_result.error_message
            );
        }
        
        // Create the EngineSnapshotInfo result
        EngineSnapshotInfo info;
        info.total_ticks = 0;  // Initial tick count is 0
        info.snapshot_file = snapshot_path;
        info.timestamp = GetCurrentTimestamp();
        
        return Result<EngineSnapshotInfo>::MakeOk(info);
    } catch (const std::exception& e) {
        return Result<EngineSnapshotInfo>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in InitializeNewSession: ") + e.what()
        );
    }
}

// Implementation of LoadFromLatestSnapshot
Result<EngineSnapshotInfo> EngineFacade::LoadFromLatestSnapshot(
    SessionMetadata& session,
    const std::string& session_dir,
    std::unique_ptr<Machine>& out_machine
) {
    try {
        // Check if the circuit has been modified since the last simulation snapshot
        if (session.circuit_revision != session.sim_revision) {
            // The circuit has changed since the last simulation snapshot, so we need
            // to rebuild the machine from the current circuit state
            std::string circuit_file = session.circuit_file;

            // Create a new machine from the original circuit file
            auto machine_result = CreateMachineFromCircuit(circuit_file);
            if (!machine_result.ok) {
                return Result<EngineSnapshotInfo>::MakeError(
                    machine_result.error_code,
                    "Failed to create machine from circuit after circuit change: " + machine_result.error_message
                );
            }

            out_machine = std::move(machine_result.data);

            // Create a new snapshot at tick 0 based on the current circuit
            std::string snapshot_path = CreateNewSnapshotPath(session_dir, 0);

            auto save_result = SaveSnapshot(*out_machine, snapshot_path);
            if (!save_result.ok) {
                return Result<EngineSnapshotInfo>::MakeError(
                    save_result.error_code,
                    save_result.error_message
                );
            }

            // Return that the machine was rebuilt and the new snapshot
            EngineSnapshotInfo info;
            info.total_ticks = 0;
            info.snapshot_file = snapshot_path;
            info.timestamp = GetCurrentTimestamp();

            return Result<EngineSnapshotInfo>::MakeOk(info);
        }

        // Circuit hasn't changed, proceed with loading the latest simulation snapshot
        std::string latest_snapshot = GetLatestSnapshotFileInternal(session_dir);
        if (latest_snapshot.empty()) {
            return Result<EngineSnapshotInfo>::MakeError(
                ErrorCode::StorageIoError,
                "No snapshots found in session directory: " + session_dir
            );
        }

        // Load the machine from the snapshot
        auto load_result = LoadFromSnapshot(latest_snapshot);
        if (!load_result.ok) {
            return Result<EngineSnapshotInfo>::MakeError(
                load_result.error_code,
                load_result.error_message
            );
        }

        out_machine = std::move(load_result.data);

        // Create the EngineSnapshotInfo result
        EngineSnapshotInfo info;
        info.total_ticks = out_machine->current_tick;
        info.snapshot_file = latest_snapshot;
        info.timestamp = GetCurrentTimestamp();

        return Result<EngineSnapshotInfo>::MakeOk(info);
    } catch (const std::exception& e) {
        return Result<EngineSnapshotInfo>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in LoadFromLatestSnapshot: ") + e.what()
        );
    }
}

// Implementation of RunTicksAndSnapshot
Result<EngineSnapshotInfo> EngineFacade::RunTicksAndSnapshot(
    SessionMetadata& session,
    std::unique_ptr<Machine>& machine,
    int ticks,
    const std::string& session_dir
) {
    try {
        // Run the requested number of ticks
        for (int i = 0; i < ticks; i++) {
            if (!machine->Tick()) {
                return Result<EngineSnapshotInfo>::MakeError(
                    ErrorCode::InternalError,
                    "Machine tick failed during execution"
                );
            }
        }

        // Create a new snapshot after running the ticks
        std::string snapshot_path = CreateNewSnapshotPath(session_dir, machine->current_tick);
        auto save_result = SaveSnapshot(*machine, snapshot_path);

        if (!save_result.ok) {
            return Result<EngineSnapshotInfo>::MakeError(
                save_result.error_code,
                save_result.error_message
            );
        }

        // Create the EngineSnapshotInfo result
        EngineSnapshotInfo info;
        info.total_ticks = machine->current_tick;
        info.snapshot_file = snapshot_path;
        info.timestamp = GetCurrentTimestamp();

        return Result<EngineSnapshotInfo>::MakeOk(info);
    } catch (const std::exception& e) {
        return Result<EngineSnapshotInfo>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in RunTicksAndSnapshot: ") + e.what()
        );
    }
}

// Implementation of ExportNetlist
Result<std::string> EngineFacade::ExportNetlist(
    SessionMetadata& session,
    std::unique_ptr<Machine>& machine,
    int pcb_id
) {
    try {
        // Generate the netlist using the machine's built-in method
        String netlist = machine->GenerateNetlist(pcb_id);
        
        // Create the netlists directory if it doesn't exist
        std::string session_netlist_dir = session.workspace + "/sessions/" + 
                                        std::to_string(session.session_id) + "/netlists";
        fs::create_directories(session_netlist_dir);
        
        // Create a filename for the netlist
        std::string netlist_file = session_netlist_dir + "/netlist_" + 
                                  std::to_string(pcb_id) + ".txt";
        
        // Write the netlist to a file
        std::ofstream file(netlist_file);
        if (!file.is_open()) {
            return Result<std::string>::MakeError(
                ErrorCode::StorageIoError,
                "Failed to open netlist file for writing: " + netlist_file
            );
        }
        
        file << netlist.ToStd();
        file.close();
        
        return Result<std::string>::MakeOk(netlist_file);
    } catch (const std::exception& e) {
        return Result<std::string>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ExportNetlist: ") + e.what()
        );
    }
}

// Implementation of QueryState
Result<EngineStateSummary> EngineFacade::QueryState(
    SessionMetadata& session,
    std::unique_ptr<Machine>& machine
) {
    try {
        EngineStateSummary summary;
        summary.total_ticks = machine->current_tick;
        
        // Extract circuit name from the session metadata
        std::string circuit_file = session.circuit_file;
        size_t last_slash = circuit_file.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            summary.circuit_name = circuit_file.substr(last_slash + 1);
        } else {
            summary.circuit_name = circuit_file;
        }
        
        // For now, set empty placeholders for breakpoints and traces
        // These would be filled with actual data in a complete implementation
        
        return Result<EngineStateSummary>::MakeOk(summary);
    } catch (const std::exception& e) {
        return Result<EngineStateSummary>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in QueryState: ") + e.what()
        );
    }
}

} // namespace ProtoVMCLI