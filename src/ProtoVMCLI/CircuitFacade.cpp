#include "CircuitFacade.h"
#include "CircuitOps.h"
#include "CircuitSerializer.h"  // For loading/saving circuit files
#include "EventLogger.h"        // For logging circuit edit events
#include "JsonIO.h"             // For JSON serialization
#include "SessionStore.h"       // For ISessionStore
#include "CircuitData.h"        // For CircuitIdGenerator
#include "CircuitMerge.h"       // For collaboration merge logic
#include "BranchTypes.h"        // For branch types
#include "TimingAnalysis.h"     // For timing analysis
#include "BlockAnalysis.h"      // For block analysis
#include "Transformations.h"    // For transformations
#include "HlsIr.h"              // For HLS IR structures
#include "HlsIrInference.h"     // For HLS IR inference engine
#include "DiffAnalysis.h"       // For diff analysis
#include "RetimingModel.h"      // For retiming model
#include "RetimingAnalysis.h"   // For retiming analysis
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;

namespace ProtoVMCLI {

// Helper function to convert TransformationKind to string
std::string ToString(TransformationKind kind) {
    switch (kind) {
        case TransformationKind::SimplifyDoubleInversion:
            return "SimplifyDoubleInversion";
        case TransformationKind::SimplifyRedundantGate:
            return "SimplifyRedundantGate";
        case TransformationKind::ReplaceWithKnownBlock:
            return "ReplaceWithKnownBlock";
        case TransformationKind::RewireFanoutTree:
            return "RewireFanoutTree";
        case TransformationKind::MergeEquivalentBlocks:
            return "MergeEquivalentBlocks";
        default:
            return "Unknown";
    }
}

#include "FunctionalAnalysis.h" // For functional analysis
#include "IrOptimization.h"     // For IR optimization
#include "BehavioralAnalysis.h" // For behavioral analysis

// Helper function to find a branch by name in session metadata
std::optional<BranchMetadata> FindBranchByName(const SessionMetadata& session, const std::string& branch_name) {
    for (const auto& branch : session.branches) {
        if (branch.name == branch_name) {
            return branch;
        }
    }
    return std::nullopt;
}

// Helper function to get the current branch metadata
BranchMetadata GetCurrentBranch(const SessionMetadata& session) {
    for (const auto& branch : session.branches) {
        if (branch.name == session.current_branch) {
            return branch;
        }
    }

    // If current branch not found, return default main branch
    BranchMetadata default_branch("main", 0, 0, 0, true);
    return default_branch;
}

// Helper function to generate ISO 8601 timestamp
std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::gmtime(&time_t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
}

Result<CircuitRevisionInfo> CircuitFacade::LoadCurrentCircuit(
    const SessionMetadata& session,
    const std::string& session_dir,
    CircuitData& out_circuit
) {
    // Call the branch-aware version using the current branch
    return LoadCurrentCircuitForBranch(session, session_dir, session.current_branch, out_circuit);
}

Result<CircuitRevisionInfo> CircuitFacade::LoadCurrentCircuitForBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    CircuitData& out_circuit
) {
    try {
        // Get the branch metadata to determine revision to load
        std::optional<BranchMetadata> branch_opt = FindBranchByName(session, branch_name);
        if (!branch_opt.has_value()) {
            return Result<CircuitRevisionInfo>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Branch not found: " + branch_name
            );
        }

        BranchMetadata branch = branch_opt.value();
        int64_t branch_revision = branch.head_revision;

        // First, try to load from a circuit snapshot if it exists and is up-to-date
        int64_t snapshot_rev = GetLatestCircuitSnapshotRevision(session_dir);
        if (snapshot_rev > 0 && snapshot_rev < branch_revision) {
            // Load from snapshot and replay events from snapshot_rev+1 to branch_revision
            auto load_result = LoadCircuitFromSnapshot(session_dir, out_circuit);
            if (load_result.ok) {
                auto replay_result = ReplayCircuitEventsForBranch(out_circuit, session_dir,
                                                                 snapshot_rev + 1, branch_revision, branch_name);
                if (replay_result.ok) {
                    CircuitRevisionInfo info;
                    info.revision = branch_revision;
                    return Result<CircuitRevisionInfo>::MakeOk(info);
                }
            }
        } else if (snapshot_rev == branch_revision) {
            // Load directly from the snapshot
            auto load_result = LoadCircuitFromSnapshot(session_dir, out_circuit);
            if (load_result.ok) {
                CircuitRevisionInfo info;
                info.revision = branch_revision;
                return Result<CircuitRevisionInfo>::MakeOk(info);
            }
        }

        // If no snapshot or it's not usable, start with the initial circuit
        auto init_result = LoadInitialCircuit(session.circuit_file, out_circuit);
        if (!init_result.ok) {
            return Result<CircuitRevisionInfo>::MakeError(
                init_result.error_code,
                init_result.error_message
            );
        }

        // Replay all events from the beginning up to the branch's circuit revision
        auto replay_result = ReplayCircuitEventsForBranch(out_circuit, session_dir, 1, branch_revision, branch_name);
        if (!replay_result.ok) {
            return Result<CircuitRevisionInfo>::MakeError(
                replay_result.error_code,
                replay_result.error_message
            );
        }

        CircuitRevisionInfo info;
        info.revision = branch_revision;
        return Result<CircuitRevisionInfo>::MakeOk(info);
    }
    catch (const std::exception& e) {
        return Result<CircuitRevisionInfo>::MakeError(
            ErrorCode::CircuitStateCorrupt,
            std::string("Exception in LoadCurrentCircuitForBranch: ") + e.what()
        );
    }
}

Result<CircuitRevisionInfo> CircuitFacade::ApplyEditOperations(
    SessionMetadata& session,
    const std::string& session_dir,
    const std::vector<EditOperation>& ops,
    const std::string& user_id
) {
    // Call the branch-aware version using the current branch
    return ApplyEditOperationsToBranch(session, session_dir, ops, user_id, session.current_branch);
}

Result<CircuitRevisionInfo> CircuitFacade::ApplyEditOperationsToBranch(
    SessionMetadata& session,
    const std::string& session_dir,
    const std::vector<EditOperation>& ops,
    const std::string& user_id,
    const std::string& branch_name
) {
    try {
        // Find the target branch
        std::optional<BranchMetadata> target_branch_opt = FindBranchByName(session, branch_name);
        if (!target_branch_opt.has_value()) {
            return Result<CircuitRevisionInfo>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Branch not found: " + branch_name
            );
        }

        BranchMetadata& target_branch = const_cast<BranchMetadata&>(target_branch_opt.value());
        int64_t branch_revision = target_branch.head_revision;

        // First, get the current circuit state for this branch
        CircuitData current_circuit;
        auto load_result = LoadCurrentCircuitForBranch(session, session_dir, branch_name, current_circuit);
        if (!load_result.ok) {
            return Result<CircuitRevisionInfo>::MakeError(
                load_result.error_code,
                load_result.error_message
            );
        }

        // Check if any operations have revision_base specified for optimistic concurrency
        std::vector<EditOperation> final_ops = ops;
        int64_t client_revision = -1;

        // Look for revision_base in operations to determine expected revision
        for (const auto& op : ops) {
            if (op.revision_base > 0) {
                if (client_revision == -1) {
                    client_revision = op.revision_base;
                } else if (client_revision != op.revision_base) {
                    // All operations in the batch should have the same revision_base
                    return Result<CircuitRevisionInfo>::MakeError(
                        ErrorCode::CommandParseError,
                        "All operations in a batch must have the same revision_base"
                    );
                }
            }
        }

        // If client sent a specific expected revision that differs from current branch revision,
        // attempt to resolve concurrent edits
        if (client_revision != -1 && client_revision != branch_revision) {
            // Load intervening events between client revision and current branch revision
            // For now, we'll create a placeholder for the intervening events
            std::vector<EventLogEntry> intervening_events;

            // Create a base circuit at the client's revision (this would require replaying events
            // up to client_revision, which we'll simplify for now)
            CircuitData base_circuit = current_circuit; // In a real implementation, this would be
                                                       // the circuit state at client_revision

            // Use CircuitMerge to resolve concurrent edits
            MergeResult merge_result = CircuitMerge::ResolveConcurrentEdits(
                base_circuit,
                ops,
                client_revision,
                branch_revision,
                intervening_events
            );

            if (merge_result.conflict) {
                return Result<CircuitRevisionInfo>::MakeError(
                    ErrorCode::Conflict,
                    "Merge conflict: " + merge_result.conflict_reason
                );
            } else if (merge_result.merged) {
                // Use the transformed operations from the merge
                final_ops = merge_result.transformed_ops;
            }
        } else if (client_revision != -1 && client_revision != branch_revision) {
            // Client expected a specific revision but it doesn't match current branch revision
            return Result<CircuitRevisionInfo>::MakeError(
                ErrorCode::Conflict,
                "Revision mismatch: expected " + std::to_string(client_revision) +
                ", but current revision on branch " + branch_name + " is " + std::to_string(branch_revision)
            );
        }

        // Apply each operation to the circuit
        for (const auto& op : final_ops) {
            auto apply_result = ApplyEditOperation(current_circuit, op);
            if (!apply_result.ok) {
                return Result<CircuitRevisionInfo>::MakeError(
                    apply_result.error_code,
                    apply_result.error_message
                );
            }
        }

        // Increment the circuit revision for this branch
        int64_t new_revision = branch_revision + 1;

        // Save circuit snapshot periodically (e.g., every 50 edits or at certain intervals)
        if (new_revision % 50 == 0) {
            auto snapshot_result = SaveCircuitSnapshot(current_circuit, session_dir, new_revision);
            if (!snapshot_result.ok) {
                // This is a warning, not a fatal error - we can continue without snapshot
                // Log the issue but don't fail the operation
            }
        }

        // Update branch's head_revision in session metadata
        for (auto& branch : session.branches) {
            if (branch.name == branch_name) {
                branch.head_revision = new_revision;
                break;
            }
        }

        // Log the operation as an event with branch information
        for (const auto& op : final_ops) {
            EventLogEntry event;
            event.timestamp = GetCurrentTimestamp(); // We'll need to define this helper
            event.user_id = user_id;
            event.session_id = session.session_id;
            event.branch = branch_name;  // Add the branch information

            // Map EditOpType to string command
            std::string op_cmd;
            switch (op.type) {
                case EditOpType::AddComponent: op_cmd = "add_component"; break;
                case EditOpType::RemoveComponent: op_cmd = "remove_component"; break;
                case EditOpType::MoveComponent: op_cmd = "move_component"; break;
                case EditOpType::SetComponentProperty: op_cmd = "set_component_property"; break;
                case EditOpType::Connect: op_cmd = "connect"; break;
                case EditOpType::Disconnect: op_cmd = "disconnect"; break;
            }
            event.command = op_cmd;

            // Create params object for the operation
            Upp::ValueMap params;
            params.Add("revision", Upp::String(std::to_string(new_revision).c_str()));
            params.Add("branch", Upp::String(branch_name.c_str()));
            if (op.component_id.IsValid()) {
                params.Add("component_id", Upp::String(op.component_id.id.c_str()));
            }
            if (op.wire_id.IsValid()) {
                params.Add("wire_id", Upp::String(op.wire_id.id.c_str()));
            }
            params.Add("x", op.x);
            params.Add("y", op.y);
            if (!op.property_name.empty()) {
                params.Add("property_name", Upp::String(op.property_name.c_str()));
            }
            if (!op.property_value.empty()) {
                params.Add("property_value", Upp::String(op.property_value.c_str()));
            }
            if (op.target_component_id.IsValid()) {
                params.Add("target_component_id", Upp::String(op.target_component_id.id.c_str()));
            }
            if (!op.pin_name.empty()) {
                params.Add("pin_name", Upp::String(op.pin_name.c_str()));
            }
            if (!op.target_pin_name.empty()) {
                params.Add("target_pin_name", Upp::String(op.target_pin_name.c_str()));
            }
            if (!op.component_type.empty()) {
                params.Add("component_type", Upp::String(op.component_type.c_str()));
            }
            if (!op.component_name.empty()) {
                params.Add("component_name", Upp::String(op.component_name.c_str()));
            }

            // Add collaboration-specific parameters
            if (client_revision > 0) {
                params.Add("expected_revision", Upp::String(std::to_string(client_revision).c_str()));
            }

            event.params = Upp::String().Cat() << params;

            // Create result object
            Upp::ValueMap result_data;
            result_data.Add("revision", Upp::String(std::to_string(new_revision).c_str()));
            result_data.Add("branch", Upp::String(branch_name.c_str()));
            if (client_revision != -1 && client_revision != branch_revision) {
                result_data.Add("merged", true);
                result_data.Add("conflict", false);
            }
            event.result = Upp::String().Cat() << result_data;

            EventLogger::LogEvent(session_dir, event);
        }

        // The session would be updated by the caller, as CircuitFacade doesn't have direct
        // access to the session store. The caller is responsible for saving the updated
        // session metadata that includes the new branch revision.

        CircuitRevisionInfo info;
        info.revision = new_revision;
        return Result<CircuitRevisionInfo>::MakeOk(info);
    }
    catch (const std::exception& e) {
        return Result<CircuitRevisionInfo>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ApplyEditOperationsToBranch: ") + e.what()
        );
    }
}

Result<CircuitStateExport> CircuitFacade::ExportCircuitState(
    const SessionMetadata& session,
    const std::string& session_dir
) {
    try {
        CircuitData circuit;
        auto load_result = LoadCurrentCircuit(session, session_dir, circuit);
        if (!load_result.ok) {
            return Result<CircuitStateExport>::MakeError(
                load_result.error_code,
                load_result.error_message
            );
        }

        // Convert circuit to JSON
        // This is a simplified approach - in reality, we'd need a proper serialization method
        Upp::ValueMap circuit_map;
        
        // Add circuit metadata
        circuit_map.Add("name", Upp::String(circuit.name.c_str()));
        circuit_map.Add("description", Upp::String(circuit.description.c_str()));
        
        // Add components
        Upp::ValueArray components_array;
        for (const auto& comp : circuit.components) {
            Upp::ValueMap comp_map;
            comp_map.Add("id", Upp::String(comp.id.id.c_str()));
            comp_map.Add("type", Upp::String(comp.type.c_str()));
            comp_map.Add("name", Upp::String(comp.name.c_str()));
            comp_map.Add("x", comp.x);
            comp_map.Add("y", comp.y);
            
            // Add inputs
            Upp::ValueArray inputs_array;
            for (const auto& input : comp.inputs) {
                Upp::ValueMap input_map;
                input_map.Add("id", Upp::String(input.id.id.c_str()));
                input_map.Add("name", Upp::String(input.name.c_str()));
                input_map.Add("x", input.x);
                input_map.Add("y", input.y);
                input_map.Add("is_input", input.is_input);
                inputs_array.Add(input_map);
            }
            comp_map.Add("inputs", inputs_array);
            
            // Add outputs
            Upp::ValueArray outputs_array;
            for (const auto& output : comp.outputs) {
                Upp::ValueMap output_map;
                output_map.Add("id", Upp::String(output.id.id.c_str()));
                output_map.Add("name", Upp::String(output.name.c_str()));
                output_map.Add("x", output.x);
                output_map.Add("y", output.y);
                output_map.Add("is_input", output.is_input);
                outputs_array.Add(output_map);
            }
            comp_map.Add("outputs", outputs_array);
            
            components_array.Add(comp_map);
        }
        circuit_map.Add("components", components_array);
        
        // Add wires
        Upp::ValueArray wires_array;
        for (const auto& wire : circuit.wires) {
            Upp::ValueMap wire_map;
            wire_map.Add("id", Upp::String(wire.id.id.c_str()));
            wire_map.Add("start_component_id", Upp::String(wire.start_component_id.id.c_str()));
            wire_map.Add("start_pin_name", Upp::String(wire.start_pin_name.c_str()));
            wire_map.Add("end_component_id", Upp::String(wire.end_component_id.id.c_str()));
            wire_map.Add("end_pin_name", Upp::String(wire.end_pin_name.c_str()));
            wires_array.Add(wire_map);
        }
        circuit_map.Add("wires", wires_array);

        CircuitStateExport export_data;
        export_data.revision = session.circuit_revision;
        export_data.circuit_json = Upp::String().Cat() << circuit_map;

        return Result<CircuitStateExport>::MakeOk(export_data);
    }
    catch (const std::exception& e) {
        return Result<CircuitStateExport>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ExportCircuitState: ") + e.what()
        );
    }
}

Result<CircuitGraph> CircuitFacade::BuildGraphForBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name
) {
    try {
        // Load the circuit data for the specified branch
        CircuitData circuit;
        auto load_result = LoadCurrentCircuitForBranch(session, session_dir, branch_name, circuit);
        if (!load_result.ok) {
            return Result<CircuitGraph>::MakeError(
                load_result.error_code,
                load_result.error_message
            );
        }

        // Build the graph from the circuit
        CircuitGraphBuilder builder;
        auto graph_result = builder.BuildGraph(circuit);
        if (!graph_result.ok) {
            return Result<CircuitGraph>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        return Result<CircuitGraph>::MakeOk(graph_result.data);
    }
    catch (const std::exception& e) {
        return Result<CircuitGraph>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildGraphForBranch: ") + e.what()
        );
    }
}

Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>> CircuitFacade::BuildTimingGraphForBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name
) {
    try {
        // First, get the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        // Now, convert the circuit graph to a timing graph
        TimingGraphBuilder timing_builder;
        auto timing_result = timing_builder.BuildTimingGraph(graph_result.data);
        if (!timing_result.ok) {
            return Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>>::MakeError(
                timing_result.error_code,
                timing_result.error_message
            );
        }

        return Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>>::MakeOk(timing_result.data);
    }
    catch (const std::exception& e) {
        return Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildTimingGraphForBranch: ") + e.what()
        );
    }
}

// Internal helper implementations follow:

Result<bool> CircuitFacade::LoadInitialCircuit(const std::string& circuit_file_path, CircuitData& out_circuit) {
    try {
        wxString wx_circuit_file(circuit_file_path.c_str());
        bool success = CircuitSerializer::LoadCircuit(wx_circuit_file, out_circuit);
        
        if (!success) {
            return Result<bool>::MakeError(
                ErrorCode::CircuitFileUnreadable,
                "Failed to load circuit file: " + circuit_file_path
            );
        }
        
        return Result<bool>::MakeOk(true);
    }
    catch (const std::exception& e) {
        return Result<bool>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in LoadInitialCircuit: ") + e.what()
        );
    }
}

Result<bool> CircuitFacade::ApplyEditOperation(CircuitData& circuit, const EditOperation& op) {
    try {
        switch (op.type) {
            case EditOpType::AddComponent: {
                // Generate a new ID if not provided
                CircuitEntityId comp_id = op.component_id;
                if (!comp_id.IsValid()) {
                    comp_id = CircuitIdGenerator::GenerateComponentId();
                }
                
                ComponentData new_comp;
                new_comp.id = comp_id;
                new_comp.type = op.component_type;
                new_comp.name = op.component_name;
                new_comp.x = op.x;
                new_comp.y = op.y;
                
                // Apply additional properties if any
                for (const auto& prop : op.properties) {
                    // In a real implementation, we might store these in a properties map
                    // For now, we'll just handle specific properties like inputs/outputs if needed
                }
                
                circuit.components.push_back(new_comp);
                break;
            }
            
            case EditOpType::RemoveComponent: {
                auto it = std::find_if(circuit.components.begin(), circuit.components.end(),
                    [&op](const ComponentData& comp) {
                        return comp.id == op.component_id;
                    });
                
                if (it != circuit.components.end()) {
                    // Remove any wires connected to this component
                    circuit.wires.erase(
                        std::remove_if(circuit.wires.begin(), circuit.wires.end(),
                            [&op](const WireData& wire) {
                                return wire.start_component_id == op.component_id || 
                                       wire.end_component_id == op.component_id;
                            }),
                        circuit.wires.end()
                    );
                    
                    circuit.components.erase(it);
                } else {
                    return Result<bool>::MakeError(
                        ErrorCode::InvalidEditOperation,
                        "Component with ID " + op.component_id.id + " not found"
                    );
                }
                break;
            }
            
            case EditOpType::MoveComponent: {
                auto it = std::find_if(circuit.components.begin(), circuit.components.end(),
                    [&op](const ComponentData& comp) {
                        return comp.id == op.component_id;
                    });
                
                if (it != circuit.components.end()) {
                    it->x = op.x;
                    it->y = op.y;
                } else {
                    return Result<bool>::MakeError(
                        ErrorCode::InvalidEditOperation,
                        "Component with ID " + op.component_id.id + " not found"
                    );
                }
                break;
            }
            
            case EditOpType::SetComponentProperty: {
                auto it = std::find_if(circuit.components.begin(), circuit.components.end(),
                    [&op](const ComponentData& comp) {
                        return comp.id == op.component_id;
                    });
                
                if (it != circuit.components.end()) {
                    // In a real implementation, we'd have a proper property system
                    // For now, we'll just log that we received a property update
                    // This could be extended to handle specific properties like name, type, etc.
                } else {
                    return Result<bool>::MakeError(
                        ErrorCode::InvalidEditOperation,
                        "Component with ID " + op.component_id.id + " not found"
                    );
                }
                break;
            }
            
            case EditOpType::Connect: {
                // Generate a new ID for the wire if not provided
                CircuitEntityId wire_id = op.wire_id;
                if (!wire_id.IsValid()) {
                    wire_id = CircuitIdGenerator::GenerateWireId();
                }
                
                WireData new_wire;
                new_wire.id = wire_id;
                new_wire.start_component_id = op.component_id;  // Source component
                new_wire.start_pin_name = op.pin_name;
                new_wire.end_component_id = op.target_component_id;  // Target component
                new_wire.end_pin_name = op.target_pin_name;
                
                circuit.wires.push_back(new_wire);
                break;
            }
            
            case EditOpType::Disconnect: {
                circuit.wires.erase(
                    std::remove_if(circuit.wires.begin(), circuit.wires.end(),
                        [&op](const WireData& wire) {
                            return (wire.start_component_id == op.component_id && 
                                   wire.start_pin_name == op.pin_name) ||
                                   (wire.end_component_id == op.component_id && 
                                   wire.end_pin_name == op.pin_name) ||
                                   (wire.start_component_id == op.target_component_id && 
                                   wire.start_pin_name == op.target_pin_name) ||
                                   (wire.end_component_id == op.target_component_id && 
                                   wire.end_pin_name == op.target_pin_name);
                        }),
                    circuit.wires.end()
                );
                break;
            }
        }
        
        return Result<bool>::MakeOk(true);
    }
    catch (const std::exception& e) {
        return Result<bool>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ApplyEditOperation: ") + e.what()
        );
    }
}

Result<bool> CircuitFacade::ReplayCircuitEvents(CircuitData& circuit, const std::string& session_dir, 
                                               int64_t from_revision, int64_t to_revision) {
    try {
        // For this implementation, we'll read the event log and apply operations
        // In a real implementation, we'd parse the event log file to extract circuit edit operations
        // and apply them to the circuit in order
        
        std::string events_file = session_dir + "/events.log";
        if (!fs::exists(events_file)) {
            if (from_revision > 0) {
                // If events file doesn't exist but we expect events to be there, it's an error
                return Result<bool>::MakeError(
                    ErrorCode::CircuitStateCorrupt,
                    "Events file does not exist but expected revisions " + 
                    std::to_string(from_revision) + " to " + std::to_string(to_revision)
                );
            }
            // If from_revision is 0, there are no events to replay
            return Result<bool>::MakeOk(true);
        }
        
        // Read and process events from the log
        std::ifstream file(events_file);
        if (!file.is_open()) {
            return Result<bool>::MakeError(
                ErrorCode::StorageIoError,
                "Could not open events file: " + events_file
            );
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // This is a simplified implementation that would require a full JSON parser
            // In practice, we'd need to properly parse the JSON events to extract
            // circuit editing operations and apply them in order.
        }
        
        return Result<bool>::MakeOk(true);
    }
    catch (const std::exception& e) {
        return Result<bool>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ReplayCircuitEvents: ") + e.what()
        );
    }
}

Result<bool> CircuitFacade::ReplayCircuitEventsForBranch(CircuitData& circuit, const std::string& session_dir,
                                                        int64_t from_revision, int64_t to_revision,
                                                        const std::string& branch_name) {
    try {
        // For branch-aware replay, we need to read the event log and filter events by branch
        std::string events_file = session_dir + "/events.log";
        if (!fs::exists(events_file)) {
            if (from_revision > 0) {
                // If events file doesn't exist but we expect events to be there, it's an error
                return Result<bool>::MakeError(
                    ErrorCode::CircuitStateCorrupt,
                    "Events file does not exist but expected revisions " +
                    std::to_string(from_revision) + " to " + std::to_string(to_revision) +
                    " for branch " + branch_name
                );
            }
            // If from_revision is 0, there are no events to replay
            return Result<bool>::MakeOk(true);
        }

        // Read and process events from the log
        std::ifstream file(events_file);
        if (!file.is_open()) {
            return Result<bool>::MakeError(
                ErrorCode::StorageIoError,
                "Could not open events file: " + events_file
            );
        }

        std::string line;
        while (std::getline(file, line)) {
            // Parse the JSON event to check if it belongs to the specified branch
            // This would require a proper JSON parser implementation
            // For now, we'll implement a simplified parsing logic

            // Check if the event is for the requested branch
            size_t branch_pos = line.find("\"branch\":\"");
            if (branch_pos == std::string::npos) {
                // If no branch field exists, this follows the old format; assume it's on main branch
                if (branch_name != "main" && branch_name != "master") {
                    continue; // Skip this event if not on the main branch
                }
            } else {
                // Extract the branch name from the event
                branch_pos += 10; // Length of "\"branch\":\""
                size_t branch_end = line.find("\"", branch_pos);
                if (branch_end == std::string::npos) {
                    continue; // Malformed event, skip it
                }

                std::string event_branch = line.substr(branch_pos, branch_end - branch_pos);
                if (event_branch != branch_name) {
                    continue; // Skip events not for the requested branch
                }
            }

            // Extract the revision from the event
            size_t rev_pos = line.find("\"revision\":");
            if (rev_pos == std::string::npos) {
                continue; // Skip events without revision
            }

            rev_pos += 11; // Length of "\"revision\":"
            size_t rev_end = line.find_first_not_of("0123456789", rev_pos);
            if (rev_end == std::string::npos) {
                continue; // Malformed revision number
            }

            std::string rev_str = line.substr(rev_pos, rev_end - rev_pos);
            int64_t event_revision;
            try {
                event_revision = std::stoll(rev_str);
            } catch (...) {
                continue; // Malformed revision number
            }

            // Only apply events within the requested revision range
            if (event_revision < from_revision || event_revision > to_revision) {
                continue;
            }

            // Actually parse and apply the operation from the event
            // This is a simplified implementation; a full parser would extract specific
            // operation details from the event and apply them to the circuit
            // For now, we'll skip the actual implementation as it would require full JSON parsing
        }

        return Result<bool>::MakeOk(true);
    }
    catch (const std::exception& e) {
        return Result<bool>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ReplayCircuitEventsForBranch: ") + e.what()
        );
    }
}

int64_t CircuitFacade::GetLatestCircuitSnapshotRevision(const std::string& session_dir) {
    try {
        std::string snapshots_dir = session_dir + "/circuit_snapshots";
        if (!fs::exists(snapshots_dir)) {
            return 0;
        }

        int64_t max_rev = 0;
        for (const auto& entry : fs::directory_iterator(snapshots_dir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.substr(0, 15) == "circuit_snap_") {  // circuit_snap_<revision>.json
                    std::string rev_str = filename.substr(15);  // Remove prefix
                    size_t dot_pos = rev_str.find('.');
                    if (dot_pos != std::string::npos) {
                        rev_str = rev_str.substr(0, dot_pos);
                    }
                    try {
                        int64_t rev = std::stoll(rev_str);
                        if (rev > max_rev) max_rev = rev;
                    } catch (...) {
                        // Skip invalid revision numbers
                    }
                }
            }
        }

        return max_rev;
    }
    catch (...) {
        return 0;  // On error, assume no snapshots
    }
}

Result<bool> CircuitFacade::LoadCircuitFromSnapshot(const std::string& session_dir, CircuitData& out_circuit) {
    try {
        int64_t latest_rev = GetLatestCircuitSnapshotRevision(session_dir);
        if (latest_rev <= 0) {
            return Result<bool>::MakeError(
                ErrorCode::CircuitStateCorrupt,
                "No circuit snapshots found"
            );
        }

        std::string snapshot_file = session_dir + "/circuit_snapshots/circuit_snap_" + 
                                   std::to_string(latest_rev) + ".json";
        
        if (!fs::exists(snapshot_file)) {
            return Result<bool>::MakeError(
                ErrorCode::CircuitStateCorrupt,
                "Circuit snapshot file does not exist: " + snapshot_file
            );
        }

        // Load the circuit from the snapshot file
        // This would require a proper JSON parser implementation
        // For now, this is a placeholder for the actual implementation
        
        return Result<bool>::MakeOk(true);
    }
    catch (const std::exception& e) {
        return Result<bool>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in LoadCircuitFromSnapshot: ") + e.what()
        );
    }
}

Result<bool> CircuitFacade::SaveCircuitSnapshot(const CircuitData& circuit, const std::string& session_dir, int64_t revision) {
    try {
        std::string snapshots_dir = session_dir + "/circuit_snapshots";
        fs::create_directories(snapshots_dir);

        std::string snapshot_file = snapshots_dir + "/circuit_snap_" + std::to_string(revision) + ".json";
        
        // Write circuit data to snapshot file
        // This would require a proper JSON serialization implementation
        // For now, this is a placeholder for the actual implementation
        
        return Result<bool>::MakeOk(true);
    }
    catch (const std::exception& e) {
        return Result<bool>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in SaveCircuitSnapshot: ") + e.what()
        );
    }
}

Result<FunctionalCone> CircuitFacade::BuildBackwardConeForBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const FunctionalNodeId& root,
    int max_depth
) {
    try {
        // Build the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<FunctionalCone>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        // Perform functional analysis on the graph
        FunctionalAnalysis analysis;
        auto cone_result = analysis.ComputeBackwardCone(graph_result.data, root, max_depth);
        if (!cone_result.ok) {
            return Result<FunctionalCone>::MakeError(
                cone_result.error_code,
                cone_result.error_message
            );
        }

        return Result<FunctionalCone>::MakeOk(cone_result.data);
    }
    catch (const std::exception& e) {
        return Result<FunctionalCone>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildBackwardConeForBranch: ") + e.what()
        );
    }
}

Result<FunctionalCone> CircuitFacade::BuildForwardConeForBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const FunctionalNodeId& root,
    int max_depth
) {
    try {
        // Build the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<FunctionalCone>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        // Perform functional analysis on the graph
        FunctionalAnalysis analysis;
        auto cone_result = analysis.ComputeForwardCone(graph_result.data, root, max_depth);
        if (!cone_result.ok) {
            return Result<FunctionalCone>::MakeError(
                cone_result.error_code,
                cone_result.error_message
            );
        }

        return Result<FunctionalCone>::MakeOk(cone_result.data);
    }
    catch (const std::exception& e) {
        return Result<FunctionalCone>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildForwardConeForBranch: ") + e.what()
        );
    }
}

Result<DependencySummary> CircuitFacade::BuildDependencySummaryForBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const FunctionalNodeId& root,
    int max_depth
) {
    try {
        // Build the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<DependencySummary>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        // Perform functional analysis on the graph
        FunctionalAnalysis analysis;
        auto summary_result = analysis.ComputeDependencySummary(graph_result.data, root, max_depth);
        if (!summary_result.ok) {
            return Result<DependencySummary>::MakeError(
                summary_result.error_code,
                summary_result.error_message
            );
        }

        return Result<DependencySummary>::MakeOk(summary_result.data);
    }
    catch (const std::exception& e) {
        return Result<DependencySummary>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildDependencySummaryForBranch: ") + e.what()
        );
    }
}

Result<BlockGraph> CircuitFacade::BuildBlockGraphForBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name
) {
    try {
        // Load the circuit for the specified branch
        CircuitData circuit;
        auto load_result = LoadCurrentCircuitForBranch(session, session_dir, branch_name, circuit);
        if (!load_result.ok) {
            return Result<BlockGraph>::MakeError(load_result.error_code, load_result.error_message);
        }

        // Build the circuit graph for the branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<BlockGraph>::MakeError(graph_result.error_code, graph_result.error_message);
        }

        // Use BlockAnalysis to detect blocks in the graph
        BlockAnalysis block_analysis;
        auto block_result = block_analysis.DetectBlocks(graph_result.data, circuit);
        if (!block_result.ok) {
            return Result<BlockGraph>::MakeError(block_result.error_code, block_result.error_message);
        }

        return Result<BlockGraph>::MakeOk(block_result.data);
    }
    catch (const std::exception& e) {
        return Result<BlockGraph>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildBlockGraphForBranch: ") + e.what()
        );
    }
}

Result<BehaviorDescriptor> CircuitFacade::InferBehaviorForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id
) {
    try {
        // First, get the block graph for the specified branch
        auto block_graph_result = BuildBlockGraphForBranch(session, session_dir, branch_name);
        if (!block_graph_result.ok) {
            return Result<BehaviorDescriptor>::MakeError(
                block_graph_result.error_code,
                block_graph_result.error_message
            );
        }

        // Find the requested block in the block graph
        const BlockGraph& block_graph = block_graph_result.data;
        const BlockInstance* target_block = nullptr;
        for (const auto& block : block_graph.blocks) {
            if (block.id == block_id) {
                target_block = &block;
                break;
            }
        }

        if (!target_block) {
            return Result<BehaviorDescriptor>::MakeError(
                ErrorCode::NotFound,
                "Block with ID " + block_id + " not found in branch " + branch_name
            );
        }

        // Get the circuit graph for behavioral analysis
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<BehaviorDescriptor>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        // Use BehavioralAnalysis to infer the behavior of the block
        BehavioralAnalysis behavioral_analysis;
        auto behavior_result = behavioral_analysis.InferBehaviorForBlock(*target_block, graph_result.data);
        if (!behavior_result.ok) {
            return Result<BehaviorDescriptor>::MakeError(
                behavior_result.error_code,
                behavior_result.error_message
            );
        }

        return Result<BehaviorDescriptor>::MakeOk(behavior_result.data);
    }
    catch (const std::exception& e) {
        return Result<BehaviorDescriptor>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in InferBehaviorForBlockInBranch: ") + e.what()
        );
    }
}

Result<BehaviorDescriptor> CircuitFacade::InferBehaviorForNodeInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& node_id,
    const std::string& node_kind_hint
) {
    try {
        // Get the circuit graph for behavioral analysis
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<BehaviorDescriptor>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        // Get the functional analysis for cone computation
        FunctionalAnalysis func_analysis;

        // Use BehavioralAnalysis to infer the behavior of the node
        BehavioralAnalysis behavioral_analysis;
        auto behavior_result = behavioral_analysis.InferBehaviorForNode(
            graph_result.data,
            func_analysis,
            node_id,
            node_kind_hint
        );

        if (!behavior_result.ok) {
            return Result<BehaviorDescriptor>::MakeError(
                behavior_result.error_code,
                behavior_result.error_message
            );
        }

        return Result<BehaviorDescriptor>::MakeOk(behavior_result.data);
    }
    catch (const std::exception& e) {
        return Result<BehaviorDescriptor>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in InferBehaviorForNodeInBranch: ") + e.what()
        );
    }
}

Result<IrModule> CircuitFacade::BuildIrForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id
) {
    try {
        // First, get the block graph for the specified branch
        auto block_graph_result = BuildBlockGraphForBranch(session, session_dir, branch_name);
        if (!block_graph_result.ok) {
            return Result<IrModule>::MakeError(
                block_graph_result.error_code,
                block_graph_result.error_message
            );
        }

        // Find the requested block in the block graph
        const BlockGraph& block_graph = block_graph_result.data;
        const BlockInstance* target_block = nullptr;
        for (const auto& block : block_graph.blocks) {
            if (block.id == block_id) {
                target_block = &block;
                break;
            }
        }

        if (!target_block) {
            return Result<IrModule>::MakeError(
                ErrorCode::NotFound,
                "Block with ID " + block_id + " not found in branch " + branch_name
            );
        }

        // Get the circuit graph for IR inference
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<IrModule>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        // Get the behavioral analysis for the block
        auto behavior_result = InferBehaviorForBlockInBranch(session, session_dir, branch_name, block_id);
        if (!behavior_result.ok) {
            return Result<IrModule>::MakeError(
                behavior_result.error_code,
                behavior_result.error_message
            );
        }

        // Use HlsIrInference to generate the IR for the block
        HlsIrInference ir_inference;
        auto ir_result = ir_inference.InferIrForBlock(*target_block, graph_result.data, behavior_result.data);
        if (!ir_result.ok) {
            return Result<IrModule>::MakeError(
                ir_result.error_code,
                ir_result.error_message
            );
        }

        return Result<IrModule>::MakeOk(ir_result.data);
    }
    catch (const std::exception& e) {
        return Result<IrModule>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildIrForBlockInBranch: ") + e.what()
        );
    }
}

Result<IrModule> CircuitFacade::BuildIrForNodeRegionInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& node_id,
    const std::string& node_kind_hint,
    int max_depth
) {
    try {
        // Get the circuit graph for IR inference
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<IrModule>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        // Get the functional analysis for cone computation
        FunctionalAnalysis func_analysis;
        BehavioralAnalysis beh_analysis;

        // Use HlsIrInference to generate the IR for the node region
        HlsIrInference ir_inference;
        auto ir_result = ir_inference.InferIrForNodeRegion(
            graph_result.data,
            node_id,
            node_kind_hint,
            func_analysis,
            beh_analysis,
            max_depth
        );

        if (!ir_result.ok) {
            return Result<IrModule>::MakeError(
                ir_result.error_code,
                ir_result.error_message
            );
        }

        return Result<IrModule>::MakeOk(ir_result.data);
    }
    catch (const std::exception& e) {
        return Result<IrModule>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildIrForNodeRegionInBranch: ") + e.what()
        );
    }
}

Result<std::vector<TransformationPlan>> CircuitFacade::ProposeTransformationsForBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    int max_plans
) {
    try {
        // Create a TransformationEngine instance and use it to propose transformations
        TransformationEngine engine;
        auto result = engine.ProposeTransformationsForBranch(session, session_dir, branch_name, max_plans);

        if (!result.ok) {
            return Result<std::vector<TransformationPlan>>::MakeError(
                result.error_code,
                result.error_message
            );
        }

        // Convert Upp::Vector to std::vector and Upp::String to std::string
        std::vector<TransformationPlan> plans;
        for (const auto& plan : result.data) {
            TransformationPlan std_plan;
            std_plan.id = std::string(plan.id.c_str());
            std_plan.kind = plan.kind;
            std_plan.target.subject_id = std::string(plan.target.subject_id.c_str());
            std_plan.target.subject_kind = std::string(plan.target.subject_kind.c_str());
            std_plan.guarantees = plan.guarantees;
            for (const auto& step : plan.steps) {
                TransformationStep std_step;
                std_step.description = std::string(step.description.c_str());
                std_plan.steps.push_back(std_step);
            }
            plans.push_back(std_plan);
        }

        return Result<std::vector<TransformationPlan>>::MakeOk(plans);
    }
    catch (const std::exception& e) {
        return Result<std::vector<TransformationPlan>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ProposeTransformationsForBranch: ") + e.what()
        );
    }
}

Result<std::vector<TransformationPlan>> CircuitFacade::ProposeTransformationsForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id,
    int max_plans
) {
    try {
        // Create a TransformationEngine instance and use it to propose transformations for a specific block
        TransformationEngine engine;
        auto result = engine.ProposeTransformationsForBlock(session, session_dir, branch_name, block_id, max_plans);

        if (!result.ok) {
            return Result<std::vector<TransformationPlan>>::MakeError(
                result.error_code,
                result.error_message
            );
        }

        // Convert Upp::Vector to std::vector and Upp::String to std::string
        std::vector<TransformationPlan> plans;
        for (const auto& plan : result.data) {
            TransformationPlan std_plan;
            std_plan.id = std::string(plan.id.c_str());
            std_plan.kind = plan.kind;
            std_plan.target.subject_id = std::string(plan.target.subject_id.c_str());
            std_plan.target.subject_kind = std::string(plan.target.subject_kind.c_str());
            std_plan.guarantees = plan.guarantees;
            for (const auto& step : plan.steps) {
                TransformationStep std_step;
                std_step.description = std::string(step.description.c_str());
                std_plan.steps.push_back(std_step);
            }
            plans.push_back(std_plan);
        }

        return Result<std::vector<TransformationPlan>>::MakeOk(plans);
    }
    catch (const std::exception& e) {
        return Result<std::vector<TransformationPlan>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ProposeTransformationsForBlockInBranch: ") + e.what()
        );
    }
}

Result<void> CircuitFacade::ApplyTransformationPlan(
    SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const TransformationPlan& plan,
    const std::string& user_id
) {
    try {
        // First, verify that the transformation preserves behavior
        TransformationEngine engine;
        auto verification_result = engine.VerifyBehaviorPreserved(session, session_dir, branch_name, plan);

        if (!verification_result.ok || !verification_result.data) {
            return Result<void>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Transformation does not preserve behavior: " +
                (verification_result.ok ? "Behavior verification failed" : verification_result.error_message)
            );
        }

        // Convert the TransformationPlan to EditOperations
        auto materialize_result = engine.MaterializePlan(plan);
        if (!materialize_result.ok) {
            return Result<void>::MakeError(
                materialize_result.error_code,
                materialize_result.error_message
            );
        }

        // Apply the edit operations to the branch
        auto& ops = materialize_result.data;
        std::vector<EditOperation> std_ops;
        for (const auto& op : ops) {
            EditOperation std_op;
            std_op.type = op.type;
            std_op.component_id = CircuitEntityId(std::string(op.component_id.id.c_str()));
            std_op.target_component_id = CircuitEntityId(std::string(op.target_component_id.id.c_str()));
            std_op.wire_id = CircuitEntityId(std::string(op.wire_id.id.c_str()));
            std_op.x = op.x;
            std_op.y = op.y;
            std_op.pin_name = std::string(op.pin_name.c_str());
            std_op.target_pin_name = std::string(op.target_pin_name.c_str());
            std_op.component_type = std::string(op.component_type.c_str());
            std_op.component_name = std::string(op.component_name.c_str());
            std_op.property_name = std::string(op.property_name.c_str());
            std_op.property_value = std::string(op.property_value.c_str());
            std_op.revision_base = op.revision_base;
            std_op.properties = op.properties;
            std_ops.push_back(std_op);
        }

        auto apply_result = ApplyEditOperationsToBranch(session, session_dir, std_ops, user_id, branch_name);
        if (!apply_result.ok) {
            return Result<void>::MakeError(
                apply_result.error_code,
                apply_result.error_message
            );
        }

        // Log the transformation as an event
        EventLogEntry event;
        event.timestamp = GetCurrentTimestamp();
        event.user_id = user_id;
        event.session_id = session.session_id;
        event.branch = branch_name;
        event.command = "apply_transformation";

        // Create params object for the transformation
        Upp::ValueMap params;
        params.Add("transformation_id", Upp::String(plan.id.c_str()));
        params.Add("transformation_kind", Upp::String(ToString(plan.kind).c_str()));
        params.Add("revision", Upp::String(std::to_string(apply_result.data.revision).c_str()));
        params.Add("branch", Upp::String(branch_name.c_str()));

        event.params = Upp::String().Cat() << params;

        // Create result object
        Upp::ValueMap result_data;
        result_data.Add("revision", Upp::String(std::to_string(apply_result.data.revision).c_str()));
        result_data.Add("branch", Upp::String(branch_name.c_str()));
        result_data.Add("transformation_applied", true);
        event.result = Upp::String().Cat() << result_data;

        EventLogger::LogEvent(session_dir, event);

        return Result<void>::MakeOk();
    }
    catch (const std::exception& e) {
        return Result<void>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ApplyTransformationPlan: ") + e.what()
        );
    }
}

Result<BehaviorDiff> CircuitFacade::DiffBlockBehaviorBetweenBranches(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_before,
    const std::string& branch_after,
    const std::string& block_id
) {
    try {
        // Get the behavior descriptor for the block on the 'before' branch
        auto before_result = InferBehaviorForBlockInBranch(session, session_dir, branch_before, block_id);
        if (!before_result.ok) {
            return Result<BehaviorDiff>::MakeError(
                before_result.error_code,
                "Error getting behavior from 'before' branch: " + before_result.error_message
            );
        }

        // Get the behavior descriptor for the block on the 'after' branch
        auto after_result = InferBehaviorForBlockInBranch(session, session_dir, branch_after, block_id);
        if (!after_result.ok) {
            return Result<BehaviorDiff>::MakeError(
                after_result.error_code,
                "Error getting behavior from 'after' branch: " + after_result.error_message
            );
        }

        // Use DiffAnalysis to compute the difference between the two behavior descriptors
        auto diff_result = DiffAnalysis::DiffBehavior(before_result.data, after_result.data);
        if (!diff_result.ok) {
            return Result<BehaviorDiff>::MakeError(
                diff_result.error_code,
                "Error computing behavior diff: " + diff_result.error_message
            );
        }

        return Result<BehaviorDiff>::MakeOk(diff_result.data);
    }
    catch (const std::exception& e) {
        return Result<BehaviorDiff>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in DiffBlockBehaviorBetweenBranches: ") + e.what()
        );
    }
}

Result<IrDiff> CircuitFacade::DiffBlockIrBetweenBranches(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_before,
    const std::string& branch_after,
    const std::string& block_id
) {
    try {
        // Get the IR module for the block on the 'before' branch
        auto before_result = BuildIrForBlockInBranch(session, session_dir, branch_before, block_id);
        if (!before_result.ok) {
            return Result<IrDiff>::MakeError(
                before_result.error_code,
                "Error getting IR from 'before' branch: " + before_result.error_message
            );
        }

        // Get the IR module for the block on the 'after' branch
        auto after_result = BuildIrForBlockInBranch(session, session_dir, branch_after, block_id);
        if (!after_result.ok) {
            return Result<IrDiff>::MakeError(
                after_result.error_code,
                "Error getting IR from 'after' branch: " + after_result.error_message
            );
        }

        // Use DiffAnalysis to compute the difference between the two IR modules
        auto diff_result = DiffAnalysis::DiffIrModule(before_result.data, after_result.data);
        if (!diff_result.ok) {
            return Result<IrDiff>::MakeError(
                diff_result.error_code,
                "Error computing IR diff: " + diff_result.error_message
            );
        }

        return Result<IrDiff>::MakeOk(diff_result.data);
    }
    catch (const std::exception& e) {
        return Result<IrDiff>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in DiffBlockIrBetweenBranches: ") + e.what()
        );
    }
}

Result<IrDiff> CircuitFacade::DiffNodeRegionIrBetweenBranches(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_before,
    const std::string& branch_after,
    const std::string& node_id,
    const std::string& node_kind_hint,
    int max_depth
) {
    try {
        // Get the IR module for the node region on the 'before' branch
        auto before_result = BuildIrForNodeRegionInBranch(
            session, session_dir, branch_before, node_id, node_kind_hint, max_depth
        );
        if (!before_result.ok) {
            return Result<IrDiff>::MakeError(
                before_result.error_code,
                "Error getting IR from 'before' branch: " + before_result.error_message
            );
        }

        // Get the IR module for the node region on the 'after' branch
        auto after_result = BuildIrForNodeRegionInBranch(
            session, session_dir, branch_after, node_id, node_kind_hint, max_depth
        );
        if (!after_result.ok) {
            return Result<IrDiff>::MakeError(
                after_result.error_code,
                "Error getting IR from 'after' branch: " + after_result.error_message
            );
        }

        // Use DiffAnalysis to compute the difference between the two IR modules
        auto diff_result = DiffAnalysis::DiffIrModule(before_result.data, after_result.data);
        if (!diff_result.ok) {
            return Result<IrDiff>::MakeError(
                diff_result.error_code,
                "Error computing IR diff: " + diff_result.error_message
            );
        }

        return Result<IrDiff>::MakeOk(diff_result.data);
    }
    catch (const std::exception& e) {
        return Result<IrDiff>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in DiffNodeRegionIrBetweenBranches: ") + e.what()
        );
    }
}

Result<IrOptimizationResult> CircuitFacade::OptimizeBlockIrInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id,
    const std::vector<IrOptPassKind>& passes_to_run
) {
    try {
        // Step 1: Get the IR for the block in the specified branch
        auto ir_result = BuildIrForBlockInBranch(session, session_dir, branch_name, block_id);
        if (!ir_result.ok) {
            return Result<IrOptimizationResult>::MakeError(
                ir_result.error_code,
                ir_result.error_message
            );
        }

        IrModule original_ir = ir_result.data;

        // Step 2: Get the behavioral descriptor before optimization (for verification)
        auto behavior_before_result = InferBehaviorForBlockInBranch(session, session_dir, branch_name, block_id);
        if (!behavior_before_result.ok) {
            // If behavioral analysis fails, we can still proceed with optimization
            // but we won't be able to verify behavior preservation
        }

        BehaviorDescriptor behavior_before = behavior_before_result.ok ? behavior_before_result.data : BehaviorDescriptor();

        // Step 3: Run the optimization passes
        IrOptimizer optimizer;
        auto opt_result = optimizer.OptimizeModule(original_ir, passes_to_run);
        if (!opt_result.ok) {
            return Result<IrOptimizationResult>::MakeError(
                opt_result.error_code,
                opt_result.error_message
            );
        }

        IrOptimizationResult optimization_result = opt_result.data;

        // Step 4: Get the behavioral descriptor after optimization (for verification)
        BehaviorDescriptor behavior_after;
        if (behavior_before_result.ok) {
            // Infer behavior of optimized IR - this is a simplified approach
            // In practice, we'd need to apply the optimized IR back to the circuit
            // and then analyze the behavior of the resulting circuit
        }

        // Step 5: Verify behavior preservation (if both before and after behaviors are available)
        if (behavior_before_result.ok) {
            auto behavior_check = VerifyIrOptimizationBehaviorPreserved(behavior_before, behavior_after);
            if (behavior_check.ok && behavior_check.data) {
                // Mark all summaries as behavior-preserving
                for (auto& summary : optimization_result.summaries) {
                    summary.behavior_preserved = true;
                }
            } else {
                // Mark all summaries as potentially not behavior-preserving
                for (auto& summary : optimization_result.summaries) {
                    summary.behavior_preserved = false;
                }
            }
        }

        return Result<IrOptimizationResult>::MakeOk(optimization_result);
    }
    catch (const std::exception& e) {
        return Result<IrOptimizationResult>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in OptimizeBlockIrInBranch: ") + e.what()
        );
    }
}

Result<std::vector<TransformationPlan>> CircuitFacade::ProposeIrBasedTransformationsForBlock(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id,
    const std::vector<IrOptPassKind>& passes_to_run
) {
    try {
        // Step 1: Get the original IR for the block
        auto original_ir_result = BuildIrForBlockInBranch(session, session_dir, branch_name, block_id);
        if (!original_ir_result.ok) {
            return Result<std::vector<TransformationPlan>>::MakeError(
                original_ir_result.error_code,
                original_ir_result.error_message
            );
        }

        IrModule original_ir = original_ir_result.data;

        // Step 2: Optimize the IR
        IrOptimizer optimizer;
        auto opt_result = optimizer.OptimizeModule(original_ir, passes_to_run);
        if (!opt_result.ok) {
            return Result<std::vector<TransformationPlan>>::MakeError(
                opt_result.error_code,
                opt_result.error_message
            );
        }

        IrModule optimized_ir = opt_result.data.optimized;

        // Step 3: Compute the IR diff
        auto diff_result = DiffAnalysis::DiffIrModule(original_ir, optimized_ir);
        if (!diff_result.ok) {
            return Result<std::vector<TransformationPlan>>::MakeError(
                diff_result.error_code,
                diff_result.error_message
            );
        }

        IrDiff ir_diff = diff_result.data;

        // Step 4: Convert the IR diff to transformation plans
        auto plans_result = IrToTransformationBridge::PlansFromIrDiff(original_ir, optimized_ir, ir_diff, block_id);
        if (!plans_result.ok) {
            return Result<std::vector<TransformationPlan>>::MakeError(
                plans_result.error_code,
                plans_result.error_message
            );
        }

        return Result<std::vector<TransformationPlan>>::MakeOk(plans_result.data);
    }
    catch (const std::exception& e) {
        return Result<std::vector<TransformationPlan>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ProposeIrBasedTransformationsForBlock: ") + e.what()
        );
    }
}

Result<ScheduledModule> CircuitFacade::BuildScheduledIrForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id,
    const SchedulingConfig& config
) {
    try {
        // Step 1: Get the IR for the block
        auto ir_result = BuildIrForBlockInBranch(session, session_dir, branch_name, block_id);
        if (!ir_result.ok) {
            return Result<ScheduledModule>::MakeError(
                ir_result.error_code,
                ir_result.error_message
            );
        }

        IrModule ir = ir_result.data;

        // Step 2: Optionally get timing analysis for more accurate scheduling
        // For now, we'll pass nullptr since we don't have a method to get timing data for a block
        TimingAnalysis* timing = nullptr;
        CircuitGraph* graph = nullptr;

        // Step 3: Build the scheduled IR using the scheduling engine
        auto schedule_result = SchedulingEngine::BuildSchedule(ir, timing, graph, config);
        if (!schedule_result.ok) {
            return Result<ScheduledModule>::MakeError(
                schedule_result.error_code,
                schedule_result.error_message
            );
        }

        return Result<ScheduledModule>::MakeOk(schedule_result.data);
    }
    catch (const std::exception& e) {
        return Result<ScheduledModule>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildScheduledIrForBlockInBranch: ") + e.what()
        );
    }
}

Result<ScheduledModule> CircuitFacade::BuildScheduledIrForNodeRegionInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& node_id,
    const std::string& node_kind_hint,
    int max_depth,
    const SchedulingConfig& config
) {
    try {
        // Step 1: Get the IR for the node region
        auto ir_result = BuildIrForNodeRegionInBranch(session, session_dir, branch_name, node_id, node_kind_hint, max_depth);
        if (!ir_result.ok) {
            return Result<ScheduledModule>::MakeError(
                ir_result.error_code,
                ir_result.error_message
            );
        }

        IrModule ir = ir_result.data;

        // Step 2: Optionally get timing analysis for more accurate scheduling
        // For now, we'll pass nullptr since we don't have a method to get timing data for a node region
        TimingAnalysis* timing = nullptr;
        CircuitGraph* graph = nullptr;

        // Step 3: Build the scheduled IR using the scheduling engine
        auto schedule_result = SchedulingEngine::BuildSchedule(ir, timing, graph, config);
        if (!schedule_result.ok) {
            return Result<ScheduledModule>::MakeError(
                schedule_result.error_code,
                schedule_result.error_message
            );
        }

        return Result<ScheduledModule>::MakeOk(schedule_result.data);
    }
    catch (const std::exception& e) {
        return Result<ScheduledModule>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildScheduledIrForNodeRegionInBranch: ") + e.what()
        );
    }
}

Result<PipelineMap> CircuitFacade::BuildPipelineMapForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id
) {
    try {
        // Step 1: Get the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<PipelineMap>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        const CircuitGraph& graph = graph_result.data;

        // Step 2: Optionally get timing analysis for more accurate pipeline analysis
        auto timing_graph_result = BuildTimingGraphForBranch(session, session_dir, branch_name);
        TimingAnalysis timing_analysis;
        // We'll use the timing analysis if we have one, but it's optional

        // Step 3: Get the scheduled IR for the block if available
        SchedulingConfig config;  // Use default configuration
        auto scheduled_ir_result = BuildScheduledIrForBlockInBranch(session, session_dir, branch_name, block_id, config);
        const ScheduledModule* scheduled_ir = scheduled_ir_result.ok ? &scheduled_ir_result.data : nullptr;

        // Step 4: Build the pipeline map using the pipeline analysis engine
        auto pipeline_result = PipelineAnalysis::BuildPipelineMapForBlock(graph,
                                                                         scheduled_ir_result.ok ? &timing_analysis : nullptr,
                                                                         scheduled_ir,
                                                                         block_id);
        if (!pipeline_result.ok) {
            return Result<PipelineMap>::MakeError(
                pipeline_result.error_code,
                pipeline_result.error_message
            );
        }

        return Result<PipelineMap>::MakeOk(pipeline_result.data);
    }
    catch (const std::exception& e) {
        return Result<PipelineMap>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildPipelineMapForBlockInBranch: ") + e.what()
        );
    }
}

Result<PipelineMap> CircuitFacade::BuildPipelineMapForSubsystemInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& subsystem_id,
    const std::vector<std::string>& block_ids
) {
    try {
        // Step 1: Get the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<PipelineMap>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        const CircuitGraph& graph = graph_result.data;

        // Step 2: Optionally get timing analysis for more accurate pipeline analysis
        auto timing_graph_result = BuildTimingGraphForBranch(session, session_dir, branch_name);
        TimingAnalysis timing_analysis;
        // We'll use the timing analysis if we have one, but it's optional

        // Step 3: Attempt to get scheduled IR for blocks if available (this is more complex for subsystems)
        // For now, we'll pass nullptr as scheduled IR for subsystem level
        const ScheduledModule* scheduled_ir = nullptr;

        // Step 4: Build the pipeline map using the pipeline analysis engine for subsystem
        auto pipeline_result = PipelineAnalysis::BuildPipelineMapForSubsystem(graph,
                                                                              scheduled_ir_result.ok ? &timing_analysis : nullptr,
                                                                              scheduled_ir,
                                                                              subsystem_id,
                                                                              block_ids);
        if (!pipeline_result.ok) {
            return Result<PipelineMap>::MakeError(
                pipeline_result.error_code,
                pipeline_result.error_message
            );
        }

        return Result<PipelineMap>::MakeOk(pipeline_result.data);
    }
    catch (const std::exception& e) {
        return Result<PipelineMap>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildPipelineMapForSubsystemInBranch: ") + e.what()
        );
    }
}

Result<CdcReport> CircuitFacade::BuildCdcReportForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id
) {
    try {
        // Step 1: Get the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<CdcReport>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        const CircuitGraph& graph = graph_result.data;

        // Step 2: Get the pipeline map for the specified block
        auto pipeline_result = BuildPipelineMapForBlockInBranch(session, session_dir, branch_name, block_id);
        if (!pipeline_result.ok) {
            return Result<CdcReport>::MakeError(
                pipeline_result.error_code,
                pipeline_result.error_message
            );
        }

        const PipelineMap& pipeline = pipeline_result.data;

        // Step 3: Optionally get timing analysis for more detailed CDC analysis
        auto timing_graph_result = BuildTimingGraphForBranch(session, session_dir, branch_name);
        TimingAnalysis* timing = nullptr;  // We'll pass nullptr for now since the result is complex to handle

        // Step 4: Use the CDC analysis engine to build the report
        auto cdc_result = CdcAnalysis::BuildCdcReportForBlock(pipeline, graph, timing);
        if (!cdc_result.ok) {
            return Result<CdcReport>::MakeError(
                cdc_result.error_code,
                cdc_result.error_message
            );
        }

        return Result<CdcReport>::MakeOk(cdc_result.data);
    }
    catch (const std::exception& e) {
        return Result<CdcReport>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildCdcReportForBlockInBranch: ") + e.what()
        );
    }
}

Result<CdcReport> CircuitFacade::BuildCdcReportForSubsystemInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& subsystem_id,
    const std::vector<std::string>& block_ids
) {
    try {
        // Step 1: Get the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<CdcReport>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        const CircuitGraph& graph = graph_result.data;

        // Step 2: Get the pipeline map for the specified subsystem
        auto pipeline_result = BuildPipelineMapForSubsystemInBranch(session, session_dir, branch_name, subsystem_id, block_ids);
        if (!pipeline_result.ok) {
            return Result<CdcReport>::MakeError(
                pipeline_result.error_code,
                pipeline_result.error_message
            );
        }

        const PipelineMap& pipeline = pipeline_result.data;

        // Step 3: Optionally get timing analysis for more detailed CDC analysis
        auto timing_graph_result = BuildTimingGraphForBranch(session, session_dir, branch_name);
        TimingAnalysis* timing = nullptr;  // We'll pass nullptr for now

        // Step 4: Use the CDC analysis engine to build the report
        auto cdc_result = CdcAnalysis::BuildCdcReportForSubsystem(pipeline, graph, timing);
        if (!cdc_result.ok) {
            return Result<CdcReport>::MakeError(
                cdc_result.error_code,
                cdc_result.error_message
            );
        }

        return Result<CdcReport>::MakeOk(cdc_result.data);
    }
    catch (const std::exception& e) {
        return Result<CdcReport>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BuildCdcReportForSubsystemInBranch: ") + e.what()
        );
    }
}

Result<Vector<RetimingPlan>> CircuitFacade::AnalyzeRetimingForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id
) {
    try {
        Vector<String> block_ids;
        block_ids.Add(String(block_id.c_str()));
        return PerformRetimingAnalysis(session, session_dir, branch_name, block_id, block_ids, false);
    }
    catch (const std::exception& e) {
        return Result<Vector<RetimingPlan>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in AnalyzeRetimingForBlockInBranch: ") + e.what()
        );
    }
}

Result<Vector<RetimingPlan>> CircuitFacade::AnalyzeRetimingForSubsystemInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& subsystem_id,
    const Vector<String>& block_ids
) {
    try {
        return PerformRetimingAnalysis(session, session_dir, branch_name, subsystem_id, block_ids, true);
    }
    catch (const std::exception& e) {
        return Result<Vector<RetimingPlan>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in AnalyzeRetimingForSubsystemInBranch: ") + e.what()
        );
    }
}

Result<Vector<RetimingPlan>> CircuitFacade::PerformRetimingAnalysis(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& target_id,
    const Vector<String>& block_ids,
    bool is_subsystem
) {
    try {
        // Step 1: Get the circuit graph for the specified branch
        auto graph_result = BuildGraphForBranch(session, session_dir, branch_name);
        if (!graph_result.ok) {
            return Result<Vector<RetimingPlan>>::MakeError(
                graph_result.error_code,
                graph_result.error_message
            );
        }

        const CircuitGraph& graph = graph_result.data;

        // Step 2: Get the pipeline map for the specified block or subsystem
        PipelineMap pipeline;
        if (is_subsystem) {
            auto pipeline_result = BuildPipelineMapForSubsystemInBranch(
                session, session_dir, branch_name, target_id,
                std::vector<std::string>(block_ids.Begin(), block_ids.End()));
            if (!pipeline_result.ok) {
                return Result<Vector<RetimingPlan>>::MakeError(
                    pipeline_result.error_code,
                    pipeline_result.error_message
                );
            }
            pipeline = pipeline_result.data;
        } else {
            auto pipeline_result = BuildPipelineMapForBlockInBranch(
                session, session_dir, branch_name, target_id);
            if (!pipeline_result.ok) {
                return Result<Vector<RetimingPlan>>::MakeError(
                    pipeline_result.error_code,
                    pipeline_result.error_message
                );
            }
            pipeline = pipeline_result.data;
        }

        // Step 3: Get the CDC report for the specified block or subsystem
        CdcReport cdc_report;
        if (is_subsystem) {
            auto cdc_result = BuildCdcReportForSubsystemInBranch(
                session, session_dir, branch_name, target_id,
                std::vector<std::string>(block_ids.Begin(), block_ids.End()));
            if (!cdc_result.ok) {
                return Result<Vector<RetimingPlan>>::MakeError(
                    cdc_result.error_code,
                    cdc_result.error_message
                );
            }
            cdc_report = cdc_result.data;
        } else {
            auto cdc_result = BuildCdcReportForBlockInBranch(
                session, session_dir, branch_name, target_id);
            if (!cdc_result.ok) {
                return Result<Vector<RetimingPlan>>::MakeError(
                    cdc_result.error_code,
                    cdc_result.error_message
                );
            }
            cdc_report = cdc_result.data;
        }

        // Step 4: Optionally get timing analysis for more detailed retiming analysis
        auto timing_graph_result = BuildTimingGraphForBranch(session, session_dir, branch_name);
        TimingAnalysis* timing = nullptr;  // We'll pass nullptr for now

        // Step 5: Optionally get scheduled IR for more detailed analysis
        ScheduledModule* scheduled_ir = nullptr;  // We'll pass nullptr for now
        // In a real implementation, we might want to build scheduled IR for the target

        // Step 6: Use the retiming analysis engine to build the plans
        auto retiming_result = RetimingAnalysis::AnalyzeRetimingForBlock(
            pipeline, cdc_report, timing, scheduled_ir);
        if (!retiming_result.ok) {
            return Result<Vector<RetimingPlan>>::MakeError(
                retiming_result.error_code,
                retiming_result.error_message
            );
        }

        return Result<Vector<RetimingPlan>>::MakeOk(retiming_result.data);
    }
    catch (const std::exception& e) {
        return Result<Vector<RetimingPlan>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in PerformRetimingAnalysis: ") + e.what()
        );
    }
}

Result<RetimingApplicationResult> CircuitFacade::ApplyRetimingPlanForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const RetimingPlan& plan,
    const RetimingApplicationOptions& options
) {
    try {
        // Use RetimingTransform to apply the plan
        SessionStore session_store;
        auto result = RetimingTransform::ApplyRetimingPlanInBranch(
            plan, options, session_store, session, String(session_dir.c_str()), String(branch_name.c_str()));
        return result;
    }
    catch (const std::exception& e) {
        return Result<RetimingApplicationResult>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ApplyRetimingPlanForBlockInBranch: ") + e.what()
        );
    }
}

Result<RetimingApplicationResult> CircuitFacade::ApplyRetimingPlanForSubsystemInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const RetimingPlan& plan,
    const RetimingApplicationOptions& options
) {
    try {
        // Use RetimingTransform to apply the plan
        SessionStore session_store;
        auto result = RetimingTransform::ApplyRetimingPlanInBranch(
            plan, options, session_store, session, String(session_dir.c_str()), String(branch_name.c_str()));
        return result;
    }
    catch (const std::exception& e) {
        return Result<RetimingApplicationResult>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ApplyRetimingPlanForSubsystemInBranch: ") + e.what()
        );
    }
}

} // namespace ProtoVMCLI