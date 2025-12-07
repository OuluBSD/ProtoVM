#include "CommandDispatcher.h"
#include "JsonIO.h"
#include "SessionTypes.h"
#include <iostream>
#include <string>

// Forward declaration of the factory function (defined in JsonFilesystemSessionStore.cpp)
namespace ProtoVMCLI {
    std::unique_ptr<ISessionStore> CreateFilesystemSessionStore(const std::string& workspace_path);
}


int main(int argc, char** argv) {
    // Parse command-line arguments
    auto args = ProtoVMCLI::JsonIO::ParseArgs(argc, argv);
    
    // Extract command
    Upp::String command = args.Get("command", Upp::String(""));
    if (command.IsEmpty()) {
        // No command provided, return an error
        Upp::String error_response = ProtoVMCLI::JsonIO::ErrorResponse(
            "No command specified", "MISSING_COMMAND");
        std::cout << error_response.ToStd() << std::endl;
        return 1;
    }
    
    // Extract common options
    ProtoVMCLI::CommandOptions opts;
    opts.workspace = args.Get("workspace", Upp::String("")).ToStd();
    
    // Extract optional parameters
    if (args.Find("session-id") >= 0) {
        try {
            opts.session_id = args.Get("session-id", 0);
        } catch (...) {
            opts.session_id = 0;
        }
    }
    if (args.Find("ticks") >= 0) {
        try {
            opts.ticks = args.Get("ticks", 0);
        } catch (...) {
            opts.ticks = 0;
        }
    }
    if (args.Find("pcb-id") >= 0) {
        try {
            opts.pcb_id = args.Get("pcb-id", 0);
        } catch (...) {
            opts.pcb_id = 0;
        }
    }
    if (args.Find("circuit-file") >= 0) {
        opts.circuit_file = args.Get("circuit-file", Upp::String("")).ToStd();
    }
    if (args.Find("netlist-file") >= 0) {
        opts.netlist_file = args.Get("netlist-file", Upp::String("")).ToStd();
    }
    
    // Create session store
    auto session_store = ProtoVMCLI::CreateFilesystemSessionStore(opts.workspace);
    if (!session_store) {
        Upp::String error_response = ProtoVMCLI::JsonIO::ErrorResponse(
            "Failed to create session store", "SESSION_STORE_INIT_ERROR");
        std::cout << error_response.ToStd() << std::endl;
        return 1;
    }
    
    // Create command dispatcher
    ProtoVMCLI::CommandDispatcher dispatcher(std::move(session_store));
    
    // Dispatch to appropriate command handler
    Upp::String result;

    if (command == "init-workspace") {
        result = dispatcher.RunInitWorkspace(opts);
    }
    else if (command == "create-session") {
        result = dispatcher.RunCreateSession(opts);
    }
    else if (command == "list-sessions") {
        result = dispatcher.RunListSessions(opts);
    }
    else if (command == "run-ticks") {
        result = dispatcher.RunRunTicks(opts);
    }
    else if (command == "get-state") {
        result = dispatcher.RunGetState(opts);
    }
    else if (command == "export-netlist") {
        result = dispatcher.RunExportNetlist(opts);
    }
    else if (command == "destroy-session") {
        result = dispatcher.RunDestroySession(opts);
    }
    else if (command == "debug") {
        // Extract subcommand for debug
        Upp::String subcommand = args.Get("subcommand", Upp::String(""));

        if (subcommand == "process") {
            Upp::String action = args.Get("action", Upp::String(""));
            if (action == "logs") {
                // Extract process ID
                int process_id = args.Get("id", -1);
                if (process_id == -1) {
                    result = ProtoVMCLI::JsonIO::ErrorResponse(
                        "Process ID is required for logs command", "MISSING_ID");
                } else {
                    // Start streaming process logs
                    result = dispatcher.RunDebugProcessLogs(process_id);
                }
            } else {
                result = ProtoVMCLI::JsonIO::ErrorResponse(
                    "Unknown debug process action: " + action.ToStd(), "UNKNOWN_ACTION");
            }
        }
        else if (subcommand == "websocket") {
            Upp::String action = args.Get("action", Upp::String(""));
            if (action == "stream") {
                // Extract websocket ID
                Upp::String ws_id = args.Get("id", Upp::String(""));
                if (ws_id.IsEmpty()) {
                    result = ProtoVMCLI::JsonIO::ErrorResponse(
                        "WebSocket ID is required for stream command", "MISSING_ID");
                } else {
                    // Start streaming websocket frames
                    result = dispatcher.RunDebugWebSocketStream(ws_id);
                }
            } else {
                result = ProtoVMCLI::JsonIO::ErrorResponse(
                    "Unknown debug websocket action: " + action.ToStd(), "UNKNOWN_ACTION");
            }
        }
        else if (subcommand == "poll") {
            Upp::String action = args.Get("action", Upp::String(""));
            if (action == "stream") {
                // Extract poll ID
                Upp::String poll_id = args.Get("id", Upp::String(""));
                if (poll_id.IsEmpty()) {
                    result = ProtoVMCLI::JsonIO::ErrorResponse(
                        "Poll ID is required for stream command", "MISSING_ID");
                } else {
                    // Start streaming poll events
                    result = dispatcher.RunDebugPollStream(poll_id);
                }
            } else {
                result = ProtoVMCLI::JsonIO::ErrorResponse(
                    "Unknown debug poll action: " + action.ToStd(), "UNKNOWN_ACTION");
            }
        }
        else {
            result = ProtoVMCLI::JsonIO::ErrorResponse(
                "Unknown debug subcommand: " + subcommand.ToStd(), "UNKNOWN_SUBCOMMAND");
        }
    }
    else {
        result = ProtoVMCLI::JsonIO::ErrorResponse(
            "Unknown command: " + command.ToStd(), "UNKNOWN_COMMAND");
    }

    // Output result as JSON
    std::cout << result.ToStd() << std::endl;

    // Extract success status from JSON response
    // Look for "ok": true in the result string
    std::string result_str = result.ToStd();
    bool success = result_str.find("\"ok\":true") != std::string::npos;

    return success ? 0 : 1;
}