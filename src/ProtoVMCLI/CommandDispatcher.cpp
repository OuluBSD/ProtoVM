#include "CommandDispatcher.h"
#include "JsonIO.h"
#include "EngineFacade.h"
#include "EventLogger.h"
#include "CircuitFacade.h"
#include "CircuitOps.h"
#include "CircuitAnalysis.h"
#include "CircuitDiagnostics.h"
#include "BranchOperations.h"
#include "BranchTypes.h"
#include "CircuitGraph.h"
#include "CircuitGraphQueries.h"
#include "BehavioralAnalysis.h"
#include "Transformations.h"
#include "DiffAnalysis.h"
#include "IrOptimization.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>

// Helper function to find a branch by name in session metadata
std::optional<BranchMetadata> FindBranchByName(const SessionMetadata& session, const std::string& branch_name) {
    for (const auto& branch : session.branches) {
        if (branch.name == branch_name) {
            return branch;
        }
    }
    return std::nullopt;
}

namespace fs = std::filesystem;

namespace ProtoVMCLI {

CommandDispatcher::CommandDispatcher(std::unique_ptr<ISessionStore> store)
    : session_store_(std::move(store)) {
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

Upp::String CommandDispatcher::RunInitWorkspace(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("init-workspace", "Workspace path is required", "INVALID_ARGUMENT");
    }

    try {
        fs::path workspace_path(opts.workspace);

        // Check if workspace exists and contains workspace.json
        fs::path workspace_json_path = workspace_path / "workspace.json";
        bool already_exists = fs::exists(workspace_path) && fs::is_directory(workspace_path);
        bool has_workspace_json = fs::exists(workspace_json_path);

        if (already_exists && !has_workspace_json) {
            // Existing directory but no workspace.json - invalid workspace
            return JsonIO::ErrorResponse("init-workspace",
                                       "Directory exists but is not a valid ProtoVM workspace (missing workspace.json)",
                                       "INVALID_WORKSPACE");
        }

        // If workspace doesn't exist, create it
        if (!already_exists) {
            fs::create_directories(workspace_path);
        }

        // Create subdirectories
        fs::create_directories(workspace_path / "sessions");
        fs::create_directories(workspace_path / "logs");
        fs::create_directories(workspace_path / "artifacts");

        // If workspace.json doesn't exist, create it with initial configuration
        if (!has_workspace_json) {
            Upp::ValueMap workspace_config;
            workspace_config.Add("schema_version", 1);
            workspace_config.Add("created_at", Upp::String(GetCurrentTimestamp().c_str()));
            workspace_config.Add("created_with", Upp::String("proto-vm-cli/0.1.0"));
            workspace_config.Add("engine_version", Upp::String("unknown"));
            workspace_config.Add("next_session_id", 1);

            std::ofstream config_file(workspace_json_path);
            config_file << Upp::String().Cat() << workspace_config;
            config_file.close();
        }

        Upp::ValueMap response_data;
        response_data.Add("workspace", Upp::String(opts.workspace.c_str()));
        response_data.Add("created", !already_exists);  // true if we just created it
        response_data.Add("version", "0.1");

        return JsonIO::SuccessResponse("init-workspace", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("init-workspace",
                                   "Failed to initialize workspace: " + std::string(e.what()),
                                   "WORKSPACE_INITIALIZATION_ERROR");
    }
}

Upp::String CommandDispatcher::RunCreateSession(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("create-session", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.circuit_file.has_value()) {
        return JsonIO::ErrorResponse("create-session", "Circuit file path is required", "INVALID_ARGUMENT");
    }

    if (!ValidateWorkspace(opts.workspace)) {
        return JsonIO::ErrorResponse("create-session", "Invalid workspace path", "INVALID_WORKSPACE");
    }

    // Check if circuit file exists
    if (!fs::exists(opts.circuit_file.value())) {
        return JsonIO::ErrorResponse("create-session",
                                    "Circuit file does not exist: " + opts.circuit_file.value(),
                                    "CIRCUIT_FILE_NOT_FOUND");
    }

    try {
        SessionCreateInfo create_info(opts.workspace, opts.circuit_file.value());
        auto result = session_store_->CreateSession(create_info);

        if (!result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(result.error_code);
            return JsonIO::ErrorResponse("create-session", result.error_message, error_code_str);
        }

        // Create the session directory structure
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(result.data);
        fs::create_directories(session_dir + "/snapshots");
        fs::create_directories(session_dir + "/netlists");

        // Initialize the engine facade and create the initial snapshot
        ProtoVMCLI::EngineFacade engine_facade;
        auto session_metadata = session_store_->LoadSession(result.data);
        if (session_metadata.ok) {
            auto init_result = engine_facade.InitializeNewSession(
                session_metadata.data,
                opts.circuit_file.value(),
                session_dir
            );

            if (!init_result.ok) {
                // Rollback: delete the session if engine initialization failed
                session_store_->DeleteSession(result.data);
                fs::remove_all(session_dir);
                std::string error_code_str = JsonIO::ErrorCodeToString(init_result.error_code);
                return JsonIO::ErrorResponse("create-session", init_result.error_message, error_code_str);
            }

            // Update the session metadata with snapshot information
            session_metadata.data.total_ticks = init_result.data.total_ticks;
            // Initialize both revisions to 0 (0 means no edits have been made yet)
            // When simulation starts from an initial circuit, both revisions are the same
            session_metadata.data.circuit_revision = 0;
            session_metadata.data.sim_revision = 0;
            session_store_->SaveSession(session_metadata.data);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", result.data);
        response_data.Add("workspace", Upp::String(opts.workspace.c_str()));
        response_data.Add("circuit_file", Upp::String(opts.circuit_file.value().c_str()));
        response_data.Add("state", "ready");
        response_data.Add("total_ticks", 0);
        response_data.Add("last_snapshot_file", Upp::String(init_result.data.snapshot_file.c_str()));

        // Log the event
        EventLogEntry event;
        event.timestamp = init_result.data.timestamp;
        event.user_id = opts.user_id;
        event.session_id = result.data;
        event.command = "create-session";

        Upp::ValueMap params;
        params.Add("circuit_file", Upp::String(opts.circuit_file.value().c_str()));
        event.params = Upp::String().Cat() << params;

        Upp::ValueMap results;
        results.Add("session_id", result.data);
        results.Add("total_ticks", 0);
        results.Add("snapshot_file", Upp::String(init_result.data.snapshot_file.c_str()));
        event.result = Upp::String().Cat() << results;

        EventLogger::LogEvent(session_dir, event);

        return JsonIO::SuccessResponse("create-session", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("create-session",
                                   "Failed to create session: " + std::string(e.what()),
                                   "SESSION_CREATION_ERROR");
    }
}

Upp::String CommandDispatcher::RunListSessions(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("list-sessions", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!ValidateWorkspace(opts.workspace)) {
        return JsonIO::ErrorResponse("list-sessions", "Invalid workspace path", "INVALID_WORKSPACE");
    }

    try {
        auto result = session_store_->ListSessions();

        if (!result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(result.error_code);
            return JsonIO::ErrorResponse("list-sessions", result.error_message, error_code_str);
        }

        Upp::ValueArray sessions_array;

        for (const auto& session : result.data.sessions) {
            Upp::ValueMap session_obj;
            session_obj.Add("session_id", session.session_id);
            session_obj.Add("state", static_cast<int>(session.state));
            session_obj.Add("circuit_file", Upp::String(session.circuit_file.c_str()));
            session_obj.Add("created_at", Upp::String(session.created_at.c_str()));
            session_obj.Add("last_used_at", Upp::String(session.last_used_at.c_str()));
            session_obj.Add("total_ticks", session.total_ticks);
            sessions_array.Add(session_obj);
        }

        Upp::ValueArray corrupt_sessions_array;
        for (const auto& corrupt_id : result.data.corrupt_sessions) {
            corrupt_sessions_array.Add(corrupt_id);
        }

        Upp::ValueMap response_data;
        response_data.Add("sessions", sessions_array);
        response_data.Add("corrupt_sessions", corrupt_sessions_array);

        return JsonIO::SuccessResponse("list-sessions", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("list-sessions",
                                   "Failed to list sessions: " + std::string(e.what()),
                                   "SESSION_LIST_ERROR");
    }
}

Upp::String CommandDispatcher::RunRunTicks(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("run-ticks", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("run-ticks", "Session ID is required", "INVALID_ARGUMENT");
    }

    int ticks = opts.ticks.value_or(1);
    if (ticks <= 0) {
        return JsonIO::ErrorResponse("run-ticks", "Ticks must be positive", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("run-ticks", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use for simulation
        std::string branch_name = opts.branch.value_or(metadata.current_branch);

        // Find the branch metadata
        std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
        if (!branch_opt.has_value()) {
            return JsonIO::ErrorResponse("run-ticks", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
        }

        BranchMetadata& branch = const_cast<BranchMetadata&>(branch_opt.value());

        // Check if circuit has changed on this branch since the last simulation snapshot
        if (branch.head_revision != branch.sim_revision) {
            // The circuit has changed since the last simulation snapshot on this branch, so we need
            // to rebuild the machine from the current circuit state before running ticks
            // This will be handled by EngineFacade::LoadFromLatestSnapshot automatically
        }

        // Use EngineFacade to run ticks and create snapshot
        ProtoVMCLI::EngineFacade engine_facade;
        std::unique_ptr<Machine> machine;

        auto load_snapshot_result = engine_facade.LoadFromLatestSnapshot(
            metadata, session_dir, machine);

        if (!load_snapshot_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_snapshot_result.error_code);
            return JsonIO::ErrorResponse("run-ticks", load_snapshot_result.error_message, error_code_str);
        }

        auto run_result = engine_facade.RunTicksAndSnapshot(
            metadata, machine, ticks, session_dir);

        if (!run_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(run_result.error_code);
            return JsonIO::ErrorResponse("run-ticks", run_result.error_message, error_code_str);
        }

        // Update the session metadata with new tick count
        metadata.total_ticks = run_result.data.total_ticks;

        // Update the branch's sim_revision to match the head_revision after running simulation
        // This indicates that the simulation is now up-to-date with the circuit state on this branch
        for (auto& b : metadata.branches) {
            if (b.name == branch_name) {
                b.sim_revision = b.head_revision;
                break;
            }
        }

        // Update last_used_at timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::gmtime(&time_t);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
        metadata.last_used_at = std::string(buffer);

        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("run-ticks", save_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("ticks_run", ticks);
        response_data.Add("total_ticks", metadata.total_ticks);
        response_data.Add("last_snapshot_file", Upp::String(run_result.data.snapshot_file.c_str()));
        response_data.Add("state", "ready");

        // Log the event
        EventLogEntry event;
        event.timestamp = run_result.data.timestamp;
        event.user_id = opts.user_id;
        event.session_id = opts.session_id.value();
        event.command = "run-ticks";

        Upp::ValueMap params;
        params.Add("ticks", ticks);
        event.params = Upp::String().Cat() << params;

        Upp::ValueMap results;
        results.Add("ticks_run", ticks);
        results.Add("total_ticks", metadata.total_ticks);
        results.Add("snapshot_file", Upp::String(run_result.data.snapshot_file.c_str()));
        event.result = Upp::String().Cat() << results;

        EventLogger::LogEvent(session_dir, event);

        return JsonIO::SuccessResponse("run-ticks", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("run-ticks",
                                   "Failed to run ticks: " + std::string(e.what()),
                                   "RUN_TICKS_ERROR");
    }
}

Upp::String CommandDispatcher::RunGetState(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("get-state", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("get-state", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        auto result = session_store_->LoadSession(opts.session_id.value());

        if (!result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(result.error_code);
            return JsonIO::ErrorResponse("get-state", result.error_message, error_code_str);
        }

        auto metadata = result.data;  // Make a copy since we might modify it
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use for state
        std::string branch_name = opts.branch.value_or(metadata.current_branch);

        // Find the branch metadata
        std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
        if (!branch_opt.has_value()) {
            return JsonIO::ErrorResponse("get-state", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
        }

        BranchMetadata& branch = const_cast<BranchMetadata&>(branch_opt.value());

        // Use EngineFacade to query the state of the latest snapshot
        ProtoVMCLI::EngineFacade engine_facade;
        std::unique_ptr<Machine> machine;

        auto load_snapshot_result = engine_facade.LoadFromLatestSnapshot(
            metadata, session_dir, machine);

        // If loading the snapshot failed, we still return the session metadata but with less detail
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("state", static_cast<int>(metadata.state));
        response_data.Add("circuit_file", Upp::String(metadata.circuit_file.c_str()));
        response_data.Add("total_ticks", metadata.total_ticks);
        response_data.Add("circuit_revision", branch.head_revision);  // Use branch revision
        response_data.Add("sim_revision", branch.sim_revision);       // Use branch sim revision
        response_data.Add("created_at", Upp::String(metadata.created_at.c_str()));
        response_data.Add("last_used_at", Upp::String(metadata.last_used_at.c_str()));

        Upp::ValueArray breakpoints_array;
        response_data.Add("breakpoints", breakpoints_array);

        Upp::ValueArray traces_array;
        response_data.Add("traces", traces_array);

        Upp::ValueArray signals_array;
        response_data.Add("signals", signals_array);

        // Add snapshot information if available
        std::string latest_snapshot = engine_facade.GetLatestSnapshotFile(session_dir);
        if (!latest_snapshot.empty()) {
            response_data.Add("last_snapshot_file", Upp::String(latest_snapshot.c_str()));
        }

        return JsonIO::SuccessResponse("get-state", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("get-state",
                                   "Failed to get state: " + std::string(e.what()),
                                   "GET_STATE_ERROR");
    }
}

Upp::String CommandDispatcher::RunExportNetlist(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("export-netlist", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("export-netlist", "Session ID is required", "INVALID_ARGUMENT");
    }

    int pcb_id = opts.pcb_id.value_or(0);

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("export-netlist", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Use EngineFacade to load the machine and export netlist
        ProtoVMCLI::EngineFacade engine_facade;
        std::unique_ptr<Machine> machine;

        auto load_snapshot_result = engine_facade.LoadFromLatestSnapshot(
            metadata, session_dir, machine);

        if (!load_snapshot_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_snapshot_result.error_code);
            return JsonIO::ErrorResponse("export-netlist", load_snapshot_result.error_message, error_code_str);
        }

        auto export_result = engine_facade.ExportNetlist(
            metadata, machine, pcb_id);

        if (!export_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(export_result.error_code);
            return JsonIO::ErrorResponse("export-netlist", export_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("pcb_id", pcb_id);
        response_data.Add("netlist_file", Upp::String(export_result.data.c_str()));

        // Log the event
        EventLogEntry event;
        event.timestamp = GetCurrentTimestamp();
        event.user_id = opts.user_id;
        event.session_id = opts.session_id.value();
        event.command = "export-netlist";

        Upp::ValueMap params;
        params.Add("pcb_id", pcb_id);
        event.params = Upp::String().Cat() << params;

        Upp::ValueMap results;
        results.Add("netlist_file", Upp::String(export_result.data.c_str()));
        event.result = Upp::String().Cat() << results;

        EventLogger::LogEvent(session_dir, event);

        return JsonIO::SuccessResponse("export-netlist", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("export-netlist",
                                   "Failed to export netlist: " + std::string(e.what()),
                                   "NETLIST_EXPORT_ERROR");
    }
}

Upp::String CommandDispatcher::RunDestroySession(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("destroy-session", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("destroy-session", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // For now, we'll just call the store's delete function
        auto result = session_store_->DeleteSession(opts.session_id.value());

        if (!result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(result.error_code);
            return JsonIO::ErrorResponse("destroy-session", result.error_message, error_code_str);
        }

        // Delete the session directory and its contents
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());
        if (fs::exists(session_dir)) {
            fs::remove_all(session_dir);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("deleted", result.data);

        // Log the event
        EventLogEntry event;
        event.timestamp = GetCurrentTimestamp();
        event.user_id = opts.user_id;
        event.session_id = opts.session_id.value();
        event.command = "destroy-session";

        Upp::ValueMap params;
        params.Add("session_id", opts.session_id.value());
        event.params = Upp::String().Cat() << params;

        Upp::ValueMap results;
        results.Add("deleted", result.data);
        event.result = Upp::String().Cat() << results;

        EventLogger::LogEvent(session_dir, event);

        return JsonIO::SuccessResponse("destroy-session", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("destroy-session",
                                   "Failed to destroy session: " + std::string(e.what()),
                                   "SESSION_DELETION_ERROR");
    }
}

bool CommandDispatcher::ValidateWorkspace(const std::string& workspace_path) {
    try {
        fs::path path(workspace_path);

        // Check if path exists and is a directory
        if (!fs::exists(path) || !fs::is_directory(path)) {
            return false;
        }

        // Check if workspace.json exists
        fs::path ws_json_path = path / "workspace.json";
        if (!fs::exists(ws_json_path)) {
            return false;
        }

        // Optionally validate the workspace.json structure
        std::ifstream file(ws_json_path);
        if (!file.is_open()) {
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        // Basic validation that it contains required fields
        return content.find("\"schema_version\"") != std::string::npos &&
               content.find("\"next_session_id\"") != std::string::npos;
    } catch (...) {
        return false;
    }
}

Upp::String CommandDispatcher::RunDebugProcessLogs(int process_id) {
    try {
        // Output streaming events as JSON lines

        // Output initial status
        Upp::ValueMap status_event;
        status_event.Add("event", "status");
        status_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        status_event.Add("message", Upp::String("Starting process logs stream for process ID: " + Upp::AsString(process_id)));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(status_event) << std::endl;

        // Simulate log events
        for (int i = 0; i < 5; ++i) {  // Generate 5 log events
            Upp::ValueMap log_event;
            log_event.Add("event", "log");
            log_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
            Upp::ValueMap log_data;
            log_data.Add("line", Upp::String("Process " + Upp::AsString(process_id) + ": Log message " + Upp::AsString(i+1)));
            log_data.Add("level", "INFO");
            log_data.Add("source", "SimulatedProcess");
            log_event.Add("data", log_data);
            log_event.Add("process_id", process_id);

            std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(log_event) << std::endl;

            // Sleep briefly to simulate real-time streaming
            Upp::Sleep(100); // 100ms delay
        }

        // Output end event
        Upp::ValueMap end_event;
        end_event.Add("event", "end");
        end_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(end_event) << std::endl;

        // Return final status
        Upp::ValueMap response_data;
        response_data.Add("stream", "completed");
        response_data.Add("process_id", process_id);
        return ProtoVMCLI::JsonIO::SuccessResponse(response_data);
    }
    catch (const std::exception& e) {
        // On error, output error event
        Upp::ValueMap error_event;
        error_event.Add("event", "error");
        error_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        error_event.Add("message", Upp::String(std::string("Error in process logs stream: ") + e.what()));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(error_event) << std::endl;

        return ProtoVMCLI::JsonIO::ErrorResponse(
            "Failed to stream process logs: " + std::string(e.what()),
            "PROCESS_LOGS_STREAM_ERROR");
    }
}

Upp::String CommandDispatcher::RunDebugWebSocketStream(const Upp::String& ws_id) {
    try {
        // Output initial status
        Upp::ValueMap status_event;
        status_event.Add("event", "status");
        status_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        status_event.Add("message", Upp::String("Starting WebSocket frames stream for ID: " + ws_id.ToStd()));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(status_event) << std::endl;

        // Simulate WebSocket frame events
        for (int i = 0; i < 5; ++i) {  // Generate 5 frame events
            Upp::ValueMap frame_event;
            frame_event.Add("event", "frame");
            frame_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
            Upp::ValueMap frame_data;
            frame_data.Add("direction", i % 2 == 0 ? "inbound" : "outbound");
            frame_data.Add("data", Upp::String("Frame data " + Upp::AsString(i+1)));
            frame_data.Add("type", "text");
            frame_data.Add("websocket_id", ws_id);
            frame_data.Add("size", 12 + i);  // Simulate varying sizes
            frame_event.Add("data", frame_data);

            std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(frame_event) << std::endl;

            // Sleep briefly to simulate real-time streaming
            Upp::Sleep(150); // 150ms delay
        }

        // Output end event
        Upp::ValueMap end_event;
        end_event.Add("event", "end");
        end_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(end_event) << std::endl;

        // Return final status
        Upp::ValueMap response_data;
        response_data.Add("stream", "completed");
        response_data.Add("websocket_id", ws_id);
        return ProtoVMCLI::JsonIO::SuccessResponse(response_data);
    }
    catch (const std::exception& e) {
        // On error, output error event
        Upp::ValueMap error_event;
        error_event.Add("event", "error");
        error_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        error_event.Add("message", Upp::String(std::string("Error in WebSocket frames stream: ") + e.what()));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(error_event) << std::endl;

        return ProtoVMCLI::JsonIO::ErrorResponse(
            "Failed to stream WebSocket frames: " + std::string(e.what()),
            "WEBSOCKET_STREAM_ERROR");
    }
}

Upp::String CommandDispatcher::RunDebugPollStream(const Upp::String& poll_id) {
    try {
        // Output initial status
        Upp::ValueMap status_event;
        status_event.Add("event", "status");
        status_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        status_event.Add("message", Upp::String("Starting poll events stream for ID: " + poll_id.ToStd()));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(status_event) << std::endl;

        // Simulate poll events
        for (int i = 0; i < 5; ++i) {  // Generate 5 poll events
            Upp::ValueMap poll_event;
            poll_event.Add("event", "poll");
            poll_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
            Upp::ValueMap poll_data;
            poll_data.Add("type", "request");
            poll_data.Add("data", Upp::String("Poll request " + Upp::AsString(i+1)));
            poll_data.Add("poll_id", poll_id);
            poll_data.Add("sequence", i + 1);
            poll_event.Add("data", poll_data);

            std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(poll_event) << std::endl;

            // Sleep briefly to simulate real-time streaming
            Upp::Sleep(120); // 120ms delay
        }

        for (int i = 0; i < 5; ++i) {  // Generate 5 poll response events
            Upp::ValueMap poll_event;
            poll_event.Add("event", "poll");
            poll_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
            Upp::ValueMap poll_data;
            poll_data.Add("type", "response");
            poll_data.Add("data", Upp::String("Poll response " + Upp::AsString(i+1)));
            poll_data.Add("poll_id", poll_id);
            poll_data.Add("sequence", i + 1);
            poll_data.Add("status", "success");
            poll_event.Add("data", poll_data);

            std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(poll_event) << std::endl;

            // Sleep briefly to simulate real-time streaming
            Upp::Sleep(80); // 80ms delay
        }

        // Output end event
        Upp::ValueMap end_event;
        end_event.Add("event", "end");
        end_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(end_event) << std::endl;

        // Return final status
        Upp::ValueMap response_data;
        response_data.Add("stream", "completed");
        response_data.Add("poll_id", poll_id);
        return ProtoVMCLI::JsonIO::SuccessResponse(response_data);
    }
    catch (const std::exception& e) {
        // On error, output error event
        Upp::ValueMap error_event;
        error_event.Add("event", "error");
        error_event.Add("timestamp", Upp::String( Upp::FormatTime( Upp::GetSysTime(), "%Y-%m-%dT%H:%M:%S.%sZ" )));
        error_event.Add("message", Upp::String(std::string("Error in poll events stream: ") + e.what()));
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(error_event) << std::endl;

        return ProtoVMCLI::JsonIO::ErrorResponse(
            "Failed to stream poll events: " + std::string(e.what()),
            "POLL_STREAM_ERROR");
    }
}

Upp::String CommandDispatcher::RunEditAddComponent(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("edit-add-component", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("edit-add-component", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("edit-add-component", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Parse component details from options
        std::string component_type = opts.circuit_file.has_value() ? opts.circuit_file.value() : "";
        std::string component_name = opts.netlist_file.has_value() ? opts.netlist_file.value() : "";
        int x = opts.ticks.value_or(0);
        int y = opts.pcb_id.value_or(0);

        // For this implementation, we'll get component details from command arguments
        // This would typically come from stdin JSON or additional command-specific args
        if (component_type.empty()) {
            return JsonIO::ErrorResponse("edit-add-component", "Component type is required", "INVALID_ARGUMENT");
        }

        if (component_name.empty()) {
            // Generate a default name if not provided
            component_name = component_type + "_" + std::to_string(metadata.circuit_revision);
        }

        // Create the edit operation
        std::vector<EditOperation> ops;
        EditOperation op;
        op.type = EditOpType::AddComponent;
        op.component_type = component_type;
        op.component_name = component_name;
        op.x = x;
        op.y = y;
        op.revision_base = 0; // For now, not using optimistic concurrency
        ops.push_back(op);

        // Use CircuitFacade to apply the operation
        CircuitFacade circuit_facade;  // Use default constructor
        auto apply_result = circuit_facade.ApplyEditOperations(metadata, session_dir, ops, opts.user_id);

        if (!apply_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(apply_result.error_code);
            return JsonIO::ErrorResponse("edit-add-component", apply_result.error_message, error_code_str);
        }

        // Save updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("edit-add-component", save_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", apply_result.data.revision);
        // In a real implementation, we would return the actual component ID generated
        response_data.Add("component_id", Upp::String("C0000001")); // Placeholder

        return JsonIO::SuccessResponse("edit-add-component", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("edit-add-component",
                                   "Failed to add component: " + std::string(e.what()),
                                   "ADD_COMPONENT_ERROR");
    }
}

Upp::String CommandDispatcher::RunEditRemoveComponent(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("edit-remove-component", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("edit-remove-component", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("edit-remove-component", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Component ID is expected to come from other fields in opts that we can repurpose
        // For a complete implementation, we'd need proper argument parsing for component ID
        std::string component_id_str = opts.circuit_file.has_value() ? opts.circuit_file.value() : "";
        if (component_id_str.empty()) {
            return JsonIO::ErrorResponse("edit-remove-component", "Component ID is required", "INVALID_ARGUMENT");
        }

        CircuitEntityId component_id(component_id_str);

        // Create the edit operation
        std::vector<EditOperation> ops;
        EditOperation op;
        op.type = EditOpType::RemoveComponent;
        op.component_id = component_id;
        op.revision_base = 0; // For now, not using optimistic concurrency
        ops.push_back(op);

        // Use CircuitFacade to apply the operation
        CircuitFacade circuit_facade;  // Use default constructor
        auto apply_result = circuit_facade.ApplyEditOperations(metadata, session_dir, ops, opts.user_id);

        if (!apply_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(apply_result.error_code);
            return JsonIO::ErrorResponse("edit-remove-component", apply_result.error_message, error_code_str);
        }

        // Save updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("edit-remove-component", save_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", apply_result.data.revision);

        return JsonIO::SuccessResponse("edit-remove-component", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("edit-remove-component",
                                   "Failed to remove component: " + std::string(e.what()),
                                   "REMOVE_COMPONENT_ERROR");
    }
}

Upp::String CommandDispatcher::RunEditMoveComponent(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("edit-move-component", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("edit-move-component", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("edit-move-component", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Component ID and position are expected to come from command options
        std::string component_id_str = opts.circuit_file.has_value() ? opts.circuit_file.value() : "";
        int x = opts.ticks.value_or(0);
        int y = opts.pcb_id.value_or(0);

        if (component_id_str.empty()) {
            return JsonIO::ErrorResponse("edit-move-component", "Component ID is required", "INVALID_ARGUMENT");
        }

        CircuitEntityId component_id(component_id_str);

        // Create the edit operation
        std::vector<EditOperation> ops;
        EditOperation op;
        op.type = EditOpType::MoveComponent;
        op.component_id = component_id;
        op.x = x;
        op.y = y;
        op.revision_base = 0; // For now, not using optimistic concurrency
        ops.push_back(op);

        // Use CircuitFacade to apply the operation
        CircuitFacade circuit_facade;  // Use default constructor
        auto apply_result = circuit_facade.ApplyEditOperations(metadata, session_dir, ops, opts.user_id);

        if (!apply_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(apply_result.error_code);
            return JsonIO::ErrorResponse("edit-move-component", apply_result.error_message, error_code_str);
        }

        // Save updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("edit-move-component", save_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", apply_result.data.revision);

        return JsonIO::SuccessResponse("edit-move-component", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("edit-move-component",
                                   "Failed to move component: " + std::string(e.what()),
                                   "MOVE_COMPONENT_ERROR");
    }
}

Upp::String CommandDispatcher::RunEditSetComponentProperty(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("edit-set-component-property", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("edit-set-component-property", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("edit-set-component-property", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Component ID, property name and value are expected to come from command options
        std::string component_id_str = opts.circuit_file.has_value() ? opts.circuit_file.value() : "";
        std::string property_name = opts.netlist_file.has_value() ? opts.netlist_file.value() : "";
        int property_value_int = opts.ticks.value_or(0);
        std::string property_value = std::to_string(property_value_int); // Convert int to string for this example

        if (component_id_str.empty()) {
            return JsonIO::ErrorResponse("edit-set-component-property", "Component ID is required", "INVALID_ARGUMENT");
        }

        if (property_name.empty()) {
            return JsonIO::ErrorResponse("edit-set-component-property", "Property name is required", "INVALID_ARGUMENT");
        }

        CircuitEntityId component_id(component_id_str);

        // Create the edit operation
        std::vector<EditOperation> ops;
        EditOperation op;
        op.type = EditOpType::SetComponentProperty;
        op.component_id = component_id;
        op.property_name = property_name;
        op.property_value = property_value;
        op.revision_base = 0; // For now, not using optimistic concurrency
        ops.push_back(op);

        // Use CircuitFacade to apply the operation
        CircuitFacade circuit_facade;  // Use default constructor
        auto apply_result = circuit_facade.ApplyEditOperations(metadata, session_dir, ops, opts.user_id);

        if (!apply_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(apply_result.error_code);
            return JsonIO::ErrorResponse("edit-set-component-property", apply_result.error_message, error_code_str);
        }

        // Save updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("edit-set-component-property", save_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", apply_result.data.revision);

        return JsonIO::SuccessResponse("edit-set-component-property", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("edit-set-component-property",
                                   "Failed to set component property: " + std::string(e.what()),
                                   "SET_COMPONENT_PROPERTY_ERROR");
    }
}

Upp::String CommandDispatcher::RunEditConnect(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("edit-connect", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("edit-connect", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("edit-connect", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Connection parameters: start component ID and pin, end component ID and pin
        std::string start_component_id_str = opts.circuit_file.has_value() ? opts.circuit_file.value() : "";
        std::string start_pin_name = opts.netlist_file.has_value() ? opts.netlist_file.value() : "";
        int end_component_id_int = opts.ticks.value_or(0);
        int end_pin_name_int = opts.pcb_id.value_or(0);
        std::string end_component_id_str = std::to_string(end_component_id_int);
        std::string end_pin_name = std::to_string(end_pin_name_int);

        if (start_component_id_str.empty() || start_pin_name.empty() ||
            end_component_id_str.empty() || end_pin_name.empty()) {
            return JsonIO::ErrorResponse("edit-connect", "All connection parameters are required", "INVALID_ARGUMENT");
        }

        CircuitEntityId start_component_id(start_component_id_str);
        CircuitEntityId end_component_id(end_component_id_str);

        // Create the edit operation
        std::vector<EditOperation> ops;
        EditOperation op;
        op.type = EditOpType::Connect;
        op.component_id = start_component_id;
        op.pin_name = start_pin_name;
        op.target_component_id = end_component_id;
        op.target_pin_name = end_pin_name;
        op.revision_base = 0; // For now, not using optimistic concurrency
        ops.push_back(op);

        // Use CircuitFacade to apply the operation
        CircuitFacade circuit_facade;  // Use default constructor
        auto apply_result = circuit_facade.ApplyEditOperations(metadata, session_dir, ops, opts.user_id);

        if (!apply_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(apply_result.error_code);
            return JsonIO::ErrorResponse("edit-connect", apply_result.error_message, error_code_str);
        }

        // Save updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("edit-connect", save_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", apply_result.data.revision);

        return JsonIO::SuccessResponse("edit-connect", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("edit-connect",
                                   "Failed to create connection: " + std::string(e.what()),
                                   "CONNECT_ERROR");
    }
}

Upp::String CommandDispatcher::RunEditDisconnect(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("edit-disconnect", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("edit-disconnect", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("edit-disconnect", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Parameters to identify the connection to disconnect
        std::string start_component_id_str = opts.circuit_file.has_value() ? opts.circuit_file.value() : "";
        std::string start_pin_name = opts.netlist_file.has_value() ? opts.netlist_file.value() : "";
        int end_component_id_int = opts.ticks.value_or(0);
        int end_pin_name_int = opts.pcb_id.value_or(0);
        std::string end_component_id_str = std::to_string(end_component_id_int);
        std::string end_pin_name = std::to_string(end_pin_name_int);

        if (start_component_id_str.empty() || start_pin_name.empty() ||
            end_component_id_str.empty() || end_pin_name.empty()) {
            return JsonIO::ErrorResponse("edit-disconnect", "All disconnection parameters are required", "INVALID_ARGUMENT");
        }

        CircuitEntityId start_component_id(start_component_id_str);
        CircuitEntityId end_component_id(end_component_id_str);

        // Create the edit operation
        std::vector<EditOperation> ops;
        EditOperation op;
        op.type = EditOpType::Disconnect;
        op.component_id = start_component_id;
        op.pin_name = start_pin_name;
        op.target_component_id = end_component_id;
        op.target_pin_name = end_pin_name;
        op.revision_base = 0; // For now, not using optimistic concurrency
        ops.push_back(op);

        // Use CircuitFacade to apply the operation
        CircuitFacade circuit_facade;  // Use default constructor
        auto apply_result = circuit_facade.ApplyEditOperations(metadata, session_dir, ops, opts.user_id);

        if (!apply_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(apply_result.error_code);
            return JsonIO::ErrorResponse("edit-disconnect", apply_result.error_message, error_code_str);
        }

        // Save updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("edit-disconnect", save_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", apply_result.data.revision);

        return JsonIO::SuccessResponse("edit-disconnect", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("edit-disconnect",
                                   "Failed to disconnect: " + std::string(e.what()),
                                   "DISCONNECT_ERROR");
    }
}

Upp::String CommandDispatcher::RunEditGetCircuit(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("edit-get-circuit", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("edit-get-circuit", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("edit-get-circuit", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Use CircuitFacade to get the current circuit state
        CircuitFacade circuit_facade;  // Use default constructor
        auto export_result = circuit_facade.ExportCircuitState(metadata, session_dir);

        if (!export_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(export_result.error_code);
            return JsonIO::ErrorResponse("edit-get-circuit", export_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", export_result.data.revision);
        response_data.Add("circuit_data", export_result.data.circuit_json);

        return JsonIO::SuccessResponse("edit-get-circuit", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("edit-get-circuit",
                                   "Failed to get circuit: " + std::string(e.what()),
                                   "GET_CIRCUIT_ERROR");
    }
}

Upp::String CommandDispatcher::RunLintCircuit(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("lint-circuit", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("lint-circuit", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("lint-circuit", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Load the current circuit state using CircuitFacade
        CircuitData circuit;
        CircuitFacade circuit_facade;
        auto load_circuit_result = circuit_facade.LoadCurrentCircuit(metadata, session_dir, circuit);

        if (!load_circuit_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_circuit_result.error_code);
            return JsonIO::ErrorResponse("lint-circuit", load_circuit_result.error_message, error_code_str);
        }

        // Run circuit analysis
        CircuitAnalysis analysis;
        auto analysis_result = analysis.AnalyzeCircuit(circuit);

        if (!analysis_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(analysis_result.error_code);
            return JsonIO::ErrorResponse("lint-circuit", analysis_result.error_message, error_code_str);
        }

        // Build response with diagnostics
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", metadata.circuit_revision);
        response_data.Add("diagnostics", JsonIO::CircuitDiagnosticsToValueArray(analysis_result.data));

        return JsonIO::SuccessResponse("lint-circuit", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("lint-circuit",
                                   "Failed to lint circuit: " + std::string(e.what()),
                                   "CIRCUIT_LINT_ERROR");
    }
}

Upp::String CommandDispatcher::RunAnalyzeCircuit(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("analyze-circuit", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("analyze-circuit", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("analyze-circuit", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Load the current circuit state using CircuitFacade
        CircuitData circuit;
        CircuitFacade circuit_facade;
        auto load_circuit_result = circuit_facade.LoadCurrentCircuit(metadata, session_dir, circuit);

        if (!load_circuit_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_circuit_result.error_code);
            return JsonIO::ErrorResponse("analyze-circuit", load_circuit_result.error_message, error_code_str);
        }

        // Run circuit analysis
        CircuitAnalysis analysis;
        auto analysis_result = analysis.AnalyzeCircuit(circuit);

        if (!analysis_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(analysis_result.error_code);
            return JsonIO::ErrorResponse("analyze-circuit", analysis_result.error_message, error_code_str);
        }

        // Build response with diagnostics and summary
        Upp::ValueMap summary;
        summary.Add("component_count", static_cast<int>(circuit.components.size()));
        summary.Add("net_count", static_cast<int>(circuit.wires.size()));

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", metadata.circuit_revision);
        response_data.Add("summary", summary);
        response_data.Add("diagnostics", JsonIO::CircuitDiagnosticsToValueArray(analysis_result.data));

        return JsonIO::SuccessResponse("analyze-circuit", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("analyze-circuit",
                                   "Failed to analyze circuit: " + std::string(e.what()),
                                   "CIRCUIT_ANALYSIS_ERROR");
    }
}

Upp::String CommandDispatcher::RunCircuitDiff(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("circuit-diff", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("circuit-diff", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("circuit-diff", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Get revision numbers from command options
        int from_revision = opts.ticks.value_or(0);  // Using ticks field for from_revision
        int to_revision = opts.pcb_id.value_or(metadata.circuit_revision);  // Using pcb_id field for to_revision

        // For now, we'll return an empty diff - a full implementation would calculate
        // the actual difference between the two revisions
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("from_revision", from_revision);
        response_data.Add("to_revision", to_revision);

        Upp::ValueArray diff_ops;
        response_data.Add("diff", diff_ops);

        return JsonIO::SuccessResponse("circuit-diff", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("circuit-diff",
                                   "Failed to generate circuit diff: " + std::string(e.what()),
                                   "CIRCUIT_DIFF_ERROR");
    }
}

Upp::String CommandDispatcher::RunCircuitPatch(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("circuit-patch", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("circuit-patch", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("circuit-patch", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // For now, we'll return a basic response - a full implementation would apply
        // the patch operations to the circuit
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_revision", metadata.circuit_revision + 1);
        response_data.Add("applied", true);

        return JsonIO::SuccessResponse("circuit-patch", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("circuit-patch",
                                   "Failed to apply circuit patch: " + std::string(e.what()),
                                   "CIRCUIT_PATCH_ERROR");
    }
}

Upp::String CommandDispatcher::RunCircuitReplay(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("circuit-replay", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("circuit-replay", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("circuit-replay", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Get the revision to replay to
        int target_revision = opts.ticks.value_or(metadata.circuit_revision);  // Using ticks field for revision

        // Use CircuitFacade to load circuit state at specific revision
        CircuitData circuit_at_revision;
        CircuitFacade circuit_facade;

        // Temporarily adjust the metadata to load a specific revision
        SessionMetadata temp_metadata = metadata;
        temp_metadata.circuit_revision = target_revision;

        auto load_circuit_result = circuit_facade.LoadCurrentCircuit(temp_metadata, session_dir, circuit_at_revision);

        if (!load_circuit_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_circuit_result.error_code);
            return JsonIO::ErrorResponse("circuit-replay", load_circuit_result.error_message, error_code_str);
        }

        // Export the circuit state at the target revision
        auto export_result = circuit_facade.ExportCircuitState(temp_metadata, session_dir);
        if (!export_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(export_result.error_code);
            return JsonIO::ErrorResponse("circuit-replay", export_result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("revision", target_revision);
        response_data.Add("circuit_data", export_result.data.circuit_json);

        return JsonIO::SuccessResponse("circuit-replay", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("circuit-replay",
                                   "Failed to replay circuit: " + std::string(e.what()),
                                   "CIRCUIT_REPLAY_ERROR");
    }
}

Upp::String CommandDispatcher::RunCircuitHistory(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("circuit-history", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("circuit-history", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("circuit-history", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use
        std::string branch_name = opts.branch.value_or(metadata.current_branch);
        std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
        if (!branch_opt.has_value()) {
            return JsonIO::ErrorResponse("circuit-history", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
        }

        int64_t branch_revision = branch_opt->head_revision;

        // For now, we'll return basic history information - a complete implementation
        // would read the event log and generate a detailed history
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("current_revision", branch_revision);
        response_data.Add("total_operations", branch_revision);  // Simplified: each edit increases revision by 1

        Upp::ValueArray history_entries;
        for (int64_t i = 0; i <= branch_revision; ++i) {
            Upp::ValueMap entry;
            entry.Add("revision", i);
            entry.Add("timestamp", Upp::String(GetCurrentTimestamp().c_str()));
            entry.Add("user", Upp::String("system"));
            entry.Add("operation", Upp::String("initial_state"));  // Placeholder
            history_entries.Add(entry);
        }
        response_data.Add("history", history_entries);

        return JsonIO::SuccessResponse("circuit-history", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("circuit-history",
                                   "Failed to get circuit history: " + std::string(e.what()),
                                   "CIRCUIT_HISTORY_ERROR");
    }
}

Upp::String CommandDispatcher::RunBranchList(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("branch-list", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("branch-list", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("branch-list", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;

        // Use BranchOperations to list branches
        auto list_result = BranchOperations::ListBranches(metadata);
        if (!list_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(list_result.error_code);
            return JsonIO::ErrorResponse("branch-list", list_result.error_message, error_code_str);
        }

        // Convert the result to the response format
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("current_branch", Upp::String(list_result.data.current_branch.c_str()));

        Upp::ValueArray branches_array;
        for (const auto& branch : list_result.data.branches) {
            Upp::ValueMap branch_map;
            branch_map.Add("name", Upp::String(branch.name.c_str()));
            branch_map.Add("head_revision", branch.head_revision);
            branch_map.Add("sim_revision", branch.sim_revision);
            branch_map.Add("base_revision", branch.base_revision);
            branch_map.Add("is_default", branch.is_default);
            branches_array.Add(branch_map);
        }
        response_data.Add("branches", branches_array);

        return JsonIO::SuccessResponse("branch-list", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("branch-list",
                                   "Failed to list branches: " + std::string(e.what()),
                                   "BRANCH_LIST_ERROR");
    }
}

Upp::String CommandDispatcher::RunBranchCreate(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("branch-create", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("branch-create", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("branch-create", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;

        // Get the branch name from command options
        if (!opts.branch_name.has_value()) {
            return JsonIO::ErrorResponse("branch-create", "Branch name is required", "INVALID_ARGUMENT");
        }

        std::string branch_name = opts.branch_name.value();
        std::string from_branch = opts.branch_from.value_or(metadata.current_branch); // Default to current branch

        // Use BranchOperations to create the branch
        auto create_result = BranchOperations::CreateBranch(metadata, branch_name, from_branch);
        if (!create_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(create_result.error_code);
            return JsonIO::ErrorResponse("branch-create", create_result.error_message, error_code_str);
        }

        // Save the updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("branch-create", save_result.error_message, error_code_str);
        }

        // Convert the result to the response format
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());

        Upp::ValueMap branch_map;
        branch_map.Add("name", Upp::String(create_result.data.branch.name.c_str()));
        branch_map.Add("head_revision", create_result.data.branch.head_revision);
        branch_map.Add("sim_revision", create_result.data.branch.sim_revision);
        branch_map.Add("base_revision", create_result.data.branch.base_revision);
        branch_map.Add("is_default", create_result.data.branch.is_default);
        response_data.Add("branch", branch_map);

        return JsonIO::SuccessResponse("branch-create", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("branch-create",
                                   "Failed to create branch: " + std::string(e.what()),
                                   "BRANCH_CREATE_ERROR");
    }
}

Upp::String CommandDispatcher::RunBranchSwitch(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("branch-switch", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("branch-switch", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("branch-switch", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;

        if (!opts.branch_name.has_value()) {
            return JsonIO::ErrorResponse("branch-switch", "Branch name is required", "INVALID_ARGUMENT");
        }

        std::string branch_name = opts.branch_name.value();

        // Use BranchOperations to switch the branch
        auto switch_result = BranchOperations::SwitchBranch(metadata, branch_name);
        if (!switch_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(switch_result.error_code);
            return JsonIO::ErrorResponse("branch-switch", switch_result.error_message, error_code_str);
        }

        // Save the updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("branch-switch", save_result.error_message, error_code_str);
        }

        // Convert the result to the response format
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("current_branch", Upp::String(switch_result.data.current_branch.c_str()));

        return JsonIO::SuccessResponse("branch-switch", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("branch-switch",
                                   "Failed to switch branch: " + std::string(e.what()),
                                   "BRANCH_SWITCH_ERROR");
    }
}

Upp::String CommandDispatcher::RunBranchDelete(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("branch-delete", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("branch-delete", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("branch-delete", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;

        if (!opts.branch_name.has_value()) {
            return JsonIO::ErrorResponse("branch-delete", "Branch name is required", "INVALID_ARGUMENT");
        }

        std::string branch_name = opts.branch_name.value();

        // Use BranchOperations to delete the branch
        auto delete_result = BranchOperations::DeleteBranch(metadata, branch_name);
        if (!delete_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(delete_result.error_code);
            return JsonIO::ErrorResponse("branch-delete", delete_result.error_message, error_code_str);
        }

        // Save the updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("branch-delete", save_result.error_message, error_code_str);
        }

        // Convert the result to the response format
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("deleted_branch", Upp::String(delete_result.data.deleted_branch.c_str()));

        return JsonIO::SuccessResponse("branch-delete", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("branch-delete",
                                   "Failed to delete branch: " + std::string(e.what()),
                                   "BRANCH_DELETE_ERROR");
    }
}

Upp::String CommandDispatcher::RunBranchMerge(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("branch-merge", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("branch-merge", "Session ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Load the session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("branch-merge", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;

        if (!opts.branch_from.has_value() || !opts.branch_to.has_value()) {
            return JsonIO::ErrorResponse("branch-merge", "Both source and target branches are required", "INVALID_ARGUMENT");
        }

        std::string source_branch = opts.branch_from.value();
        std::string target_branch = opts.branch_to.value();

        // Use BranchOperations to merge the branches
        auto merge_result = BranchOperations::MergeBranch(metadata, source_branch, target_branch);
        if (!merge_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(merge_result.error_code);
            return JsonIO::ErrorResponse("branch-merge", merge_result.error_message, error_code_str);
        }

        // Save the updated session metadata
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(save_result.error_code);
            return JsonIO::ErrorResponse("branch-merge", save_result.error_message, error_code_str);
        }

        // Convert the result to the response format
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("source_branch", Upp::String(merge_result.data.source_branch.c_str()));
        response_data.Add("target_branch", Upp::String(merge_result.data.target_branch.c_str()));
        response_data.Add("target_new_revision", merge_result.data.target_new_revision);
        response_data.Add("merged_ops_count", merge_result.data.merged_ops_count);

        return JsonIO::SuccessResponse("branch-merge", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("branch-merge",
                                   "Failed to merge branches: " + std::string(e.what()),
                                   "BRANCH_MERGE_ERROR");
    }
}

    // Graph commands
    Upp::String CommandDispatcher::RunGraphExport(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("graph-export", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("graph-export", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("graph-export", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for graph
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("graph-export", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Load the circuit for the specified branch
            CircuitData circuit;
            CircuitFacade circuit_facade;
            auto load_circuit_result = circuit_facade.LoadCurrentCircuitForBranch(
                metadata, session_dir, branch_name, circuit);

            if (!load_circuit_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_circuit_result.error_code);
                return JsonIO::ErrorResponse("graph-export", load_circuit_result.error_message, error_code_str);
            }

            // Build the graph
            CircuitGraphBuilder builder;
            auto graph_result = builder.BuildGraph(circuit);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("graph-export", graph_result.error_message, error_code_str);
            }

            // Convert the graph to JSON format for response
            const CircuitGraph& graph = graph_result.data;

            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            Upp::ValueArray nodes_array;
            for (const auto& node : graph.nodes) {
                Upp::ValueMap node_map;
                node_map.Add("kind", Upp::String(
                    node.kind == GraphNodeKind::Component ? "Component" :
                    node.kind == GraphNodeKind::Pin ? "Pin" : "Net"));
                node_map.Add("id", Upp::String(node.id.c_str()));
                nodes_array.Add(node_map);
            }
            response_data.Add("graph", Upp::ValueMap().Add("nodes", nodes_array));

            // Add edges to response
            Upp::ValueArray edges_array;
            for (const auto& edge : graph.edges) {
                Upp::ValueMap edge_map;

                Upp::ValueMap from_map;
                from_map.Add("kind", Upp::String(
                    edge.from.kind == GraphNodeKind::Component ? "Component" :
                    edge.from.kind == GraphNodeKind::Pin ? "Pin" : "Net"));
                from_map.Add("id", Upp::String(edge.from.id.c_str()));

                Upp::ValueMap to_map;
                to_map.Add("kind", Upp::String(
                    edge.to.kind == GraphNodeKind::Component ? "Component" :
                    edge.to.kind == GraphNodeKind::Pin ? "Pin" : "Net"));
                to_map.Add("id", Upp::String(edge.to.id.c_str()));

                edge_map.Add("from", from_map);
                edge_map.Add("to", to_map);
                edge_map.Add("kind", Upp::String(
                    edge.kind == GraphEdgeKind::Connectivity ? "Connectivity" : "SignalFlow"));

                edges_array.Add(edge_map);
            }

            Upp::ValueMap graph_map = response_data.Get("graph", Upp::ValueMap());
            graph_map.Add("edges", edges_array);
            response_data.Set("graph", graph_map);

            return JsonIO::SuccessResponse("graph-export", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("graph-export",
                                       "Failed to export graph: " + std::string(e.what()),
                                       "GRAPH_EXPORT_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunGraphPaths(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("graph-paths", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("graph-paths", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("graph-paths", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for graph
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("graph-paths", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Load the circuit for the specified branch
            CircuitData circuit;
            CircuitFacade circuit_facade;
            auto load_circuit_result = circuit_facade.LoadCurrentCircuitForBranch(
                metadata, session_dir, branch_name, circuit);

            if (!load_circuit_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_circuit_result.error_code);
                return JsonIO::ErrorResponse("graph-paths", load_circuit_result.error_message, error_code_str);
            }

            // Build the graph
            CircuitGraphBuilder builder;
            auto graph_result = builder.BuildGraph(circuit);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("graph-paths", graph_result.error_message, error_code_str);
            }

            // Get source and target from command options
            GraphNodeKind source_kind = GraphNodeKind::Pin;  // Default
            std::string source_id = "C1:OUT";  // Default placeholder
            GraphNodeKind target_kind = GraphNodeKind::Pin;  // Default
            std::string target_id = "C2:IN";  // Default placeholder
            int max_depth = 128;  // Default

            if (opts.graph_source_kind.has_value()) {
                std::string kind_str = opts.graph_source_kind.value();
                if (kind_str == "Component") source_kind = GraphNodeKind::Component;
                else if (kind_str == "Pin") source_kind = GraphNodeKind::Pin;
                else if (kind_str == "Net") source_kind = GraphNodeKind::Net;
            }

            if (opts.graph_source_id.has_value()) {
                source_id = opts.graph_source_id.value();
            }

            if (opts.graph_target_kind.has_value()) {
                std::string kind_str = opts.graph_target_kind.value();
                if (kind_str == "Component") target_kind = GraphNodeKind::Component;
                else if (kind_str == "Pin") target_kind = GraphNodeKind::Pin;
                else if (kind_str == "Net") target_kind = GraphNodeKind::Net;
            }

            if (opts.graph_target_id.has_value()) {
                target_id = opts.graph_target_id.value();
            }

            if (opts.graph_max_depth.has_value()) {
                max_depth = opts.graph_max_depth.value();
            }

            // Create the source and target nodes
            GraphNodeId source_node(source_kind, source_id);
            GraphNodeId target_node(target_kind, target_id);

            // Find paths
            CircuitGraphQueries queries;
            auto paths_result = queries.FindSignalPaths(graph_result.data, source_node, target_node, max_depth);
            if (!paths_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(paths_result.error_code);
                return JsonIO::ErrorResponse("graph-paths", paths_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            Upp::ValueArray paths_array;
            for (const auto& path : paths_result.data) {
                Upp::ValueArray path_nodes_array;
                for (const auto& node : path.nodes) {
                    Upp::ValueMap node_map;
                    node_map.Add("kind", Upp::String(
                        node.kind == GraphNodeKind::Component ? "Component" :
                        node.kind == GraphNodeKind::Pin ? "Pin" : "Net"));
                    node_map.Add("id", Upp::String(node.id.c_str()));
                    path_nodes_array.Add(node_map);
                }

                Upp::ValueMap path_map;
                path_map.Add("nodes", path_nodes_array);
                paths_array.Add(path_map);
            }
            response_data.Add("paths", paths_array);

            return JsonIO::SuccessResponse("graph-paths", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("graph-paths",
                                       "Failed to find paths: " + std::string(e.what()),
                                       "GRAPH_PATHS_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunGraphFanIn(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("graph-fanin", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("graph-fanin", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("graph-fanin", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for graph
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("graph-fanin", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Load the circuit for the specified branch
            CircuitData circuit;
            CircuitFacade circuit_facade;
            auto load_circuit_result = circuit_facade.LoadCurrentCircuitForBranch(
                metadata, session_dir, branch_name, circuit);

            if (!load_circuit_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_circuit_result.error_code);
                return JsonIO::ErrorResponse("graph-fanin", load_circuit_result.error_message, error_code_str);
            }

            // Build the graph
            CircuitGraphBuilder builder;
            auto graph_result = builder.BuildGraph(circuit);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("graph-fanin", graph_result.error_message, error_code_str);
            }

            // Get target node from command options
            GraphNodeKind node_kind = GraphNodeKind::Pin;  // Default
            std::string node_id = "C2:IN";  // Default placeholder
            int max_depth = 128;  // Default

            if (opts.graph_node_kind.has_value()) {
                std::string kind_str = opts.graph_node_kind.value();
                if (kind_str == "Component") node_kind = GraphNodeKind::Component;
                else if (kind_str == "Pin") node_kind = GraphNodeKind::Pin;
                else if (kind_str == "Net") node_kind = GraphNodeKind::Net;
            }

            if (opts.graph_node_id.has_value()) {
                node_id = opts.graph_node_id.value();
            }

            if (opts.graph_max_depth.has_value()) {
                max_depth = opts.graph_max_depth.value();
            }

            // Create the target node
            GraphNodeId target_node(node_kind, node_id);

            // Find fan-in
            CircuitGraphQueries queries;
            auto fanin_result = queries.FindFanIn(graph_result.data, target_node, max_depth);
            if (!fanin_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(fanin_result.error_code);
                return JsonIO::ErrorResponse("graph-fanin", fanin_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            Upp::ValueMap node_map;
            node_map.Add("kind", Upp::String(
                target_node.kind == GraphNodeKind::Component ? "Component" :
                target_node.kind == GraphNodeKind::Pin ? "Pin" : "Net"));
            node_map.Add("id", Upp::String(target_node.id.c_str()));
            response_data.Add("node", node_map);

            Upp::ValueArray endpoints_array;
            for (const auto& endpoint : fanin_result.data.endpoints) {
                Upp::ValueMap endpoint_map;
                endpoint_map.Add("kind", Upp::String(
                    endpoint.kind == GraphNodeKind::Component ? "Component" :
                    endpoint.kind == GraphNodeKind::Pin ? "Pin" : "Net"));
                endpoint_map.Add("id", Upp::String(endpoint.id.c_str()));
                endpoints_array.Add(endpoint_map);
            }
            response_data.Add("endpoints", endpoints_array);

            return JsonIO::SuccessResponse("graph-fanin", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("graph-fanin",
                                       "Failed to find fan-in: " + std::string(e.what()),
                                       "GRAPH_FANIN_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunGraphFanOut(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("graph-fanout", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("graph-fanout", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("graph-fanout", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for graph
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("graph-fanout", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Load the circuit for the specified branch
            CircuitData circuit;
            CircuitFacade circuit_facade;
            auto load_circuit_result = circuit_facade.LoadCurrentCircuitForBranch(
                metadata, session_dir, branch_name, circuit);

            if (!load_circuit_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_circuit_result.error_code);
                return JsonIO::ErrorResponse("graph-fanout", load_circuit_result.error_message, error_code_str);
            }

            // Build the graph
            CircuitGraphBuilder builder;
            auto graph_result = builder.BuildGraph(circuit);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("graph-fanout", graph_result.error_message, error_code_str);
            }

            // Get source node from command options
            GraphNodeKind node_kind = GraphNodeKind::Pin;  // Default
            std::string node_id = "C1:OUT";  // Default placeholder
            int max_depth = 128;  // Default

            if (opts.graph_node_kind.has_value()) {
                std::string kind_str = opts.graph_node_kind.value();
                if (kind_str == "Component") node_kind = GraphNodeKind::Component;
                else if (kind_str == "Pin") node_kind = GraphNodeKind::Pin;
                else if (kind_str == "Net") node_kind = GraphNodeKind::Net;
            }

            if (opts.graph_node_id.has_value()) {
                node_id = opts.graph_node_id.value();
            }

            if (opts.graph_max_depth.has_value()) {
                max_depth = opts.graph_max_depth.value();
            }

            // Create the source node
            GraphNodeId source_node(node_kind, node_id);

            // Find fan-out
            CircuitGraphQueries queries;
            auto fanout_result = queries.FindFanOut(graph_result.data, source_node, max_depth);
            if (!fanout_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(fanout_result.error_code);
                return JsonIO::ErrorResponse("graph-fanout", fanout_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            Upp::ValueMap node_map;
            node_map.Add("kind", Upp::String(
                source_node.kind == GraphNodeKind::Component ? "Component" :
                source_node.kind == GraphNodeKind::Pin ? "Pin" : "Net"));
            node_map.Add("id", Upp::String(source_node.id.c_str()));
            response_data.Add("node", node_map);

            Upp::ValueArray endpoints_array;
            for (const auto& endpoint : fanout_result.data.endpoints) {
                Upp::ValueMap endpoint_map;
                endpoint_map.Add("kind", Upp::String(
                    endpoint.kind == GraphNodeKind::Component ? "Component" :
                    endpoint.kind == GraphNodeKind::Pin ? "Pin" : "Net"));
                endpoint_map.Add("id", Upp::String(endpoint.id.c_str()));
                endpoints_array.Add(endpoint_map);
            }
            response_data.Add("endpoints", endpoints_array);

            return JsonIO::SuccessResponse("graph-fanout", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("graph-fanout",
                                       "Failed to find fan-out: " + std::string(e.what()),
                                       "GRAPH_FANOUT_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunGraphStats(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("graph-stats", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("graph-stats", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("graph-stats", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for graph
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("graph-stats", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Load the circuit for the specified branch
            CircuitData circuit;
            CircuitFacade circuit_facade;
            auto load_circuit_result = circuit_facade.LoadCurrentCircuitForBranch(
                metadata, session_dir, branch_name, circuit);

            if (!load_circuit_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_circuit_result.error_code);
                return JsonIO::ErrorResponse("graph-stats", load_circuit_result.error_message, error_code_str);
            }

            // Build the graph
            CircuitGraphBuilder builder;
            auto graph_result = builder.BuildGraph(circuit);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("graph-stats", graph_result.error_message, error_code_str);
            }

            // Get stats
            CircuitGraphQueries queries;
            int node_count = 0, edge_count = 0;
            auto stats_result = queries.ComputeGraphStats(graph_result.data, node_count, edge_count);
            if (!stats_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(stats_result.error_code);
                return JsonIO::ErrorResponse("graph-stats", stats_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("node_count", node_count);
            response_data.Add("edge_count", edge_count);

            return JsonIO::SuccessResponse("graph-stats", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("graph-stats",
                                       "Failed to compute stats: " + std::string(e.what()),
                                       "GRAPH_STATS_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunTimingSummary(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("timing-summary", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("timing-summary", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("timing-summary", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for timing analysis
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("timing-summary", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Load timing graph for the specified branch
            CircuitFacade circuit_facade;
            auto timing_graph_result = circuit_facade.BuildTimingGraphForBranch(
                metadata, session_dir, branch_name);

            if (!timing_graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(timing_graph_result.error_code);
                return JsonIO::ErrorResponse("timing-summary", timing_graph_result.error_message, error_code_str);
            }

            // Perform timing analysis
            TimingAnalysis timing_analysis;
            auto summary_result = timing_analysis.ComputeTimingSummary(
                timing_graph_result.data.first,  // nodes
                timing_graph_result.data.second  // edges
            );

            if (!summary_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(summary_result.error_code);
                return JsonIO::ErrorResponse("timing-summary", summary_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("max_depth", summary_result.data.max_depth);
            response_data.Add("path_count", summary_result.data.path_count);

            return JsonIO::SuccessResponse("timing-summary", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("timing-summary",
                                       "Failed to compute timing summary: " + std::string(e.what()),
                                       "TIMING_SUMMARY_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunTimingCriticalPaths(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("timing-critical-paths", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("timing-critical-paths", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("timing-critical-paths", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for timing analysis
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("timing-critical-paths", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Get max_paths and max_depth from options
            int max_paths = 5; // default
            if (opts.payload.IsKey("max_paths")) {
                Upp::Value max_paths_val = opts.payload.Get("max_paths");
                if (max_paths_val.Is<int>()) {
                    max_paths = max_paths_val.Get<int>();
                }
            }

            int max_depth = 1024; // default
            if (opts.payload.IsKey("max_depth")) {
                Upp::Value max_depth_val = opts.payload.Get("max_depth");
                if (max_depth_val.Is<int>()) {
                    max_depth = max_depth_val.Get<int>();
                }
            }

            // Load timing graph for the specified branch
            CircuitFacade circuit_facade;
            auto timing_graph_result = circuit_facade.BuildTimingGraphForBranch(
                metadata, session_dir, branch_name);

            if (!timing_graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(timing_graph_result.error_code);
                return JsonIO::ErrorResponse("timing-critical-paths", timing_graph_result.error_message, error_code_str);
            }

            // Perform timing analysis for critical paths
            TimingAnalysis timing_analysis;
            auto paths_result = timing_analysis.ComputeCriticalPaths(
                timing_graph_result.data.first,  // nodes
                timing_graph_result.data.second, // edges
                max_paths,
                max_depth
            );

            if (!paths_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(paths_result.error_code);
                return JsonIO::ErrorResponse("timing-critical-paths", paths_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("paths", JsonIO::TimingPathsToValueArray(paths_result.data));

            return JsonIO::SuccessResponse("timing-critical-paths", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("timing-critical-paths",
                                       "Failed to compute critical paths: " + std::string(e.what()),
                                       "TIMING_CRITICAL_PATHS_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunTimingLoops(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("timing-loops", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("timing-loops", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("timing-loops", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for timing analysis
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("timing-loops", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Load timing graph for the specified branch
            CircuitFacade circuit_facade;
            auto timing_graph_result = circuit_facade.BuildTimingGraphForBranch(
                metadata, session_dir, branch_name);

            if (!timing_graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(timing_graph_result.error_code);
                return JsonIO::ErrorResponse("timing-loops", timing_graph_result.error_message, error_code_str);
            }

            // Perform timing analysis to detect loops
            TimingAnalysis timing_analysis;
            auto loops_result = timing_analysis.DetectCombinationalLoops(
                timing_graph_result.data.first,  // nodes
                timing_graph_result.data.second  // edges
            );

            if (!loops_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(loops_result.error_code);
                return JsonIO::ErrorResponse("timing-loops", loops_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("loops", JsonIO::TimingLoopsToValueArray(loops_result.data));

            return JsonIO::SuccessResponse("timing-loops", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("timing-loops",
                                       "Failed to detect loops: " + std::string(e.what()),
                                       "TIMING_LOOPS_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunTimingHazards(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("timing-hazards", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("timing-hazards", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("timing-hazards", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for timing analysis
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("timing-hazards", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Get max_results from options
            int max_results = 64; // default
            if (opts.payload.IsKey("max_results")) {
                Upp::Value max_results_val = opts.payload.Get("max_results");
                if (max_results_val.Is<int>()) {
                    max_results = max_results_val.Get<int>();
                }
            }

            // Load timing graph for the specified branch
            CircuitFacade circuit_facade;
            auto timing_graph_result = circuit_facade.BuildTimingGraphForBranch(
                metadata, session_dir, branch_name);

            if (!timing_graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(timing_graph_result.error_code);
                return JsonIO::ErrorResponse("timing-hazards", timing_graph_result.error_message, error_code_str);
            }

            // Perform hazard analysis
            TimingAnalysis timing_analysis;
            auto hazards_result = timing_analysis.DetectReconvergentFanoutHazards(
                timing_graph_result.data.first,  // nodes
                timing_graph_result.data.second, // edges
                max_results
            );

            if (!hazards_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(hazards_result.error_code);
                return JsonIO::ErrorResponse("timing-hazards", hazards_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("hazards", JsonIO::HazardCandidatesToValueArray(hazards_result.data));

            return JsonIO::SuccessResponse("timing-hazards", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("timing-hazards",
                                       "Failed to detect hazards: " + std::string(e.what()),
                                       "TIMING_HAZARDS_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunDepsSummary(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("deps-summary", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("deps-summary", "Session ID is required", "INVALID_ARGUMENT");
        }

        // Extract node parameters
        std::string node_id = opts.deps_node_id;
        std::string node_kind = opts.deps_node_kind;
        int max_depth = opts.deps_max_depth;

        if (node_id.empty()) {
            // Try to get node_id from payload if not in opts
            if (opts.payload.IsKey("node_id")) {
                node_id = opts.payload.Get("node_id").ToString().ToStd();
            }
        }

        if (node_id.empty()) {
            return JsonIO::ErrorResponse("deps-summary", "Node ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("deps-summary", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for analysis
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("deps-summary", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Resolve the functional node
            CircuitFacade circuit_facade;
            auto graph_result = circuit_facade.BuildGraphForBranch(metadata, session_dir, branch_name);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("deps-summary", graph_result.error_message, error_code_str);
            }

            auto resolve_result = ResolveFunctionalNode(graph_result.data, node_id, node_kind);
            if (!resolve_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(resolve_result.error_code);
                return JsonIO::ErrorResponse("deps-summary", resolve_result.error_message, error_code_str);
            }

            FunctionalNodeId func_node = resolve_result.data;

            // Perform dependency summary computation
            auto summary_result = circuit_facade.BuildDependencySummaryForBranch(
                metadata, session_dir, branch_name, func_node, max_depth);

            if (!summary_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(summary_result.error_code);
                return JsonIO::ErrorResponse("deps-summary", summary_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("root", JsonIO::FunctionalNodeIdToValueMap(summary_result.data.root));
            response_data.Add("upstream_count", summary_result.data.upstream_count);
            response_data.Add("downstream_count", summary_result.data.downstream_count);

            return JsonIO::SuccessResponse("deps-summary", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("deps-summary",
                                       "Failed to compute dependency summary: " + std::string(e.what()),
                                       "DEPS_SUMMARY_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunDepsBackward(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("deps-backward", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("deps-backward", "Session ID is required", "INVALID_ARGUMENT");
        }

        // Extract node parameters
        std::string node_id = opts.deps_node_id;
        std::string node_kind = opts.deps_node_kind;
        int max_depth = opts.deps_max_depth;

        if (node_id.empty()) {
            // Try to get node_id from payload if not in opts
            if (opts.payload.IsKey("node_id")) {
                node_id = opts.payload.Get("node_id").ToString().ToStd();
            }
        }

        if (node_id.empty()) {
            return JsonIO::ErrorResponse("deps-backward", "Node ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("deps-backward", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for analysis
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("deps-backward", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Resolve the functional node
            CircuitFacade circuit_facade;
            auto graph_result = circuit_facade.BuildGraphForBranch(metadata, session_dir, branch_name);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("deps-backward", graph_result.error_message, error_code_str);
            }

            auto resolve_result = ResolveFunctionalNode(graph_result.data, node_id, node_kind);
            if (!resolve_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(resolve_result.error_code);
                return JsonIO::ErrorResponse("deps-backward", resolve_result.error_message, error_code_str);
            }

            FunctionalNodeId func_node = resolve_result.data;

            // Perform backward cone computation
            auto cone_result = circuit_facade.BuildBackwardConeForBranch(
                metadata, session_dir, branch_name, func_node, max_depth);

            if (!cone_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(cone_result.error_code);
                return JsonIO::ErrorResponse("deps-backward", cone_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("cone", JsonIO::FunctionalConeToValueMap(cone_result.data));

            return JsonIO::SuccessResponse("deps-backward", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("deps-backward",
                                       "Failed to compute backward cone: " + std::string(e.what()),
                                       "DEPS_BACKWARD_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunDepsForward(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("deps-forward", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("deps-forward", "Session ID is required", "INVALID_ARGUMENT");
        }

        // Extract node parameters
        std::string node_id = opts.deps_node_id;
        std::string node_kind = opts.deps_node_kind;
        int max_depth = opts.deps_max_depth;

        if (node_id.empty()) {
            // Try to get node_id from payload if not in opts
            if (opts.payload.IsKey("node_id")) {
                node_id = opts.payload.Get("node_id").ToString().ToStd();
            }
        }

        if (node_id.empty()) {
            return JsonIO::ErrorResponse("deps-forward", "Node ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("deps-forward", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for analysis
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("deps-forward", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Resolve the functional node
            CircuitFacade circuit_facade;
            auto graph_result = circuit_facade.BuildGraphForBranch(metadata, session_dir, branch_name);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("deps-forward", graph_result.error_message, error_code_str);
            }

            auto resolve_result = ResolveFunctionalNode(graph_result.data, node_id, node_kind);
            if (!resolve_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(resolve_result.error_code);
                return JsonIO::ErrorResponse("deps-forward", resolve_result.error_message, error_code_str);
            }

            FunctionalNodeId func_node = resolve_result.data;

            // Perform forward cone computation
            auto cone_result = circuit_facade.BuildForwardConeForBranch(
                metadata, session_dir, branch_name, func_node, max_depth);

            if (!cone_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(cone_result.error_code);
                return JsonIO::ErrorResponse("deps-forward", cone_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("cone", JsonIO::FunctionalConeToValueMap(cone_result.data));

            return JsonIO::SuccessResponse("deps-forward", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("deps-forward",
                                       "Failed to compute forward cone: " + std::string(e.what()),
                                       "DEPS_FORWARD_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunDepsBoth(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("deps-both", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("deps-both", "Session ID is required", "INVALID_ARGUMENT");
        }

        // Extract node parameters
        std::string node_id = opts.deps_node_id;
        std::string node_kind = opts.deps_node_kind;
        int max_depth = opts.deps_max_depth;

        if (node_id.empty()) {
            // Try to get node_id from payload if not in opts
            if (opts.payload.IsKey("node_id")) {
                node_id = opts.payload.Get("node_id").ToString().ToStd();
            }
        }

        if (node_id.empty()) {
            return JsonIO::ErrorResponse("deps-both", "Node ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Load the session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("deps-both", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use for analysis
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Find the branch metadata
            std::optional<BranchMetadata> branch_opt = FindBranchByName(metadata, branch_name);
            if (!branch_opt.has_value()) {
                return JsonIO::ErrorResponse("deps-both", "Branch not found: " + branch_name, "INVALID_ARGUMENT");
            }

            // Resolve the functional node
            CircuitFacade circuit_facade;
            auto graph_result = circuit_facade.BuildGraphForBranch(metadata, session_dir, branch_name);
            if (!graph_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(graph_result.error_code);
                return JsonIO::ErrorResponse("deps-both", graph_result.error_message, error_code_str);
            }

            auto resolve_result = ResolveFunctionalNode(graph_result.data, node_id, node_kind);
            if (!resolve_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(resolve_result.error_code);
                return JsonIO::ErrorResponse("deps-both", resolve_result.error_message, error_code_str);
            }

            FunctionalNodeId func_node = resolve_result.data;

            // Perform both backward and forward cone computations
            auto backward_result = circuit_facade.BuildBackwardConeForBranch(
                metadata, session_dir, branch_name, func_node, max_depth);

            if (!backward_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(backward_result.error_code);
                return JsonIO::ErrorResponse("deps-both", backward_result.error_message, error_code_str);
            }

            auto forward_result = circuit_facade.BuildForwardConeForBranch(
                metadata, session_dir, branch_name, func_node, max_depth);

            if (!forward_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(forward_result.error_code);
                return JsonIO::ErrorResponse("deps-both", forward_result.error_message, error_code_str);
            }

            // Also compute the dependency summary
            auto summary_result = circuit_facade.BuildDependencySummaryForBranch(
                metadata, session_dir, branch_name, func_node, max_depth);

            if (!summary_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(summary_result.error_code);
                return JsonIO::ErrorResponse("deps-both", summary_result.error_message, error_code_str);
            }

            // Format the response
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));
            response_data.Add("root", JsonIO::FunctionalNodeIdToValueMap(func_node));
            response_data.Add("backward", JsonIO::FunctionalConeToValueMap(backward_result.data));
            response_data.Add("forward", JsonIO::FunctionalConeToValueMap(forward_result.data));
            response_data.Add("summary", JsonIO::DependencySummaryToValueMap(summary_result.data));

            return JsonIO::SuccessResponse("deps-both", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("deps-both",
                                       "Failed to compute both cones: " + std::string(e.what()),
                                       "DEPS_BOTH_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunBlocksList(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("blocks-list", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("blocks-list", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Create CircuitFacade to access block analysis functionality
            CircuitFacade facade;

            // Load session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("blocks-list", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Build block graph for the specified branch
            auto block_result = facade.BuildBlockGraphForBranch(metadata, session_dir, branch_name);
            if (!block_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(block_result.error_code);
                return JsonIO::ErrorResponse("blocks-list", block_result.error_message, error_code_str);
            }

            // Convert result to JSON
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            // Convert blocks to ValueArray
            Upp::ValueArray blocks_array;
            for (const auto& block : block_result.data.blocks) {
                Upp::ValueMap block_map;
                block_map.Add("id", Upp::String(block.id.c_str()));

                // Convert BlockKind to string
                std::string kind_str = "GenericComb";
                switch (block.kind) {
                    case BlockKind::GenericComb: kind_str = "GenericComb"; break;
                    case BlockKind::Adder: kind_str = "Adder"; break;
                    case BlockKind::Comparator: kind_str = "Comparator"; break;
                    case BlockKind::Mux: kind_str = "Mux"; break;
                    case BlockKind::Decoder: kind_str = "Decoder"; break;
                    case BlockKind::Encoder: kind_str = "Encoder"; break;
                    case BlockKind::Register: kind_str = "Register"; break;
                    case BlockKind::Counter: kind_str = "Counter"; break;
                    case BlockKind::Latch: kind_str = "Latch"; break;
                }
                block_map.Add("kind", Upp::String(kind_str.c_str()));

                // Add components
                Upp::ValueArray components_array;
                for (const auto& comp_id : block.components) {
                    components_array.Add(Upp::String(comp_id.c_str()));
                }
                block_map.Add("components", components_array);

                // Add ports
                Upp::ValueArray ports_array;
                for (const auto& port : block.ports) {
                    Upp::ValueMap port_map;
                    port_map.Add("name", Upp::String(port.name.c_str()));
                    port_map.Add("direction", Upp::String(port.direction.c_str()));

                    Upp::ValueArray pins_array;
                    for (const auto& pin_id : port.pins) {
                        pins_array.Add(Upp::String(pin_id.c_str()));
                    }
                    port_map.Add("pins", pins_array);

                    ports_array.Add(port_map);
                }
                block_map.Add("ports", ports_array);

                blocks_array.Add(block_map);
            }
            response_data.Add("blocks", blocks_array);

            return JsonIO::SuccessResponse("blocks-list", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("blocks-list",
                                       "Failed to list blocks: " + std::string(e.what()),
                                       "BLOCKS_LIST_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunBlocksExport(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("blocks-export", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("blocks-export", "Session ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Create CircuitFacade to access block analysis functionality
            CircuitFacade facade;

            // Load session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("blocks-export", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Build block graph for the specified branch
            auto block_result = facade.BuildBlockGraphForBranch(metadata, session_dir, branch_name);
            if (!block_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(block_result.error_code);
                return JsonIO::ErrorResponse("blocks-export", block_result.error_message, error_code_str);
            }

            // Convert result to JSON
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            // Create block graph representation
            Upp::ValueMap block_graph_map;

            // Add blocks
            Upp::ValueArray blocks_array;
            for (const auto& block : block_result.data.blocks) {
                Upp::ValueMap block_map;
                block_map.Add("id", Upp::String(block.id.c_str()));

                // Convert BlockKind to string
                std::string kind_str = "GenericComb";
                switch (block.kind) {
                    case BlockKind::GenericComb: kind_str = "GenericComb"; break;
                    case BlockKind::Adder: kind_str = "Adder"; break;
                    case BlockKind::Comparator: kind_str = "Comparator"; break;
                    case BlockKind::Mux: kind_str = "Mux"; break;
                    case BlockKind::Decoder: kind_str = "Decoder"; break;
                    case BlockKind::Encoder: kind_str = "Encoder"; break;
                    case BlockKind::Register: kind_str = "Register"; break;
                    case BlockKind::Counter: kind_str = "Counter"; break;
                    case BlockKind::Latch: kind_str = "Latch"; break;
                }
                block_map.Add("kind", Upp::String(kind_str.c_str()));

                // Add components
                Upp::ValueArray components_array;
                for (const auto& comp_id : block.components) {
                    components_array.Add(Upp::String(comp_id.c_str()));
                }
                block_map.Add("components", components_array);

                // Add ports
                Upp::ValueArray ports_array;
                for (const auto& port : block.ports) {
                    Upp::ValueMap port_map;
                    port_map.Add("name", Upp::String(port.name.c_str()));
                    port_map.Add("direction", Upp::String(port.direction.c_str()));

                    Upp::ValueArray pins_array;
                    for (const auto& pin_id : port.pins) {
                        pins_array.Add(Upp::String(pin_id.c_str()));
                    }
                    port_map.Add("pins", pins_array);

                    ports_array.Add(port_map);
                }
                block_map.Add("ports", ports_array);

                blocks_array.Add(block_map);
            }
            block_graph_map.Add("blocks", blocks_array);

            // For now, add empty edges array (we can enhance this later)
            Upp::ValueArray edges_array;
            block_graph_map.Add("edges", edges_array);

            response_data.Add("block_graph", block_graph_map);

            return JsonIO::SuccessResponse("blocks-export", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("blocks-export",
                                       "Failed to export block graph: " + std::string(e.what()),
                                       "BLOCKS_EXPORT_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunBlockInspect(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("block-inspect", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("block-inspect", "Session ID is required", "INVALID_ARGUMENT");
        }

        if (!opts.block_id.has_value()) {
            return JsonIO::ErrorResponse("block-inspect", "Block ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Create CircuitFacade to access block analysis functionality
            CircuitFacade facade;

            // Load session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("block-inspect", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Build block graph for the specified branch
            auto block_result = facade.BuildBlockGraphForBranch(metadata, session_dir, branch_name);
            if (!block_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(block_result.error_code);
                return JsonIO::ErrorResponse("block-inspect", block_result.error_message, error_code_str);
            }

            // Find the requested block
            BlockInstance* found_block = nullptr;
            for (auto& block : block_result.data.blocks) {
                if (block.id == opts.block_id.value()) {
                    found_block = &block;
                    break;
                }
            }

            if (!found_block) {
                return JsonIO::ErrorResponse("block-inspect",
                                           "Block not found: " + opts.block_id.value(),
                                           "NOT_FOUND");
            }

            // Convert result to JSON
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            Upp::ValueMap block_map;
            block_map.Add("id", Upp::String(found_block->id.c_str()));

            // Convert BlockKind to string
            std::string kind_str = "GenericComb";
            switch (found_block->kind) {
                case BlockKind::GenericComb: kind_str = "GenericComb"; break;
                case BlockKind::Adder: kind_str = "Adder"; break;
                case BlockKind::Comparator: kind_str = "Comparator"; break;
                case BlockKind::Mux: kind_str = "Mux"; break;
                case BlockKind::Decoder: kind_str = "Decoder"; break;
                case BlockKind::Encoder: kind_str = "Encoder"; break;
                case BlockKind::Register: kind_str = "Register"; break;
                case BlockKind::Counter: kind_str = "Counter"; break;
                case BlockKind::Latch: kind_str = "Latch"; break;
            }
            block_map.Add("kind", Upp::String(kind_str.c_str()));

            // Add components
            Upp::ValueArray components_array;
            for (const auto& comp_id : found_block->components) {
                components_array.Add(Upp::String(comp_id.c_str()));
            }
            block_map.Add("components", components_array);

            // Add nets (currently empty, but can be populated if needed)
            Upp::ValueArray nets_array;
            for (const auto& net_id : found_block->nets) {
                nets_array.Add(Upp::String(net_id.c_str()));
            }
            block_map.Add("nets", nets_array);

            // Add ports
            Upp::ValueArray ports_array;
            for (const auto& port : found_block->ports) {
                Upp::ValueMap port_map;
                port_map.Add("name", Upp::String(port.name.c_str()));
                port_map.Add("direction", Upp::String(port.direction.c_str()));

                Upp::ValueArray pins_array;
                for (const auto& pin_id : port.pins) {
                    pins_array.Add(Upp::String(pin_id.c_str()));
                }
                port_map.Add("pins", pins_array);

                ports_array.Add(port_map);
            }
            block_map.Add("ports", ports_array);

            response_data.Add("block", block_map);

            return JsonIO::SuccessResponse("block-inspect", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("block-inspect",
                                       "Failed to inspect block: " + std::string(e.what()),
                                       "BLOCK_INSPECT_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunBehaviorBlock(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("behavior-block", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("behavior-block", "Session ID is required", "INVALID_ARGUMENT");
        }

        if (!opts.block_id.has_value()) {
            return JsonIO::ErrorResponse("behavior-block", "Block ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Create CircuitFacade to access behavioral analysis functionality
            CircuitFacade facade;

            // Load session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("behavior-block", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Infer behavior for the specified block
            auto behavior_result = facade.InferBehaviorForBlockInBranch(
                metadata, session_dir, branch_name, opts.block_id.value());

            if (!behavior_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(behavior_result.error_code);
                return JsonIO::ErrorResponse("behavior-block", behavior_result.error_message, error_code_str);
            }

            // Convert the behavior descriptor to JSON
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            // Convert behavior descriptor to ValueMap
            Upp::ValueMap behavior_map;
            behavior_map.Add("subject_id", Upp::String(behavior_result.data.subject_id.c_str()));
            behavior_map.Add("subject_kind", Upp::String(behavior_result.data.subject_kind.c_str()));

            // Convert BehaviorKind to string
            std::string behavior_kind_str = "Unknown";
            switch (behavior_result.data.behavior_kind) {
                case BehaviorKind::Unknown: behavior_kind_str = "Unknown"; break;
                case BehaviorKind::CombinationalLogic: behavior_kind_str = "CombinationalLogic"; break;
                case BehaviorKind::Adder: behavior_kind_str = "Adder"; break;
                case BehaviorKind::Subtractor: behavior_kind_str = "Subtractor"; break;
                case BehaviorKind::Comparator: behavior_kind_str = "Comparator"; break;
                case BehaviorKind::EqualityComparator: behavior_kind_str = "EqualityComparator"; break;
                case BehaviorKind::InequalityComparator: behavior_kind_str = "InequalityComparator"; break;
                case BehaviorKind::Mux: behavior_kind_str = "Mux"; break;
                case BehaviorKind::Decoder: behavior_kind_str = "Decoder"; break;
                case BehaviorKind::Encoder: behavior_kind_str = "Encoder"; break;
                case BehaviorKind::Register: behavior_kind_str = "Register"; break;
                case BehaviorKind::Counter: behavior_kind_str = "Counter"; break;
                case BehaviorKind::StateMachine: behavior_kind_str = "StateMachine"; break;
            }
            behavior_map.Add("behavior_kind", Upp::String(behavior_kind_str.c_str()));

            // Add bit_width
            behavior_map.Add("bit_width", behavior_result.data.bit_width);

            // Add ports
            Upp::ValueArray ports_array;
            for (const auto& port : behavior_result.data.ports) {
                Upp::ValueMap port_map;
                port_map.Add("port_name", Upp::String(port.port_name.c_str()));
                port_map.Add("role", Upp::String(port.role.c_str()));
                ports_array.Add(port_map);
            }
            behavior_map.Add("ports", ports_array);

            // Add description
            behavior_map.Add("description", Upp::String(behavior_result.data.description.c_str()));

            response_data.Add("behavior", behavior_map);

            return JsonIO::SuccessResponse("behavior-block", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("behavior-block",
                                       "Failed to infer behavior for block: " + std::string(e.what()),
                                       "BEHAVIOR_BLOCK_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunBehaviorNode(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("behavior-node", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("behavior-node", "Session ID is required", "INVALID_ARGUMENT");
        }

        if (opts.node_id.empty()) {
            // Try to get node_id from payload if not in opts
            if (opts.payload.IsKey("node_id")) {
                opts.node_id = opts.payload.Get("node_id").ToString().ToStd();
            }
        }

        if (opts.node_id.empty()) {
            return JsonIO::ErrorResponse("behavior-node", "Node ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Extract optional node kind
            std::string node_kind_hint = opts.node_kind;
            if (opts.payload.IsKey("node_kind")) {
                node_kind_hint = opts.payload.Get("node_kind").ToString().ToStd();
            }

            // Create CircuitFacade to access behavioral analysis functionality
            CircuitFacade facade;

            // Load session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("behavior-node", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Infer behavior for the specified node
            auto behavior_result = facade.InferBehaviorForNodeInBranch(
                metadata, session_dir, branch_name, opts.node_id, node_kind_hint);

            if (!behavior_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(behavior_result.error_code);
                return JsonIO::ErrorResponse("behavior-node", behavior_result.error_message, error_code_str);
            }

            // Convert the behavior descriptor to JSON
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            // Convert behavior descriptor to ValueMap
            Upp::ValueMap behavior_map;
            behavior_map.Add("subject_id", Upp::String(behavior_result.data.subject_id.c_str()));
            behavior_map.Add("subject_kind", Upp::String(behavior_result.data.subject_kind.c_str()));

            // Convert BehaviorKind to string
            std::string behavior_kind_str = "Unknown";
            switch (behavior_result.data.behavior_kind) {
                case BehaviorKind::Unknown: behavior_kind_str = "Unknown"; break;
                case BehaviorKind::CombinationalLogic: behavior_kind_str = "CombinationalLogic"; break;
                case BehaviorKind::Adder: behavior_kind_str = "Adder"; break;
                case BehaviorKind::Subtractor: behavior_kind_str = "Subtractor"; break;
                case BehaviorKind::Comparator: behavior_kind_str = "Comparator"; break;
                case BehaviorKind::EqualityComparator: behavior_kind_str = "EqualityComparator"; break;
                case BehaviorKind::InequalityComparator: behavior_kind_str = "InequalityComparator"; break;
                case BehaviorKind::Mux: behavior_kind_str = "Mux"; break;
                case BehaviorKind::Decoder: behavior_kind_str = "Decoder"; break;
                case BehaviorKind::Encoder: behavior_kind_str = "Encoder"; break;
                case BehaviorKind::Register: behavior_kind_str = "Register"; break;
                case BehaviorKind::Counter: behavior_kind_str = "Counter"; break;
                case BehaviorKind::StateMachine: behavior_kind_str = "StateMachine"; break;
            }
            behavior_map.Add("behavior_kind", Upp::String(behavior_kind_str.c_str()));

            // Add bit_width
            behavior_map.Add("bit_width", behavior_result.data.bit_width);

            // Add ports
            Upp::ValueArray ports_array;
            for (const auto& port : behavior_result.data.ports) {
                Upp::ValueMap port_map;
                port_map.Add("port_name", Upp::String(port.port_name.c_str()));
                port_map.Add("role", Upp::String(port.role.c_str()));
                ports_array.Add(port_map);
            }
            behavior_map.Add("ports", ports_array);

            // Add description
            behavior_map.Add("description", Upp::String(behavior_result.data.description.c_str()));

            response_data.Add("behavior", behavior_map);

            return JsonIO::SuccessResponse("behavior-node", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("behavior-node",
                                       "Failed to infer behavior for node: " + std::string(e.what()),
                                       "BEHAVIOR_NODE_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunIrBlock(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("ir-block", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("ir-block", "Session ID is required", "INVALID_ARGUMENT");
        }

        if (!opts.block_id.has_value()) {
            return JsonIO::ErrorResponse("ir-block", "Block ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Create CircuitFacade to access IR functionality
            CircuitFacade facade;

            // Load session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("ir-block", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Build IR for the specified block
            auto ir_result = facade.BuildIrForBlockInBranch(
                metadata, session_dir, branch_name, opts.block_id.value());

            if (!ir_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(ir_result.error_code);
                return JsonIO::ErrorResponse("ir-block", ir_result.error_message, error_code_str);
            }

            // Convert the IR module to JSON
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            // Use JsonIO to serialize the IR module
            response_data.Add("ir", JsonIO::IrModuleToValueMap(ir_result.data));

            return JsonIO::SuccessResponse("ir-block", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("ir-block",
                                       "Failed to build IR for block: " + std::string(e.what()),
                                       "IR_BLOCK_ERROR");
        }
    }

    Upp::String CommandDispatcher::RunIrNodeRegion(const CommandOptions& opts) {
        if (opts.workspace.empty()) {
            return JsonIO::ErrorResponse("ir-node-region", "Workspace path is required", "INVALID_ARGUMENT");
        }

        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("ir-node-region", "Session ID is required", "INVALID_ARGUMENT");
        }

        if (opts.node_id.empty()) {
            // Try to get node_id from payload if not in opts
            if (opts.payload.IsKey("node_id")) {
                opts.node_id = opts.payload.Get("node_id").ToString().ToStd();
            }
        }

        if (opts.node_id.empty()) {
            return JsonIO::ErrorResponse("ir-node-region", "Node ID is required", "INVALID_ARGUMENT");
        }

        try {
            // Extract optional parameters
            std::string node_kind_hint = opts.node_kind;
            if (opts.payload.IsKey("node_kind")) {
                node_kind_hint = opts.payload.Get("node_kind").ToString().ToStd();
            }

            int max_depth = 4; // default
            if (opts.payload.IsKey("max_depth")) {
                max_depth = opts.payload.Get("max_depth").ToInt();
            }

            // Create CircuitFacade to access IR functionality
            CircuitFacade facade;

            // Load session metadata
            auto load_result = session_store_->LoadSession(opts.session_id.value());
            if (!load_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
                return JsonIO::ErrorResponse("ir-node-region", load_result.error_message, error_code_str);
            }

            SessionMetadata metadata = load_result.data;
            std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

            // Determine which branch to use
            std::string branch_name = opts.branch.value_or(metadata.current_branch);

            // Build IR for the node region
            auto ir_result = facade.BuildIrForNodeRegionInBranch(
                metadata, session_dir, branch_name, opts.node_id, node_kind_hint, max_depth);

            if (!ir_result.ok) {
                std::string error_code_str = JsonIO::ErrorCodeToString(ir_result.error_code);
                return JsonIO::ErrorResponse("ir-node-region", ir_result.error_message, error_code_str);
            }

            // Convert the IR module to JSON
            Upp::ValueMap response_data;
            response_data.Add("session_id", opts.session_id.value());
            response_data.Add("branch", Upp::String(branch_name.c_str()));

            // Use JsonIO to serialize the IR module
            response_data.Add("ir", JsonIO::IrModuleToValueMap(ir_result.data));

            return JsonIO::SuccessResponse("ir-node-region", response_data);
        } catch (const std::exception& e) {
            return JsonIO::ErrorResponse("ir-node-region",
                                       "Failed to build IR for node region: " + std::string(e.what()),
                                       "IR_NODE_REGION_ERROR");
        }
    }

Upp::String CommandDispatcher::RunRefactorSuggest(const CommandOptions& opts) {
    try {
        if (opts.workspace.empty() || opts.session_id.empty()) {
            return JsonIO::ErrorResponse("refactor-suggest",
                                       "Required parameters: --workspace, --session-id",
                                       "PARAMETER_ERROR");
        }

        // Get session metadata
        auto session_result = session_store_->LoadSession(opts.session_id);
        if (!session_result.ok) {
            return JsonIO::ErrorResponse("refactor-suggest",
                                       session_result.error_message,
                                       JsonIO::ErrorCodeToString(session_result.error_code));
        }

        SessionMetadata session = session_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + opts.session_id;

        // Get branch name, defaulting to current branch
        std::string branch_name = opts.branch.empty() ? session.current_branch : opts.branch;

        // Determine max plans, defaulting to 10
        int max_plans = 10; // Default
        if (!opts.max_plans.empty()) {
            try {
                max_plans = std::stoi(opts.max_plans);
            } catch (const std::exception&) {
                return JsonIO::ErrorResponse("refactor-suggest",
                                           "Invalid max-plans value: " + opts.max_plans,
                                           "PARAMETER_ERROR");
            }
        }

        // Use CircuitFacade to propose transformations
        CircuitFacade circuit_facade;
        auto plans_result = circuit_facade.ProposeTransformationsForBranch(
            session, session_dir, branch_name, max_plans);

        if (!plans_result.ok) {
            return JsonIO::ErrorResponse("refactor-suggest",
                                       plans_result.error_message,
                                       JsonIO::ErrorCodeToString(plans_result.error_code));
        }

        // Build response data
        Upp::ValueMap response_data;
        response_data.Add("session_id", Upp::String(opts.session_id.c_str()));
        response_data.Add("branch", Upp::String(branch_name.c_str()));

        // Convert the transformation plans to JSON
        Upp::ValueArray plans_array;
        for (const auto& plan : plans_result.data) {
            Upp::ValueMap plan_map;
            plan_map.Add("id", Upp::String(plan.id.c_str()));
            plan_map.Add("kind", JsonIO::TransformationKindToJson(plan.kind));
            plan_map.Add("target", JsonIO::TransformationTargetToValueMap(plan.target));
            plan_map.Add("guarantees", JsonIO::PreservationLevelsToValueArray(plan.guarantees));
            plan_map.Add("steps", JsonIO::TransformationStepsToValueArray(plan.steps));
            plans_array.Add(plan_map);
        }
        response_data.Add("plans", plans_array);

        return JsonIO::SuccessResponse("refactor-suggest", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("refactor-suggest",
                                   "Failed to suggest transformations: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunRefactorSuggestBlock(const CommandOptions& opts) {
    try {
        if (opts.workspace.empty() || opts.session_id.empty() || opts.block_id.empty()) {
            return JsonIO::ErrorResponse("refactor-suggest-block",
                                       "Required parameters: --workspace, --session-id, --block-id",
                                       "PARAMETER_ERROR");
        }

        // Get session metadata
        auto session_result = session_store_->LoadSession(opts.session_id);
        if (!session_result.ok) {
            return JsonIO::ErrorResponse("refactor-suggest-block",
                                       session_result.error_message,
                                       JsonIO::ErrorCodeToString(session_result.error_code));
        }

        SessionMetadata session = session_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + opts.session_id;

        // Get branch name, defaulting to current branch
        std::string branch_name = opts.branch.empty() ? session.current_branch : opts.branch;

        // Determine max plans, defaulting to 10
        int max_plans = 10; // Default
        if (!opts.max_plans.empty()) {
            try {
                max_plans = std::stoi(opts.max_plans);
            } catch (const std::exception&) {
                return JsonIO::ErrorResponse("refactor-suggest-block",
                                           "Invalid max-plans value: " + opts.max_plans,
                                           "PARAMETER_ERROR");
            }
        }

        // Use CircuitFacade to propose transformations for a specific block
        CircuitFacade circuit_facade;
        auto plans_result = circuit_facade.ProposeTransformationsForBlockInBranch(
            session, session_dir, branch_name, opts.block_id, max_plans);

        if (!plans_result.ok) {
            return JsonIO::ErrorResponse("refactor-suggest-block",
                                       plans_result.error_message,
                                       JsonIO::ErrorCodeToString(plans_result.error_code));
        }

        // Build response data
        Upp::ValueMap response_data;
        response_data.Add("session_id", Upp::String(opts.session_id.c_str()));
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("block_id", Upp::String(opts.block_id.c_str()));

        // Convert the transformation plans to JSON
        Upp::ValueArray plans_array;
        for (const auto& plan : plans_result.data) {
            Upp::ValueMap plan_map;
            plan_map.Add("id", Upp::String(plan.id.c_str()));
            plan_map.Add("kind", JsonIO::TransformationKindToJson(plan.kind));
            plan_map.Add("target", JsonIO::TransformationTargetToValueMap(plan.target));
            plan_map.Add("guarantees", JsonIO::PreservationLevelsToValueArray(plan.guarantees));
            plan_map.Add("steps", JsonIO::TransformationStepsToValueArray(plan.steps));
            plans_array.Add(plan_map);
        }
        response_data.Add("plans", plans_array);

        return JsonIO::SuccessResponse("refactor-suggest-block", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("refactor-suggest-block",
                                   "Failed to suggest transformations for block: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunRefactorApply(const CommandOptions& opts) {
    try {
        if (opts.workspace.empty() || opts.session_id.empty() || opts.plan_id.empty()) {
            return JsonIO::ErrorResponse("refactor-apply",
                                       "Required parameters: --workspace, --session-id, --plan-id",
                                       "PARAMETER_ERROR");
        }

        // Get session metadata
        auto session_result = session_store_->LoadSession(opts.session_id);
        if (!session_result.ok) {
            return JsonIO::ErrorResponse("refactor-apply",
                                       session_result.error_message,
                                       JsonIO::ErrorCodeToString(session_result.error_code));
        }

        SessionMetadata session = session_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + opts.session_id;

        // Get branch name, defaulting to current branch
        std::string branch_name = opts.branch.empty() ? session.current_branch : opts.branch;

        // For now, we'll implement a simple approach where the plan is specified as JSON in the options
        // In a more advanced implementation, plans might be stored and referenced by ID
        // For now, this is a simplified implementation that will return an error
        return JsonIO::ErrorResponse("refactor-apply",
                                   "Plan application requires the full plan details, which is not provided in this implementation",
                                   "NOT_IMPLEMENTED_ERROR");
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("refactor-apply",
                                   "Failed to apply transformation: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunIrOptBlock(const CommandOptions& opts) {
    try {
        if (opts.workspace.empty() || opts.session_id.empty() || opts.block_id.empty()) {
            return JsonIO::ErrorResponse("ir-opt-block",
                                       "Required parameters: --workspace, --session-id, --block-id",
                                       "PARAMETER_ERROR");
        }

        // Get session metadata
        auto session_result = session_store_->LoadSession(opts.session_id);
        if (!session_result.ok) {
            return JsonIO::ErrorResponse("ir-opt-block",
                                       session_result.error_message,
                                       JsonIO::ErrorCodeToString(session_result.error_code));
        }

        SessionMetadata session = session_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + opts.session_id;

        // Get branch name, defaulting to current branch
        std::string branch_name = opts.branch.empty() ? session.current_branch : opts.branch;

        // Parse optimization passes from command line, default to all passes if none specified
        std::vector<IrOptPassKind> passes_to_run;
        if (!opts.passes.empty()) {
            // Parse the comma-separated pass names
            std::vector<std::string> pass_names;
            size_t start = 0;
            size_t end = opts.passes.find(',');
            while (end != std::string::npos) {
                pass_names.push_back(opts.passes.substr(start, end - start));
                start = end + 1;
                end = opts.passes.find(',', start);
            }
            pass_names.push_back(opts.passes.substr(start));

            for (const auto& pass_name : pass_names) {
                // Trim whitespace
                std::string trimmed = pass_name;
                trimmed.erase(0, trimmed.find_first_not_of(" \t"));
                trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

                if (trimmed == "SimplifyAlgebraic") {
                    passes_to_run.push_back(IrOptPassKind::SimplifyAlgebraic);
                } else if (trimmed == "FoldConstants") {
                    passes_to_run.push_back(IrOptPassKind::FoldConstants);
                } else if (trimmed == "SimplifyMux") {
                    passes_to_run.push_back(IrOptPassKind::SimplifyMux);
                } else if (trimmed == "EliminateTrivialLogic") {
                    passes_to_run.push_back(IrOptPassKind::EliminateTrivialLogic);
                }
            }
        } else {
            // Default to all passes
            passes_to_run.push_back(IrOptPassKind::SimplifyAlgebraic);
            passes_to_run.push_back(IrOptPassKind::FoldConstants);
            passes_to_run.push_back(IrOptPassKind::SimplifyMux);
            passes_to_run.push_back(IrOptPassKind::EliminateTrivialLogic);
        }

        // Create CircuitFacade instance
        CircuitFacade facade(session_store_);

        // Run IR optimization
        auto result = facade.OptimizeBlockIrInBranch(session, session_dir, branch_name, opts.block_id, passes_to_run);
        if (!result.ok) {
            return JsonIO::ErrorResponse("ir-opt-block",
                                       result.error_message,
                                       JsonIO::ErrorCodeToString(result.error_code));
        }

        // Convert the result to JSON
        auto converter = [](const IrOptimizationResult& opt_result) -> Upp::ValueMap {
            Upp::ValueMap data;
            data.Add("session_id", Upp::String(opts.session_id.c_str()));
            data.Add("branch", Upp::String(branch_name.c_str()));
            data.Add("block_id", Upp::String(opts.block_id.c_str()));
            data.Add("optimization", Upp::ValueMap()
                .Add("original", JsonIO::IrModuleToValueMap(opt_result.original))
                .Add("optimized", JsonIO::IrModuleToValueMap(opt_result.optimized))
                .Add("summaries", JsonIO::IrOptChangeSummariesToValueArray(opt_result.summaries))
            );
            return data;
        };

        return JsonIO::FromResult<IrOptimizationResult>("ir-opt-block", result, converter);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("ir-opt-block",
                                   "Failed to run IR optimization: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunIrOptRefactorBlock(const CommandOptions& opts) {
    try {
        if (opts.workspace.empty() || opts.session_id.empty() || opts.block_id.empty()) {
            return JsonIO::ErrorResponse("ir-opt-refactor-block",
                                       "Required parameters: --workspace, --session-id, --block-id",
                                       "PARAMETER_ERROR");
        }

        // Get session metadata
        auto session_result = session_store_->LoadSession(opts.session_id);
        if (!session_result.ok) {
            return JsonIO::ErrorResponse("ir-opt-refactor-block",
                                       session_result.error_message,
                                       JsonIO::ErrorCodeToString(session_result.error_code));
        }

        SessionMetadata session = session_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + opts.session_id;

        // Get branch name, defaulting to current branch
        std::string branch_name = opts.branch.empty() ? session.current_branch : opts.branch;

        // Parse optimization passes from command line, default to all passes if none specified
        std::vector<IrOptPassKind> passes_to_run;
        if (!opts.passes.empty()) {
            // Parse the comma-separated pass names
            std::vector<std::string> pass_names;
            size_t start = 0;
            size_t end = opts.passes.find(',');
            while (end != std::string::npos) {
                pass_names.push_back(opts.passes.substr(start, end - start));
                start = end + 1;
                end = opts.passes.find(',', start);
            }
            pass_names.push_back(opts.passes.substr(start));

            for (const auto& pass_name : pass_names) {
                // Trim whitespace
                std::string trimmed = pass_name;
                trimmed.erase(0, trimmed.find_first_not_of(" \t"));
                trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

                if (trimmed == "SimplifyAlgebraic") {
                    passes_to_run.push_back(IrOptPassKind::SimplifyAlgebraic);
                } else if (trimmed == "FoldConstants") {
                    passes_to_run.push_back(IrOptPassKind::FoldConstants);
                } else if (trimmed == "SimplifyMux") {
                    passes_to_run.push_back(IrOptPassKind::SimplifyMux);
                } else if (trimmed == "EliminateTrivialLogic") {
                    passes_to_run.push_back(IrOptPassKind::EliminateTrivialLogic);
                }
            }
        } else {
            // Default to all passes
            passes_to_run.push_back(IrOptPassKind::SimplifyAlgebraic);
            passes_to_run.push_back(IrOptPassKind::FoldConstants);
            passes_to_run.push_back(IrOptPassKind::SimplifyMux);
            passes_to_run.push_back(IrOptPassKind::EliminateTrivialLogic);
        }

        // Create CircuitFacade instance
        CircuitFacade facade(session_store_);

        // Run IR-based transformation proposal
        auto result = facade.ProposeIrBasedTransformationsForBlock(session, session_dir, branch_name, opts.block_id, passes_to_run);
        if (!result.ok) {
            return JsonIO::ErrorResponse("ir-opt-refactor-block",
                                       result.error_message,
                                       JsonIO::ErrorCodeToString(result.error_code));
        }

        // Convert the result to JSON
        auto converter = [](const std::vector<TransformationPlan>& plans) -> Upp::ValueMap {
            Upp::ValueMap data;
            data.Add("session_id", Upp::String(opts.session_id.c_str()));
            data.Add("branch", Upp::String(branch_name.c_str()));
            data.Add("block_id", Upp::String(opts.block_id.c_str()));
            data.Add("plans", JsonIO::TransformationPlansToValueArray(plans));
            return data;
        };

        return JsonIO::FromResult<std::vector<TransformationPlan>>("ir-opt-refactor-block", result, converter);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("ir-opt-refactor-block",
                                   "Failed to run IR optimization refactor: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunBehaviorDiffBlock(const CommandOptions& opts) {
    try {
        if (opts.workspace.empty() || opts.session_id.empty() || opts.block_id.empty() ||
            opts.branch_before.empty() || opts.branch_after.empty()) {
            return JsonIO::ErrorResponse("behavior-diff-block",
                                       "Required parameters: --workspace, --session-id, --block-id, --branch-before, --branch-after",
                                       "PARAMETER_ERROR");
        }

        // Get session metadata
        auto session_result = session_store_->LoadSession(opts.session_id);
        if (!session_result.ok) {
            return JsonIO::ErrorResponse("behavior-diff-block",
                                       session_result.error_message,
                                       JsonIO::ErrorCodeToString(session_result.error_code));
        }

        SessionMetadata session = session_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + opts.session_id;

        // Use CircuitFacade to compute the behavior diff between branches
        CircuitFacade circuit_facade;
        auto diff_result = circuit_facade.DiffBlockBehaviorBetweenBranches(
            session, session_dir, opts.branch_before, opts.branch_after, opts.block_id);

        if (!diff_result.ok) {
            return JsonIO::ErrorResponse("behavior-diff-block",
                                       diff_result.error_message,
                                       JsonIO::ErrorCodeToString(diff_result.error_code));
        }

        // Build response data
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id);
        response_data.Add("branch_before", Upp::String(opts.branch_before.c_str()));
        response_data.Add("branch_after", Upp::String(opts.branch_after.c_str()));
        response_data.Add("behavior_diff", JsonIO::BehaviorDiffToValueMap(diff_result.data));

        return JsonIO::SuccessResponse("behavior-diff-block", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("behavior-diff-block",
                                   "Failed to compute behavior diff: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunIrDiffBlock(const CommandOptions& opts) {
    try {
        if (opts.workspace.empty() || opts.session_id.empty() || opts.block_id.empty() ||
            opts.branch_before.empty() || opts.branch_after.empty()) {
            return JsonIO::ErrorResponse("ir-diff-block",
                                       "Required parameters: --workspace, --session-id, --block-id, --branch-before, --branch-after",
                                       "PARAMETER_ERROR");
        }

        // Get session metadata
        auto session_result = session_store_->LoadSession(opts.session_id);
        if (!session_result.ok) {
            return JsonIO::ErrorResponse("ir-diff-block",
                                       session_result.error_message,
                                       JsonIO::ErrorCodeToString(session_result.error_code));
        }

        SessionMetadata session = session_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + opts.session_id;

        // Use CircuitFacade to compute the IR diff between branches
        CircuitFacade circuit_facade;
        auto diff_result = circuit_facade.DiffBlockIrBetweenBranches(
            session, session_dir, opts.branch_before, opts.branch_after, opts.block_id);

        if (!diff_result.ok) {
            return JsonIO::ErrorResponse("ir-diff-block",
                                       diff_result.error_message,
                                       JsonIO::ErrorCodeToString(diff_result.error_code));
        }

        // Build response data
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id);
        response_data.Add("branch_before", Upp::String(opts.branch_before.c_str()));
        response_data.Add("branch_after", Upp::String(opts.branch_after.c_str()));
        response_data.Add("ir_diff", JsonIO::IrDiffToValueMap(diff_result.data));

        return JsonIO::SuccessResponse("ir-diff-block", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("ir-diff-block",
                                   "Failed to compute IR diff: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunIrDiffNodeRegion(const CommandOptions& opts) {
    try {
        if (opts.workspace.empty() || opts.session_id.empty() || opts.node_id.empty() ||
            opts.branch_before.empty() || opts.branch_after.empty()) {
            return JsonIO::ErrorResponse("ir-diff-node-region",
                                       "Required parameters: --workspace, --session-id, --node-id, --branch-before, --branch-after",
                                       "PARAMETER_ERROR");
        }

        // Get session metadata
        auto session_result = session_store_->LoadSession(opts.session_id);
        if (!session_result.ok) {
            return JsonIO::ErrorResponse("ir-diff-node-region",
                                       session_result.error_message,
                                       JsonIO::ErrorCodeToString(session_result.error_code));
        }

        SessionMetadata session = session_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + opts.session_id;

        // Determine max depth, defaulting to 4
        int max_depth = 4; // Default
        if (!opts.max_depth.empty()) {
            try {
                max_depth = std::stoi(opts.max_depth);
            } catch (const std::exception&) {
                return JsonIO::ErrorResponse("ir-diff-node-region",
                                           "Invalid max-depth value: " + opts.max_depth,
                                           "PARAMETER_ERROR");
            }
        }

        // Use CircuitFacade to compute the node region IR diff between branches
        CircuitFacade circuit_facade;
        auto diff_result = circuit_facade.DiffNodeRegionIrBetweenBranches(
            session, session_dir, opts.branch_before, opts.branch_after,
            opts.node_id, opts.node_kind_hint, max_depth);

        if (!diff_result.ok) {
            return JsonIO::ErrorResponse("ir-diff-node-region",
                                       diff_result.error_message,
                                       JsonIO::ErrorCodeToString(diff_result.error_code));
        }

        // Build response data
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id);
        response_data.Add("branch_before", Upp::String(opts.branch_before.c_str()));
        response_data.Add("branch_after", Upp::String(opts.branch_after.c_str()));
        response_data.Add("ir_diff", JsonIO::IrDiffToValueMap(diff_result.data));

        return JsonIO::SuccessResponse("ir-diff-node-region", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("ir-diff-node-region",
                                   "Failed to compute node region IR diff: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerCreateSession(const CommandOptions& opts) {
    try {
        // Extract required parameters
        if (!opts.session_id.has_value()) {
            return JsonIO::ErrorResponse("designer-create-session", "proto_session_id is required", "INVALID_PARAMETER");
        }

        std::string branch = opts.branch.value_or("main");

        // This is a simulation of the command - in a real implementation, this would
        // connect to the daemon to execute the designer-create-session command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String("cd-dummy-session-123"));
        designer_session_map.Add("proto_session_id", opts.session_id.value());
        designer_session_map.Add("branch", Upp::String(branch.c_str()));
        designer_session_map.Add("current_block_id", Upp::String(""));
        designer_session_map.Add("current_node_id", Upp::String(""));
        designer_session_map.Add("current_node_kind", Upp::String(""));
        designer_session_map.Add("use_optimized_ir", false);

        response_data.Add("designer_session", designer_session_map);

        return JsonIO::SuccessResponse("designer-create-session", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-create-session",
                                   "Failed to create co-designer session: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerSetFocus(const CommandOptions& opts) {
    try {
        // Extract required parameters
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-set-focus", "designer_session_id is required", "INVALID_PARAMETER");
        }

        std::string block_id = opts.payload.Get("block_id", Upp::String("")).ToStd();
        std::string node_id = opts.payload.Get("node_id", Upp::String("")).ToStd();
        std::string node_kind = opts.payload.Get("node_kind", Upp::String("")).ToStd();
        bool use_optimized_ir = opts.payload.Get("use_optimized_ir", false);

        // This is a simulation of the command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String(block_id.c_str()));
        designer_session_map.Add("current_node_id", Upp::String(node_id.c_str()));
        designer_session_map.Add("current_node_kind", Upp::String(node_kind.c_str()));
        designer_session_map.Add("use_optimized_ir", use_optimized_ir);

        response_data.Add("designer_session", designer_session_map);

        return JsonIO::SuccessResponse("designer-set-focus", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-set-focus",
                                   "Failed to set co-designer focus: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerGetContext(const CommandOptions& opts) {
    try {
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-get-context", "designer_session_id is required", "INVALID_PARAMETER");
        }

        // This is a simulation of the command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String("B1"));
        designer_session_map.Add("current_node_id", Upp::String("C10:OUT"));
        designer_session_map.Add("current_node_kind", Upp::String("Pin"));
        designer_session_map.Add("use_optimized_ir", false);

        response_data.Add("designer_session", designer_session_map);

        return JsonIO::SuccessResponse("designer-get-context", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-get-context",
                                   "Failed to get co-designer context: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerAnalyze(const CommandOptions& opts) {
    try {
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-analyze", "designer_session_id is required", "INVALID_PARAMETER");
        }

        bool include_behavior = opts.payload.Get("include_behavior", true);
        bool include_ir = opts.payload.Get("include_ir", true);
        bool include_graph_stats = opts.payload.Get("include_graph_stats", false);
        bool include_timing = opts.payload.Get("include_timing", false);

        // This is a simulation of the command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String("B1"));
        designer_session_map.Add("current_node_id", Upp::String(""));
        designer_session_map.Add("current_node_kind", Upp::String(""));
        designer_session_map.Add("use_optimized_ir", false);

        response_data.Add("designer_session", designer_session_map);

        // Add block analysis if requested
        if (include_behavior || include_ir) {
            Upp::ValueMap block_map;
            block_map.Add("block_id", Upp::String("B1"));

            if (include_behavior) {
                BehaviorDescriptor behavior;
                behavior.subject_id = "B1";
                behavior.subject_kind = "Block";
                behavior.behavior_kind = BehaviorKind::Adder;
                behavior.description = "4-bit ripple carry adder";
                block_map.Add("behavior", JsonIO::BehaviorDescriptorToValueMap(behavior));
            }

            if (include_ir) {
                IrModule ir;
                ir.id = "B1";
                ir.inputs.push_back({"A", 4, false, 0});
                ir.inputs.push_back({"B", 4, false, 0});
                ir.inputs.push_back({"CIN", 1, false, 0});
                ir.outputs.push_back({"SUM", 4, false, 0});
                ir.outputs.push_back({"COUT", 1, false, 0});
                block_map.Add("ir", JsonIO::IrModuleToValueMap(ir));
            }

            response_data.Add("block", block_map);
        }

        return JsonIO::SuccessResponse("designer-analyze", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-analyze",
                                   "Failed to analyze: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerOptimize(const CommandOptions& opts) {
    try {
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-optimize", "designer_session_id is required", "INVALID_PARAMETER");
        }

        std::string target = opts.payload.Get("target", Upp::String("block")).ToStd();
        Upp::ValueArray passes_array = opts.payload.Get("passes", Upp::ValueArray());

        // This is a simulation of the command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String("B1"));
        designer_session_map.Add("current_node_id", Upp::String(""));
        designer_session_map.Add("current_node_kind", Upp::String(""));
        designer_session_map.Add("use_optimized_ir", false);

        response_data.Add("designer_session", designer_session_map);

        // Add optimization result
        Upp::ValueMap optimization_map;

        IrModule original;
        original.id = target == "block" ? "B1" : "N10";
        original.inputs.push_back({"A", 4, false, 0});
        original.outputs.push_back({"Y", 4, false, 0});

        IrModule optimized = original;  // In practice, this would be the optimized version

        optimization_map.Add("original", JsonIO::IrModuleToValueMap(original));
        optimization_map.Add("optimized", JsonIO::IrModuleToValueMap(optimized));

        // Add summaries
        Upp::ValueArray summaries_array;
        for (int i = 0; i < passes_array.GetCount(); ++i) {
            IrOptChangeSummary summary;
            summary.pass_kind = IrOptPassKind::SimplifyAlgebraic;
            summary.expr_changes = 2;
            summary.reg_changes = 0;
            summary.behavior_preserved = true;
            summaries_array.Add(JsonIO::IrOptChangeSummaryToValueMap(summary));
        }

        optimization_map.Add("summaries", summaries_array);
        response_data.Add("optimization", optimization_map);

        return JsonIO::SuccessResponse("designer-optimize", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-optimize",
                                   "Failed to optimize: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerProposeRefactors(const CommandOptions& opts) {
    try {
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-propose-refactors", "designer_session_id is required", "INVALID_PARAMETER");
        }

        std::string target = opts.payload.Get("target", Upp::String("block")).ToStd();
        Upp::ValueArray passes_array = opts.payload.Get("passes", Upp::ValueArray());

        // This is a simulation of the command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String("B1"));
        designer_session_map.Add("current_node_id", Upp::String(""));
        designer_session_map.Add("current_node_kind", Upp::String(""));
        designer_session_map.Add("use_optimized_ir", false);

        response_data.Add("designer_session", designer_session_map);

        // Create dummy transformation plans
        Upp::ValueArray plans_array;
        for (int i = 0; i < passes_array.GetCount(); ++i) {
            TransformationPlan plan;
            plan.id = "TRANS_" + std::to_string(i+1);
            plan.kind = TransformationKind::SimplifyRedundantGate;
            plan.target.subject_id = target == "block" ? "B1" : "C10";
            plan.target.subject_kind = target == "block" ? "Block" : "Component";

            plan.guarantees.push_back(PreservationLevel::BehaviorKindPreserved);
            plan.guarantees.push_back(PreservationLevel::IOContractPreserved);

            TransformationStep step;
            step.description = "Simplify redundant logic in " + plan.target.subject_id;
            plan.steps.push_back(step);

            plans_array.Add(JsonIO::TransformationPlanToValueMap(plan));
        }

        response_data.Add("plans", plans_array);

        return JsonIO::SuccessResponse("designer-propose-refactors", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-propose-refactors",
                                   "Failed to propose refactors: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerApplyRefactors(const CommandOptions& opts) {
    try {
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-apply-refactors", "designer_session_id is required", "INVALID_PARAMETER");
        }

        Upp::ValueArray plans_array = opts.payload.Get("plans", Upp::ValueArray());
        std::string user_id = opts.user_id;
        bool allow_unverified = opts.payload.Get("allow_unverified", false);

        // This is a simulation of the command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String("B1"));
        designer_session_map.Add("current_node_id", Upp::String(""));
        designer_session_map.Add("current_node_kind", Upp::String(""));
        designer_session_map.Add("use_optimized_ir", false);

        response_data.Add("designer_session", designer_session_map);

        // Add applied plan IDs
        Upp::ValueArray applied_ids_array;
        for (int i = 0; i < plans_array.GetCount(); ++i) {
            applied_ids_array.Add(Upp::String(("TRANS_" + std::to_string(i+1)).c_str()));
        }
        response_data.Add("applied_plan_ids", applied_ids_array);
        response_data.Add("new_circuit_revision", 43);

        return JsonIO::SuccessResponse("designer-apply-refactors", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-apply-refactors",
                                   "Failed to apply refactors: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerDiff(const CommandOptions& opts) {
    try {
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-diff", "designer_session_id is required", "INVALID_PARAMETER");
        }

        std::string compare_branch = opts.payload.Get("compare_branch", Upp::String("main")).ToStd();
        bool include_behavior_diff = opts.payload.Get("include_behavior_diff", true);
        bool include_ir_diff = opts.payload.Get("include_ir_diff", true);

        // This is a simulation of the command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String("B1"));
        designer_session_map.Add("current_node_id", Upp::String(""));
        designer_session_map.Add("current_node_kind", Upp::String(""));
        designer_session_map.Add("use_optimized_ir", false);

        response_data.Add("designer_session", designer_session_map);

        if (include_behavior_diff) {
            BehaviorDiff behavior_diff;
            behavior_diff.subject_id = "B1";
            behavior_diff.subject_kind = "Block";
            behavior_diff.change_kind = BehaviorChangeKind::None;

            behavior_diff.before_behavior.subject_id = "B1";
            behavior_diff.before_behavior.subject_kind = "Block";
            behavior_diff.before_behavior.behavior_kind = BehaviorKind::Adder;
            behavior_diff.before_behavior.description = "4-bit ripple carry adder";

            behavior_diff.after_behavior = behavior_diff.before_behavior;

            response_data.Add("behavior_diff", JsonIO::BehaviorDiffToValueMap(behavior_diff));
        }

        if (include_ir_diff) {
            IrDiff ir_diff;
            ir_diff.module_id = "B1";
            ir_diff.change_kind = IrChangeKind::None;

            response_data.Add("ir_diff", JsonIO::IrDiffToValueMap(ir_diff));
        }

        return JsonIO::SuccessResponse("designer-diff", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-diff",
                                   "Failed to compute diff: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerCodegen(const CommandOptions& opts) {
    try {
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-codegen", "designer_session_id is required", "INVALID_PARAMETER");
        }

        std::string target = opts.payload.Get("target", Upp::String("block")).ToStd();
        std::string flavor = opts.payload.Get("flavor", Upp::String("PseudoVerilog")).ToStd();
        bool use_optimized_ir = opts.payload.Get("use_optimized_ir", true);

        // This is a simulation of the command
        Upp::ValueMap response_data;
        Upp::ValueMap designer_session_map;

        // Create a dummy response simulating what the daemon would return
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String("B1"));
        designer_session_map.Add("current_node_id", Upp::String(""));
        designer_session_map.Add("current_node_kind", Upp::String(""));
        designer_session_map.Add("use_optimized_ir", use_optimized_ir);

        response_data.Add("designer_session", designer_session_map);

        // Create codegen result
        Upp::ValueMap codegen_map;
        std::string id = target == "block" ? "B1" : "C10";
        codegen_map.Add("id", Upp::String(id.c_str()));
        codegen_map.Add("name", Upp::String((id + "_generated").c_str()));
        codegen_map.Add("flavor", Upp::String(flavor.c_str()));

        std::string code;
        if (flavor == "PseudoVerilog" || flavor == "Verilog") {
            code = "// Generated " + flavor + " code for " + id + "\n";
            code += "module " + id + "(\n";
            code += "    input [3:0] A,\n";
            code += "    input [3:0] B,\n";
            code += "    input CIN,\n";
            code += "    output [3:0] SUM,\n";
            code += "    output COUT\n";
            code += ");\n";
            code += "  // Implementation goes here\n";
            code += "endmodule\n";
        } else {
            code = "// Generated code for " + id + " in " + flavor + " format";
        }

        codegen_map.Add("code", Upp::String(code.c_str()));
        response_data.Add("codegen", codegen_map);

        return JsonIO::SuccessResponse("designer-codegen", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-codegen",
                                   "Failed to generate code: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunDesignerRunPlaybook(const CommandOptions& opts) {
    try {
        std::string designer_session_id = opts.payload.Get("designer_session_id", Upp::String("")).ToStd();
        if (designer_session_id.empty()) {
            return JsonIO::ErrorResponse("designer-run-playbook", "designer_session_id is required", "INVALID_PARAMETER");
        }

        // This is a simulation of the command - in a real implementation, this would
        // connect to the daemon to execute the designer-run-playbook command
        Upp::ValueMap response_data;

        // Create a dummy playbook result simulating what the daemon would return
        Upp::ValueMap playbook_result_map;

        // Add basic playbook info
        playbook_result_map.Add("kind", Upp::String("OptimizeBlockAndReport"));

        // Add config
        Upp::ValueMap config_map;
        config_map.Add("kind", Upp::String("OptimizeBlockAndReport"));
        config_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        config_map.Add("target", Upp::String("block"));
        config_map.Add("block_id", Upp::String("B1"));
        config_map.Add("baseline_branch", Upp::String("main"));
        Upp::ValueArray passes_array;
        passes_array.Add(Upp::String("SimplifyAlgebraic"));
        passes_array.Add(Upp::String("FoldConstants"));
        config_map.Add("passes", passes_array);
        config_map.Add("use_optimized_ir", true);
        config_map.Add("apply_refactors", false);
        playbook_result_map.Add("config", config_map);

        // Add session state
        Upp::ValueMap designer_session_map;
        designer_session_map.Add("designer_session_id", Upp::String(designer_session_id.c_str()));
        designer_session_map.Add("proto_session_id", opts.session_id.value_or(-1));
        designer_session_map.Add("branch", Upp::String(opts.branch.value_or("main").c_str()));
        designer_session_map.Add("current_block_id", Upp::String("B1"));
        designer_session_map.Add("current_node_id", Upp::String(""));
        designer_session_map.Add("current_node_kind", Upp::String(""));
        designer_session_map.Add("use_optimized_ir", true);
        playbook_result_map.Add("designer_session", designer_session_map);

        // Add dummy behavior descriptors
        Upp::ValueMap behavior_descriptor_map;
        behavior_descriptor_map.Add("subject_id", Upp::String("B1"));
        behavior_descriptor_map.Add("subject_kind", Upp::String("Block"));
        behavior_descriptor_map.Add("behavior_kind", Upp::String("Adder"));
        behavior_descriptor_map.Add("bit_width", 4);
        behavior_descriptor_map.Add("description", Upp::String("4-bit ripple-carry adder with carry in/out"));

        Upp::ValueArray ports_array;
        Upp::ValueMap port_map;
        port_map.Add("port_name", Upp::String("A"));
        port_map.Add("role", Upp::String("data_in"));
        ports_array.Add(port_map);
        port_map.Add("port_name", Upp::String("B"));
        port_map.Add("role", Upp::String("data_in"));
        ports_array.Add(port_map);
        port_map.Add("port_name", Upp::String("SUM"));
        port_map.Add("role", Upp::String("data_out"));
        ports_array.Add(port_map);
        behavior_descriptor_map.Add("ports", ports_array);
        playbook_result_map.Add("initial_behavior", behavior_descriptor_map);
        playbook_result_map.Add("final_behavior", behavior_descriptor_map);

        // Add dummy IR modules
        Upp::ValueMap ir_module_map;
        ir_module_map.Add("id", Upp::String("B1"));

        Upp::ValueArray inputs_array, outputs_array;
        Upp::ValueMap io_map;
        io_map.Add("name", Upp::String("A"));
        io_map.Add("bit_width", 4);
        io_map.Add("is_literal", false);
        inputs_array.Add(io_map);
        io_map.Add("name", Upp::String("B"));
        inputs_array.Add(io_map);
        io_map.Add("name", Upp::String("CIN"));
        io_map.Add("bit_width", 1);
        inputs_array.Add(io_map);
        ir_module_map.Add("inputs", inputs_array);

        io_map.Add("name", Upp::String("SUM"));
        outputs_array.Add(io_map);
        io_map.Add("name", Upp::String("COUT"));
        io_map.Add("bit_width", 1);
        outputs_array.Add(io_map);
        ir_module_map.Add("outputs", outputs_array);

        Upp::ValueArray comb_assigns_array;
        Upp::ValueMap expr_map;
        expr_map.Add("kind", Upp::String("Add"));
        Upp::ValueMap target_map;
        target_map.Add("name", Upp::String("SUM"));
        target_map.Add("bit_width", 4);
        target_map.Add("is_literal", false);
        expr_map.Add("target", target_map);
        Upp::ValueArray args_array;
        Upp::ValueMap arg_map;
        arg_map.Add("name", Upp::String("A"));
        arg_map.Add("bit_width", 4);
        arg_map.Add("is_literal", false);
        args_array.Add(arg_map);
        arg_map.Add("name", Upp::String("B"));
        args_array.Add(arg_map);
        expr_map.Add("args", args_array);
        comb_assigns_array.Add(expr_map);
        ir_module_map.Add("comb_assigns", comb_assigns_array);
        Upp::ValueArray reg_assigns_array;
        ir_module_map.Add("reg_assigns", reg_assigns_array);

        playbook_result_map.Add("initial_ir", ir_module_map);
        playbook_result_map.Add("final_ir", ir_module_map);

        // Add dummy optimization result
        Upp::ValueMap optimization_map;
        optimization_map.Add("original", ir_module_map);
        optimization_map.Add("optimized", ir_module_map);
        Upp::ValueArray summaries_array;
        Upp::ValueMap summary_map;
        summary_map.Add("pass_kind", Upp::String("SimplifyAlgebraic"));
        summary_map.Add("expr_changes", 0);
        summary_map.Add("reg_changes", 0);
        summary_map.Add("behavior_preserved", true);
        summaries_array.Add(summary_map);
        optimization_map.Add("summaries", summaries_array);
        playbook_result_map.Add("optimization", optimization_map);

        // Add dummy transformation plans
        Upp::ValueArray plans_array;
        Upp::ValueMap plan_map;
        plan_map.Add("id", Upp::String("IR_T1"));
        plan_map.Add("kind", Upp::String("SimplifyDoubleInversion"));
        Upp::ValueMap target_target_map;
        target_target_map.Add("subject_id", Upp::String("B1"));
        target_target_map.Add("subject_kind", Upp::String("Block"));
        plan_map.Add("target", target_target_map);
        Upp::ValueArray guarantees_array;
        guarantees_array.Add(Upp::String("BehaviorKindPreserved"));
        guarantees_array.Add(Upp::String("IOContractPreserved"));
        plan_map.Add("guarantees", guarantees_array);
        Upp::ValueArray steps_array;
        Upp::ValueMap step_map;
        step_map.Add("description", Upp::String("Remove redundant NOT-then-NOT around SUM path"));
        steps_array.Add(step_map);
        plan_map.Add("steps", steps_array);
        plans_array.Add(plan_map);
        playbook_result_map.Add("proposed_plans", plans_array);

        // Add empty applied plans array and revision
        Upp::ValueArray applied_ids_array;
        playbook_result_map.Add("applied_plan_ids", applied_ids_array);
        playbook_result_map.Add("new_circuit_revision", -1);

        // Add dummy diff results
        Upp::ValueMap behavior_diff_map;
        behavior_diff_map.Add("subject_id", Upp::String("B1"));
        behavior_diff_map.Add("subject_kind", Upp::String("Block"));
        behavior_diff_map.Add("change_kind", Upp::String("None"));
        behavior_diff_map.Add("before_behavior", behavior_descriptor_map);
        behavior_diff_map.Add("after_behavior", behavior_descriptor_map);
        Upp::ValueArray port_changes_array;
        behavior_diff_map.Add("port_changes", port_changes_array);
        playbook_result_map.Add("behavior_diff", behavior_diff_map);

        Upp::ValueMap ir_diff_map;
        ir_diff_map.Add("module_id", Upp::String("B1"));
        ir_diff_map.Add("change_kind", Upp::String("None"));
        Upp::ValueMap iface_changes_map;
        iface_changes_map.Add("added_inputs", JsonIO::IrValuesToValueArraySimple(std::vector<IrValue>()));
        iface_changes_map.Add("removed_inputs", JsonIO::IrValuesToValueArraySimple(std::vector<IrValue>()));
        iface_changes_map.Add("added_outputs", JsonIO::IrValuesToValueArraySimple(std::vector<IrValue>()));
        iface_changes_map.Add("removed_outputs", JsonIO::IrValuesToValueArraySimple(std::vector<IrValue>()));
        ir_diff_map.Add("iface_changes", iface_changes_map);
        Upp::ValueArray expr_changes_array, reg_changes_array;
        ir_diff_map.Add("comb_changes", expr_changes_array);
        ir_diff_map.Add("reg_changes", reg_changes_array);
        playbook_result_map.Add("ir_diff", ir_diff_map);

        // Add dummy codegen result
        Upp::ValueMap codegen_map;
        codegen_map.Add("id", Upp::String("B1"));
        codegen_map.Add("name", Upp::String("B1_PseudoVerilog"));
        codegen_map.Add("flavor", Upp::String("PseudoVerilog"));
        codegen_map.Add("code", Upp::String("// Generated code for block B1\nmodule B1(...);\n  // Implementation\nendmodule\n"));
        playbook_result_map.Add("codegen", codegen_map);

        response_data.Add("playbook_result", playbook_result_map);

        return JsonIO::SuccessResponse("designer-run-playbook", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("designer-run-playbook",
                                   "Failed to run playbook: " + std::string(e.what()),
                                   "INTERNAL_ERROR");
    }
}

Upp::String CommandDispatcher::RunScheduleBlock(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("schedule-block", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("schedule-block", "Session ID is required", "INVALID_ARGUMENT");
    }

    if (!opts.block_id.has_value()) {
        return JsonIO::ErrorResponse("schedule-block", "Block ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Create CircuitFacade to access scheduled IR functionality
        CircuitFacade facade;

        // Load session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("schedule-block", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use
        std::string branch_name = opts.branch.value_or(metadata.current_branch);

        // Parse scheduling configuration from options
        SchedulingConfig config;

        // Strategy
        std::string strategy_str = "SingleStage";
        if (opts.payload.IsKey("strategy")) {
            strategy_str = opts.payload.Get("strategy").ToString().ToStd();
        }

        if (strategy_str == "SingleStage") {
            config.strategy = SchedulingStrategy::SingleStage;
        } else if (strategy_str == "DepthBalancedStages") {
            config.strategy = SchedulingStrategy::DepthBalancedStages;
        } else if (strategy_str == "FixedStageCount") {
            config.strategy = SchedulingStrategy::FixedStageCount;
        } else {
            return JsonIO::ErrorResponse("schedule-block",
                                       "Invalid strategy: " + strategy_str +
                                       ". Must be SingleStage, DepthBalancedStages, or FixedStageCount",
                                       "INVALID_ARGUMENT");
        }

        // Requested stages
        int requested_stages = 1;
        if (opts.payload.IsKey("requested_stages")) {
            requested_stages = opts.payload.Get("requested_stages").ToInt();
        } else if (opts.payload.IsKey("stages")) {
            requested_stages = opts.payload.Get("stages").ToInt();
        }
        config.requested_stages = requested_stages;

        // Build scheduled IR for the specified block
        auto scheduled_ir_result = facade.BuildScheduledIrForBlockInBranch(
            metadata, session_dir, branch_name, opts.block_id.value(), config);

        if (!scheduled_ir_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(scheduled_ir_result.error_code);
            return JsonIO::ErrorResponse("schedule-block", scheduled_ir_result.error_message, error_code_str);
        }

        // Convert the scheduled IR module to JSON
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("block_id", Upp::String(opts.block_id.value().c_str()));

        // Use JsonIO to serialize the scheduled IR module
        response_data.Add("scheduled_ir", JsonIO::ScheduledModuleToValueMap(scheduled_ir_result.data));

        return JsonIO::SuccessResponse("schedule-block", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("schedule-block",
                                   "Failed to build scheduled IR for block: " + std::string(e.what()),
                                   "SCHEDULE_BLOCK_ERROR");
    }
}

Upp::String CommandDispatcher::RunScheduleNodeRegion(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("schedule-node-region", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("schedule-node-region", "Session ID is required", "INVALID_ARGUMENT");
    }

    if (opts.node_id.empty()) {
        // Try to get node_id from payload if not in opts
        if (opts.payload.IsKey("node_id")) {
            opts.node_id = opts.payload.Get("node_id").ToString().ToStd();
        }
    }

    if (opts.node_id.empty()) {
        return JsonIO::ErrorResponse("schedule-node-region", "Node ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Extract optional parameters
        std::string node_kind_hint = opts.node_kind;
        if (opts.payload.IsKey("node_kind")) {
            node_kind_hint = opts.payload.Get("node_kind").ToString().ToStd();
        }

        int max_depth = 4; // default
        if (opts.payload.IsKey("max_depth")) {
            max_depth = opts.payload.Get("max_depth").ToInt();
        }

        // Create CircuitFacade to access scheduled IR functionality
        CircuitFacade facade;

        // Load session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("schedule-node-region", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use
        std::string branch_name = opts.branch.value_or(metadata.current_branch);

        // Parse scheduling configuration from options
        SchedulingConfig config;

        // Strategy
        std::string strategy_str = "SingleStage";
        if (opts.payload.IsKey("strategy")) {
            strategy_str = opts.payload.Get("strategy").ToString().ToStd();
        }

        if (strategy_str == "SingleStage") {
            config.strategy = SchedulingStrategy::SingleStage;
        } else if (strategy_str == "DepthBalancedStages") {
            config.strategy = SchedulingStrategy::DepthBalancedStages;
        } else if (strategy_str == "FixedStageCount") {
            config.strategy = SchedulingStrategy::FixedStageCount;
        } else {
            return JsonIO::ErrorResponse("schedule-node-region",
                                       "Invalid strategy: " + strategy_str +
                                       ". Must be SingleStage, DepthBalancedStages, or FixedStageCount",
                                       "INVALID_ARGUMENT");
        }

        // Requested stages
        int requested_stages = 1;
        if (opts.payload.IsKey("requested_stages")) {
            requested_stages = opts.payload.Get("requested_stages").ToInt();
        } else if (opts.payload.IsKey("stages")) {
            requested_stages = opts.payload.Get("stages").ToInt();
        }
        config.requested_stages = requested_stages;

        // Build scheduled IR for the node region
        auto scheduled_ir_result = facade.BuildScheduledIrForNodeRegionInBranch(
            metadata, session_dir, branch_name, opts.node_id, node_kind_hint, max_depth, config);

        if (!scheduled_ir_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(scheduled_ir_result.error_code);
            return JsonIO::ErrorResponse("schedule-node-region", scheduled_ir_result.error_message, error_code_str);
        }

        // Convert the scheduled IR module to JSON
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("node_id", Upp::String(opts.node_id.c_str()));

        // Use JsonIO to serialize the scheduled IR module
        response_data.Add("scheduled_ir", JsonIO::ScheduledModuleToValueMap(scheduled_ir_result.data));

        return JsonIO::SuccessResponse("schedule-node-region", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("schedule-node-region",
                                   "Failed to build scheduled IR for node region: " + std::string(e.what()),
                                   "SCHEDULE_NODE_REGION_ERROR");
    }
}

Upp::String CommandDispatcher::RunPipelineBlock(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("pipeline-block", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("pipeline-block", "Session ID is required", "INVALID_ARGUMENT");
    }

    if (!opts.block_id.has_value()) {
        return JsonIO::ErrorResponse("pipeline-block", "Block ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Create CircuitFacade to access pipeline analysis functionality
        CircuitFacade facade;

        // Load session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("pipeline-block", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use
        std::string branch_name = opts.branch.value_or(metadata.current_branch);

        // Build pipeline map for the specified block
        auto pipeline_map_result = facade.BuildPipelineMapForBlockInBranch(
            metadata, session_dir, branch_name, opts.block_id.value());

        if (!pipeline_map_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(pipeline_map_result.error_code);
            return JsonIO::ErrorResponse("pipeline-block", pipeline_map_result.error_message, error_code_str);
        }

        // Convert the pipeline map to JSON
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("block_id", Upp::String(opts.block_id.value().c_str()));

        // Use JsonIO to serialize the pipeline map
        response_data.Add("pipeline_map", JsonIO::PipelineMapToValueMap(pipeline_map_result.data));

        return JsonIO::SuccessResponse("pipeline-block", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("pipeline-block",
                                   "Failed to build pipeline map for block: " + std::string(e.what()),
                                   "PIPELINE_BLOCK_ERROR");
    }
}

Upp::String CommandDispatcher::RunPipelineSubsystem(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("pipeline-subsystem", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("pipeline-subsystem", "Session ID is required", "INVALID_ARGUMENT");
    }

    if (!opts.subsystem_id.has_value()) {
        return JsonIO::ErrorResponse("pipeline-subsystem", "Subsystem ID is required", "INVALID_ARGUMENT");
    }

    if (!opts.block_ids.has_value()) {
        return JsonIO::ErrorResponse("pipeline-subsystem", "Block IDs list is required", "INVALID_ARGUMENT");
    }

    try {
        // Create CircuitFacade to access pipeline analysis functionality
        CircuitFacade facade;

        // Load session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("pipeline-subsystem", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use
        std::string branch_name = opts.branch.value_or(metadata.current_branch);

        // Convert block IDs string to vector
        std::vector<std::string> block_ids;
        std::string block_ids_str = opts.block_ids.value();
        if (!block_ids_str.empty()) {
            // Simple parsing of comma-separated values
            size_t start = 0;
            size_t end = block_ids_str.find(',');
            while (end != std::string::npos) {
                block_ids.push_back(block_ids_str.substr(start, end - start));
                start = end + 1;
                end = block_ids_str.find(',', start);
            }
            block_ids.push_back(block_ids_str.substr(start));
        }

        // Build pipeline map for the specified subsystem
        auto pipeline_map_result = facade.BuildPipelineMapForSubsystemInBranch(
            metadata, session_dir, branch_name, opts.subsystem_id.value(), block_ids);

        if (!pipeline_map_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(pipeline_map_result.error_code);
            return JsonIO::ErrorResponse("pipeline-subsystem", pipeline_map_result.error_message, error_code_str);
        }

        // Convert the pipeline map to JSON
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("subsystem_id", Upp::String(opts.subsystem_id.value().c_str()));
        response_data.Add("block_ids", JsonIO::VectorToStringValueArray(block_ids));

        // Use JsonIO to serialize the pipeline map
        response_data.Add("pipeline_map", JsonIO::PipelineMapToValueMap(pipeline_map_result.data));

        return JsonIO::SuccessResponse("pipeline-subsystem", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("pipeline-subsystem",
                                   "Failed to build pipeline map for subsystem: " + std::string(e.what()),
                                   "PIPELINE_SUBSYSTEM_ERROR");
    }
}

Upp::String CommandDispatcher::RunCdcBlock(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("cdc-block", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("cdc-block", "Session ID is required", "INVALID_ARGUMENT");
    }

    if (!opts.block_id.has_value()) {
        return JsonIO::ErrorResponse("cdc-block", "Block ID is required", "INVALID_ARGUMENT");
    }

    try {
        // Create CircuitFacade to access CDC analysis functionality
        CircuitFacade facade;

        // Load session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("cdc-block", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use
        std::string branch_name = opts.branch.value_or(metadata.current_branch);

        // Build CDC report for the specified block
        auto cdc_report_result = facade.BuildCdcReportForBlockInBranch(
            metadata, session_dir, branch_name, opts.block_id.value());

        if (!cdc_report_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(cdc_report_result.error_code);
            return JsonIO::ErrorResponse("cdc-block", cdc_report_result.error_message, error_code_str);
        }

        // Convert the CDC report to JSON
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("block_id", Upp::String(opts.block_id.value().c_str()));

        // Use JsonIO to serialize the CDC report (after implementing JsonIO serialization)
        response_data.Add("cdc_report", JsonIO::CdcReportToValueMap(cdc_report_result.data));

        return JsonIO::SuccessResponse("cdc-block", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("cdc-block",
                                   "Failed to build CDC report for block: " + std::string(e.what()),
                                   "CDC_BLOCK_ERROR");
    }
}

Upp::String CommandDispatcher::RunCdcSubsystem(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("cdc-subsystem", "Workspace path is required", "INVALID_ARGUMENT");
    }

    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("cdc-subsystem", "Session ID is required", "INVALID_ARGUMENT");
    }

    if (!opts.subsystem_id.has_value()) {
        return JsonIO::ErrorResponse("cdc-subsystem", "Subsystem ID is required", "INVALID_ARGUMENT");
    }

    if (!opts.block_ids.has_value()) {
        return JsonIO::ErrorResponse("cdc-subsystem", "Block IDs list is required", "INVALID_ARGUMENT");
    }

    try {
        // Create CircuitFacade to access CDC analysis functionality
        CircuitFacade facade;

        // Load session metadata
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("cdc-subsystem", load_result.error_message, error_code_str);
        }

        SessionMetadata metadata = load_result.data;
        std::string session_dir = opts.workspace + "/sessions/" + std::to_string(opts.session_id.value());

        // Determine which branch to use
        std::string branch_name = opts.branch.value_or(metadata.current_branch);

        // Convert block IDs string to vector
        std::vector<std::string> block_ids;
        std::string block_ids_str = opts.block_ids.value();
        if (!block_ids_str.empty()) {
            // Simple parsing of comma-separated values
            size_t start = 0;
            size_t end = block_ids_str.find(',');
            while (end != std::string::npos) {
                block_ids.push_back(block_ids_str.substr(start, end - start));
                start = end + 1;
                end = block_ids_str.find(',', start);
            }
            block_ids.push_back(block_ids_str.substr(start));
        }

        // Build CDC report for the specified subsystem
        auto cdc_report_result = facade.BuildCdcReportForSubsystemInBranch(
            metadata, session_dir, branch_name, opts.subsystem_id.value(), block_ids);

        if (!cdc_report_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(cdc_report_result.error_code);
            return JsonIO::ErrorResponse("cdc-subsystem", cdc_report_result.error_message, error_code_str);
        }

        // Convert the CDC report to JSON
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("branch", Upp::String(branch_name.c_str()));
        response_data.Add("subsystem_id", Upp::String(opts.subsystem_id.value().c_str()));
        response_data.Add("block_ids", JsonIO::VectorToStringValueArray(block_ids));

        // Use JsonIO to serialize the CDC report (after implementing JsonIO serialization)
        response_data.Add("cdc_report", JsonIO::CdcReportToValueMap(cdc_report_result.data));

        return JsonIO::SuccessResponse("cdc-subsystem", response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("cdc-subsystem",
                                   "Failed to build CDC report for subsystem: " + std::string(e.what()),
                                   "CDC_SUBSYSTEM_ERROR");
    }
}

} // namespace ProtoVMCLI