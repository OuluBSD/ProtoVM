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
            "unknown", "No command specified", "MISSING_COMMAND");
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
    if (args.Find("user-id") >= 0) {
        opts.user_id = args.Get("user-id", Upp::String("anonymous")).ToStd();
    }
    if (args.Find("branch") >= 0) {
        opts.branch = args.Get("branch", Upp::String("")).ToStd();
    }
    if (args.Find("from-branch") >= 0) {
        opts.branch_from = args.Get("from-branch", Upp::String("")).ToStd();
    }
    if (args.Find("to-branch") >= 0) {
        opts.branch_to = args.Get("to-branch", Upp::String("")).ToStd();
    }
    if (args.Find("branch-name") >= 0) {
        opts.branch_name = args.Get("branch-name", Upp::String("")).ToStd();
    }
    // Graph query parameters
    if (args.Find("graph-source-kind") >= 0) {
        opts.graph_source_kind = args.Get("graph-source-kind", Upp::String("")).ToStd();
    }
    if (args.Find("graph-source-id") >= 0) {
        opts.graph_source_id = args.Get("graph-source-id", Upp::String("")).ToStd();
    }
    if (args.Find("graph-target-kind") >= 0) {
        opts.graph_target_kind = args.Get("graph-target-kind", Upp::String("")).ToStd();
    }
    if (args.Find("graph-target-id") >= 0) {
        opts.graph_target_id = args.Get("graph-target-id", Upp::String("")).ToStd();
    }
    if (args.Find("graph-node-kind") >= 0) {
        opts.graph_node_kind = args.Get("graph-node-kind", Upp::String("")).ToStd();
    }
    if (args.Find("graph-node-id") >= 0) {
        opts.graph_node_id = args.Get("graph-node-id", Upp::String("")).ToStd();
    }
    if (args.Find("graph-max-depth") >= 0) {
        try {
            opts.graph_max_depth = args.Get("graph-max-depth", 128);
        } catch (...) {
            opts.graph_max_depth = 128;
        }
    }
    // Refactoring parameters
    if (args.Find("block-id") >= 0) {
        opts.block_id = args.Get("block-id", Upp::String("")).ToStd();
    }
    if (args.Find("max-plans") >= 0) {
        opts.max_plans = args.Get("max-plans", Upp::String("")).ToStd();
    }
    if (args.Find("plan-id") >= 0) {
        opts.plan_id = args.Get("plan-id", Upp::String("")).ToStd();
    }
    // Instrument parameters
    if (args.Find("instrument-id") >= 0) {
        opts.instrument_id = args.Get("instrument-id", Upp::String("")).ToStd();
    }
    if (args.Find("instrument-from-json") >= 0) {
        opts.instrument_from_json = args.Get("instrument-from-json", Upp::String("")).ToStd();
    }
    if (args.Find("analog-block-id") >= 0) {
        opts.analog_block_id = args.Get("analog-block-id", Upp::String("")).ToStd();
    }
    if (args.Find("digital-block-id") >= 0) {
        opts.digital_block_id = args.Get("digital-block-id", Upp::String("")).ToStd();
    }
    if (args.Find("use-analog-primary") >= 0) {
        opts.use_analog_primary = true;
    }
    if (args.Find("voice-count") >= 0) {
        opts.voice_count = args.Get("voice-count", Upp::String("")).ToStd();
    }
    if (args.Find("sample-rate") >= 0) {
        opts.sample_rate = args.Get("sample-rate", Upp::String("")).ToStd();
    }
    if (args.Find("duration-sec") >= 0) {
        opts.duration_sec = args.Get("duration-sec", Upp::String("")).ToStd();
    }
    if (args.Find("base-freq-hz") >= 0) {
        opts.base_freq_hz = args.Get("base-freq-hz", Upp::String("")).ToStd();
    }
    if (args.Find("detune-spread-cents") >= 0) {
        opts.detune_spread_cents = args.Get("detune-spread-cents", Upp::String("")).ToStd();
    }
    if (args.Find("pan-lfo-hz") >= 0) {
        opts.pan_lfo_hz = args.Get("pan-lfo-hz", Upp::String("")).ToStd();
    }
    // Plugin export parameters
    if (args.Find("plugin-target") >= 0) {
        opts.plugin_target = args.Get("plugin-target", Upp::String("")).ToStd();
    }
    if (args.Find("plugin-name") >= 0) {
        opts.plugin_name = args.Get("plugin-name", Upp::String("")).ToStd();
    }
    if (args.Find("plugin-id") >= 0) {
        opts.plugin_id = args.Get("plugin-id", Upp::String("")).ToStd();
    }
    if (args.Find("vendor") >= 0) {
        opts.vendor = args.Get("vendor", Upp::String("")).ToStd();
    }
    if (args.Find("version") >= 0) {
        opts.version = args.Get("version", Upp::String("")).ToStd();
    }
    if (args.Find("output-dir") >= 0) {
        opts.output_dir = args.Get("output-dir", Upp::String("")).ToStd();
    }

    // Create session store
    auto session_store = ProtoVMCLI::CreateFilesystemSessionStore(opts.workspace);
    if (!session_store) {
        Upp::String error_response = ProtoVMCLI::JsonIO::ErrorResponse(
            command.ToStd(), "Failed to create session store", "SESSION_STORE_INIT_ERROR");
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
    else if (command == "edit-add-component") {
        result = dispatcher.RunEditAddComponent(opts);
    }
    else if (command == "edit-remove-component") {
        result = dispatcher.RunEditRemoveComponent(opts);
    }
    else if (command == "edit-move-component") {
        result = dispatcher.RunEditMoveComponent(opts);
    }
    else if (command == "edit-set-component-property") {
        result = dispatcher.RunEditSetComponentProperty(opts);
    }
    else if (command == "edit-connect") {
        result = dispatcher.RunEditConnect(opts);
    }
    else if (command == "edit-disconnect") {
        result = dispatcher.RunEditDisconnect(opts);
    }
    else if (command == "edit-get-circuit") {
        result = dispatcher.RunEditGetCircuit(opts);
    }
    else if (command == "lint-circuit") {
        result = dispatcher.RunLintCircuit(opts);
    }
    else if (command == "analyze-circuit") {
        result = dispatcher.RunAnalyzeCircuit(opts);
    }
    else if (command == "circuit-diff") {
        result = dispatcher.RunCircuitDiff(opts);
    }
    else if (command == "circuit-patch") {
        result = dispatcher.RunCircuitPatch(opts);
    }
    else if (command == "circuit-replay") {
        result = dispatcher.RunCircuitReplay(opts);
    }
    else if (command == "circuit-history") {
        result = dispatcher.RunCircuitHistory(opts);
    }
    else if (command == "branch-list") {
        result = dispatcher.RunBranchList(opts);
    }
    else if (command == "branch-create") {
        result = dispatcher.RunBranchCreate(opts);
    }
    else if (command == "branch-switch") {
        result = dispatcher.RunBranchSwitch(opts);
    }
    else if (command == "branch-delete") {
        result = dispatcher.RunBranchDelete(opts);
    }
    else if (command == "branch-merge") {
        result = dispatcher.RunBranchMerge(opts);
    }
    else if (command == "graph-export") {
        result = dispatcher.RunGraphExport(opts);
    }
    else if (command == "graph-paths") {
        result = dispatcher.RunGraphPaths(opts);
    }
    else if (command == "graph-fanin") {
        result = dispatcher.RunGraphFanIn(opts);
    }
    else if (command == "graph-fanout") {
        result = dispatcher.RunGraphFanOut(opts);
    }
    else if (command == "graph-stats") {
        result = dispatcher.RunGraphStats(opts);
    }
    else if (command == "refactor-suggest") {
        result = dispatcher.RunRefactorSuggest(opts);
    }
    else if (command == "refactor-suggest-block") {
        result = dispatcher.RunRefactorSuggestBlock(opts);
    }
    else if (command == "refactor-apply") {
        result = dispatcher.RunRefactorApply(opts);
    }
    else if (command == "schedule-block") {
        result = dispatcher.RunScheduleBlock(opts);
    }
    else if (command == "schedule-node-region") {
        result = dispatcher.RunScheduleNodeRegion(opts);
    }
    else if (command == "pipeline-block") {
        result = dispatcher.RunPipelineBlock(opts);
    }
    else if (command == "pipeline-subsystem") {
        result = dispatcher.RunPipelineSubsystem(opts);
    }
    else if (command == "instrument-build-hybrid") {
        result = dispatcher.RunInstrumentBuildHybrid(opts);
    }
    else if (command == "instrument-render-hybrid") {
        result = dispatcher.RunInstrumentRenderHybrid(opts);
    }
    else if (command == "instrument-export-cpp") {
        result = dispatcher.RunInstrumentExportCpp(opts);
    }
    else if (command == "instrument-export-plugin-skeleton") {
        result = dispatcher.RunInstrumentExportPluginSkeleton(opts);
    }
    else if (command == "instrument-export-plugin-project") {
        result = dispatcher.RunInstrumentExportPluginProject(opts);
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
                        "debug-process-logs", "Process ID is required for logs command", "MISSING_ID");
                } else {
                    // Start streaming process logs
                    result = dispatcher.RunDebugProcessLogs(process_id);
                }
            } else {
                result = ProtoVMCLI::JsonIO::ErrorResponse(
                    "debug-process-logs", "Unknown debug process action: " + action.ToStd(), "UNKNOWN_ACTION");
            }
        }
        else if (subcommand == "websocket") {
            Upp::String action = args.Get("action", Upp::String(""));
            if (action == "stream") {
                // Extract websocket ID
                Upp::String ws_id = args.Get("id", Upp::String(""));
                if (ws_id.IsEmpty()) {
                    result = ProtoVMCLI::JsonIO::ErrorResponse(
                        "debug-websocket-stream", "WebSocket ID is required for stream command", "MISSING_ID");
                } else {
                    // Start streaming websocket frames
                    result = dispatcher.RunDebugWebSocketStream(ws_id);
                }
            } else {
                result = ProtoVMCLI::JsonIO::ErrorResponse(
                    "debug-websocket-stream", "Unknown debug websocket action: " + action.ToStd(), "UNKNOWN_ACTION");
            }
        }
        else if (subcommand == "poll") {
            Upp::String action = args.Get("action", Upp::String(""));
            if (action == "stream") {
                // Extract poll ID
                Upp::String poll_id = args.Get("id", Upp::String(""));
                if (poll_id.IsEmpty()) {
                    result = ProtoVMCLI::JsonIO::ErrorResponse(
                        "debug-poll-stream", "Poll ID is required for stream command", "MISSING_ID");
                } else {
                    // Start streaming poll events
                    result = dispatcher.RunDebugPollStream(poll_id);
                }
            } else {
                result = ProtoVMCLI::JsonIO::ErrorResponse(
                    "debug-poll-stream", "Unknown debug poll action: " + action.ToStd(), "UNKNOWN_ACTION");
            }
        }
        else {
            result = ProtoVMCLI::JsonIO::ErrorResponse(
                "debug", "Unknown debug subcommand: " + subcommand.ToStd(), "UNKNOWN_SUBCOMMAND");
        }
    }
    else {
        result = ProtoVMCLI::JsonIO::ErrorResponse(
            command.ToStd(), "Unknown command: " + command.ToStd(), "UNKNOWN_COMMAND");
    }

    // Output result as JSON
    std::cout << result.ToStd() << std::endl;

    // Extract success status from JSON response
    // Look for "ok": true in the result string
    std::string result_str = result.ToStd();
    bool success = result_str.find("\"ok\":true") != std::string::npos;

    return success ? 0 : 1;
}