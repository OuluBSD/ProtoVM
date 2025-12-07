#include "CommandDispatcher.h"
#include "JsonIO.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

namespace ProtoVMCLI {

CommandDispatcher::CommandDispatcher(std::unique_ptr<ISessionStore> store) 
    : session_store_(std::move(store)) {
}

Upp::String CommandDispatcher::RunInitWorkspace(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("Workspace path is required", "INVALID_ARGUMENT");
    }
    
    try {
        fs::path workspace_path(opts.workspace);
        
        // Create workspace directory if it doesn't exist
        if (!fs::exists(workspace_path)) {
            fs::create_directories(workspace_path);
        }
        
        // Create subdirectories
        fs::create_directories(workspace_path / "sessions");
        fs::create_directories(workspace_path / "logs");
        fs::create_directories(workspace_path / "artifacts");
        
        // Write workspace.json
        Upp::ValueMap workspace_config;
        workspace_config.Add("version", "0.1");
        workspace_config.Add("created_at", "2025-01-01T00:00:00Z");  // In real implementation, use current time
        
        std::ofstream config_file(workspace_path / "workspace.json");
        config_file << Upp::String().Cat() << workspace_config;
        config_file.close();
        
        Upp::ValueMap response_data;
        response_data.Add("workspace", Upp::String(opts.workspace.c_str()));
        response_data.Add("created", true);
        response_data.Add("version", "0.1");
        
        return JsonIO::SuccessResponse(response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("Failed to initialize workspace: " + std::string(e.what()), 
                                    "WORKSPACE_INITIALIZATION_ERROR");
    }
}

Upp::String CommandDispatcher::RunCreateSession(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("Workspace path is required", "INVALID_ARGUMENT");
    }
    
    if (!opts.circuit_file.has_value()) {
        return JsonIO::ErrorResponse("Circuit file path is required", "INVALID_ARGUMENT");
    }
    
    if (!ValidateWorkspace(opts.workspace)) {
        return JsonIO::ErrorResponse("Invalid workspace path", "INVALID_WORKSPACE");
    }
    
    // Check if circuit file exists
    if (!fs::exists(opts.circuit_file.value())) {
        return JsonIO::ErrorResponse("Circuit file does not exist: " + opts.circuit_file.value(), 
                                    "CIRCUIT_FILE_NOT_FOUND");
    }
    
    try {
        SessionCreateInfo create_info(opts.workspace, opts.circuit_file.value());
        auto result = session_store_->CreateSession(create_info);
        
        if (!result.success) {
            return JsonIO::ErrorResponse(result.error.c_str(), result.error_code.c_str());
        }
        
        Upp::ValueMap response_data;
        response_data.Add("session_id", result.value);
        response_data.Add("workspace", Upp::String(opts.workspace.c_str()));
        response_data.Add("circuit_file", Upp::String(opts.circuit_file.value().c_str()));
        
        return JsonIO::SuccessResponse(response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("Failed to create session: " + std::string(e.what()), 
                                    "SESSION_CREATION_ERROR");
    }
}

Upp::String CommandDispatcher::RunListSessions(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("Workspace path is required", "INVALID_ARGUMENT");
    }
    
    if (!ValidateWorkspace(opts.workspace)) {
        return JsonIO::ErrorResponse("Invalid workspace path", "INVALID_WORKSPACE");
    }
    
    try {
        auto result = session_store_->ListSessions();
        
        if (!result.success) {
            return JsonIO::ErrorResponse(result.error.c_str(), result.error_code.c_str());
        }
        
        Upp::ValueArray sessions_array;
        
        for (const auto& session : result.value) {
            Upp::ValueMap session_obj;
            session_obj.Add("session_id", session.session_id);
            session_obj.Add("state", static_cast<int>(session.state));
            session_obj.Add("circuit_file", Upp::String(session.circuit_file.c_str()));
            session_obj.Add("created_at", Upp::String(session.created_at.c_str()));
            session_obj.Add("last_used_at", Upp::String(session.last_used_at.c_str()));
            sessions_array.Add(session_obj);
        }
        
        Upp::ValueMap response_data;
        response_data.Add("sessions", sessions_array);
        
        return JsonIO::SuccessResponse(response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("Failed to list sessions: " + std::string(e.what()), 
                                    "SESSION_LIST_ERROR");
    }
}

Upp::String CommandDispatcher::RunRunTicks(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("Workspace path is required", "INVALID_ARGUMENT");
    }
    
    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("Session ID is required", "INVALID_ARGUMENT");
    }
    
    int ticks = opts.ticks.value_or(1);
    if (ticks <= 0) {
        return JsonIO::ErrorResponse("Ticks must be positive", "INVALID_ARGUMENT");
    }
    
    try {
        // In a real implementation, this would:
        // 1. Load the session's machine instance
        // 2. Execute ticks on the machine
        // 3. Update session metadata with new tick count
        // 4. Save session state
        
        // For now, we'll simulate the update
        auto load_result = session_store_->LoadSession(opts.session_id.value());
        if (!load_result.success) {
            return JsonIO::ErrorResponse(load_result.error.c_str(), load_result.error_code.c_str());
        }
        
        // Update the tick count
        SessionMetadata metadata = load_result.value;
        metadata.total_ticks += ticks;
        
        auto save_result = session_store_->SaveSession(metadata);
        if (!save_result.success) {
            return JsonIO::ErrorResponse(save_result.error.c_str(), save_result.error_code.c_str());
        }
        
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("ticks_run", ticks);
        response_data.Add("total_ticks", metadata.total_ticks);
        
        return JsonIO::SuccessResponse(response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("Failed to run ticks: " + std::string(e.what()), 
                                    "RUN_TICKS_ERROR");
    }
}

Upp::String CommandDispatcher::RunGetState(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("Workspace path is required", "INVALID_ARGUMENT");
    }
    
    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("Session ID is required", "INVALID_ARGUMENT");
    }
    
    try {
        auto result = session_store_->LoadSession(opts.session_id.value());
        
        if (!result.success) {
            return JsonIO::ErrorResponse(result.error.c_str(), result.error_code.c_str());
        }
        
        const auto& metadata = result.value;
        
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("circuit_name", Upp::String(metadata.circuit_file.c_str()));
        response_data.Add("total_ticks", metadata.total_ticks);
        
        Upp::ValueArray breakpoints_array;
        response_data.Add("breakpoints_hit", breakpoints_array);
        
        Upp::ValueArray traces_array;
        response_data.Add("traces", traces_array);
        
        return JsonIO::SuccessResponse(response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("Failed to get state: " + std::string(e.what()), 
                                    "GET_STATE_ERROR");
    }
}

Upp::String CommandDispatcher::RunExportNetlist(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("Workspace path is required", "INVALID_ARGUMENT");
    }
    
    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("Session ID is required", "INVALID_ARGUMENT");
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
        
        return JsonIO::SuccessResponse(response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("Failed to export netlist: " + std::string(e.what()), 
                                    "NETLIST_EXPORT_ERROR");
    }
}

Upp::String CommandDispatcher::RunDestroySession(const CommandOptions& opts) {
    if (opts.workspace.empty()) {
        return JsonIO::ErrorResponse("Workspace path is required", "INVALID_ARGUMENT");
    }
    
    if (!opts.session_id.has_value()) {
        return JsonIO::ErrorResponse("Session ID is required", "INVALID_ARGUMENT");
    }
    
    try {
        // In a real implementation, this would:
        // 1. Remove the session's machine instance from memory
        // 2. Delete the session directory and all its contents
        
        // For now, we'll just call the store's delete function
        auto result = session_store_->DeleteSession(opts.session_id.value());
        
        if (!result.success) {
            return JsonIO::ErrorResponse(result.error.c_str(), result.error_code.c_str());
        }
        
        Upp::ValueMap response_data;
        response_data.Add("session_id", opts.session_id.value());
        response_data.Add("deleted", result.value);
        
        return JsonIO::SuccessResponse(response_data);
    } catch (const std::exception& e) {
        return JsonIO::ErrorResponse("Failed to destroy session: " + std::string(e.what()), 
                                    "SESSION_DELETION_ERROR");
    }
}

bool CommandDispatcher::ValidateWorkspace(const std::string& workspace_path) {
    try {
        fs::path path(workspace_path);
        return fs::exists(path) && fs::is_directory(path);
    } catch (...) {
        return false;
    }
}

int CommandDispatcher::GetNextSessionId() {
    // In a real implementation, this would scan the sessions directory
    // and find the next available ID
    // For now, we'll just return 1 (this would need to be more sophisticated in real impl)
    return 1;
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