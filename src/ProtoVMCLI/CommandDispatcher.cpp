#include "CommandDispatcher.h"
#include "JsonIO.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>

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

        Upp::ValueMap response_data;
        response_data.Add("session_id", result.data);
        response_data.Add("workspace", Upp::String(opts.workspace.c_str()));
        response_data.Add("circuit_file", Upp::String(opts.circuit_file.value().c_str()));

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
        // In a real implementation, this would:
        // 1. Load the session's machine instance
        // 2. Execute ticks on the machine
        // 3. Update session metadata with new tick count
        // 4. Save session state

        // For now, we'll simulate the update
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(load_result.error_code);
            return JsonIO::ErrorResponse("run-ticks", load_result.error_message, error_code_str);
        }

        // Update the tick count
        SessionMetadata metadata = load_result.data;
        metadata.total_ticks += ticks;

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

        const auto& metadata = result.data;

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("state", static_cast<int>(metadata.state));
        response_data.Add("circuit_file", Upp::String(metadata.circuit_file.c_str()));
        response_data.Add("total_ticks", metadata.total_ticks);
        response_data.Add("created_at", Upp::String(metadata.created_at.c_str()));
        response_data.Add("last_used_at", Upp::String(metadata.last_used_at.c_str()));

        Upp::ValueArray breakpoints_array;
        response_data.Add("breakpoints", breakpoints_array);

        Upp::ValueArray traces_array;
        response_data.Add("traces", traces_array);

        Upp::ValueArray signals_array;
        response_data.Add("signals", signals_array);

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
        // In a real implementation, this would:
        // 1. Load the session's machine instance
        // 2. Call Machine::GenerateNetlist(pcb_id)
        // 3. Write the result to a file in the session directory

        // For now, we'll just return a placeholder
        std::string netlist_file = opts.workspace + "/sessions/" +
                                  std::to_string(opts.session_id.value()) +
                                  "/netlists/netlist_" + std::to_string(pcb_id) + ".txt";

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("pcb_id", pcb_id);
        response_data.Add("netlist_file", Upp::String(netlist_file.c_str()));

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
        // In a real implementation, this would:
        // 1. Remove the session's machine instance from memory
        // 2. Delete the session directory and all its contents

        // For now, we'll just call the store's delete function
        auto result = session_store_->DeleteSession(opts.session_id.value());

        if (!result.ok) {
            std::string error_code_str = JsonIO::ErrorCodeToString(result.error_code);
            return JsonIO::ErrorResponse("destroy-session", result.error_message, error_code_str);
        }

        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("deleted", result.data);

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

} // namespace ProtoVMCLI