#include "SessionServer.h"
#include "JsonIO.h"
#include "SessionStore.h"
#include "JsonFilesystemSessionStore.h"
#include "CircuitGraph.h"
#include "CircuitGraphQueries.h"
#include "FunctionalAnalysis.h"
#include "BehavioralAnalysis.h"
#include "DiffAnalysis.h"
#include "IrOptimization.h"
#include "Playbooks.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace ProtoVMCLI {

SessionServer::SessionServer() {
    // Initialize CoDesignerManager with a circuit facade
    // We'll create a circuit facade without a session store since the CoDesigner
    // manager is for in-memory sessions only
    auto circuit_facade = std::make_shared<CircuitFacade>();
    co_designer_manager_ = std::make_shared<CoDesignerManager>(circuit_facade);
}

SessionServer::~SessionServer() {
}

Result<void> SessionServer::HandleRequest(const DaemonRequest& req, DaemonResponse& out_resp) {
    try {
        Result<DaemonResponse> result;
        
        if (req.command == "init-workspace") {
            result = HandleInitWorkspace(req);
        }
        else if (req.command == "create-session") {
            result = HandleCreateSession(req);
        }
        else if (req.command == "list-sessions") {
            result = HandleListSessions(req);
        }
        else if (req.command == "run-ticks") {
            result = HandleRunTicks(req);
        }
        else if (req.command == "get-state") {
            result = HandleGetState(req);
        }
        else if (req.command == "export-netlist") {
            result = HandleExportNetlist(req);
        }
        else if (req.command == "destroy-session") {
            result = HandleDestroySession(req);
        }
        else if (req.command == "lint-circuit") {
            result = HandleLintCircuit(req);
        }
        else if (req.command == "analyze-circuit") {
            result = HandleAnalyzeCircuit(req);
        }
        else if (req.command == "edit-add-component") {
            result = HandleEditAddComponent(req);
        }
        else if (req.command == "edit-remove-component") {
            result = HandleEditRemoveComponent(req);
        }
        else if (req.command == "edit-move-component") {
            result = HandleEditMoveComponent(req);
        }
        else if (req.command == "edit-set-component-property") {
            result = HandleEditSetComponentProperty(req);
        }
        else if (req.command == "edit-connect") {
            result = HandleEditConnect(req);
        }
        else if (req.command == "edit-disconnect") {
            result = HandleEditDisconnect(req);
        }
        else if (req.command == "edit-get-circuit") {
            result = HandleEditGetCircuit(req);
        }
        else if (req.command == "circuit-diff") {
            result = HandleCircuitDiff(req);
        }
        else if (req.command == "circuit-patch") {
            result = HandleCircuitPatch(req);
        }
        else if (req.command == "circuit-replay") {
            result = HandleCircuitReplay(req);
        }
        else if (req.command == "circuit-history") {
            result = HandleCircuitHistory(req);
        }
        else if (req.command == "branch-list") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the branch-list command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBranchList(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "branch-create") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch parameters from payload
            if (req.payload.Get("branch_name", Upp::Value()).IsString()) {
                opts.branch_name = req.payload.Get("branch_name", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("from_branch", Upp::Value()).IsString()) {
                opts.branch_from = req.payload.Get("from_branch", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the branch-create command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBranchCreate(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "branch-switch") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch parameters from payload
            if (req.payload.Get("branch_name", Upp::Value()).IsString()) {
                opts.branch_name = req.payload.Get("branch_name", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the branch-switch command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBranchSwitch(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "branch-delete") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch parameters from payload
            if (req.payload.Get("branch_name", Upp::Value()).IsString()) {
                opts.branch_name = req.payload.Get("branch_name", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the branch-delete command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBranchDelete(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "branch-merge") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch parameters from payload
            if (req.payload.Get("source_branch", Upp::Value()).IsString()) {
                opts.branch_from = req.payload.Get("source_branch", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("target_branch", Upp::Value()).IsString()) {
                opts.branch_to = req.payload.Get("target_branch", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("allow_merge", Upp::Value()).IsBool()) {
                // We might need to handle this differently if needed
            }

            // Use the CommandDispatcher to handle the branch-merge command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBranchMerge(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "refactor-suggest") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract max_plans from payload if provided
            if (req.payload.Get("max_plans", Upp::Value()).IsString()) {
                opts.max_plans = req.payload.Get("max_plans", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the refactor-suggest command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunRefactorSuggest(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "refactor-suggest-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Extract max_plans from payload if provided
            if (req.payload.Get("max_plans", Upp::Value()).IsString()) {
                opts.max_plans = req.payload.Get("max_plans", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the refactor-suggest-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunRefactorSuggestBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "refactor-apply") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract plan_id from payload
            if (req.payload.Get("plan_id", Upp::Value()).IsString()) {
                opts.plan_id = req.payload.Get("plan_id", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the refactor-apply command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunRefactorApply(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "graph-export") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the graph-export command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunGraphExport(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "graph-paths") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract graph-specific parameters from payload
            if (req.payload.Get("source", Upp::Value()).IsMap()) {
                Upp::ValueMap source_map = req.payload.Get("source", Upp::ValueMap());
                if (source_map.Get("kind", Upp::Value()).IsString()) {
                    opts.graph_source_kind = source_map.Get("kind", Upp::Value()).ToString().ToStd();
                }
                if (source_map.Get("id", Upp::Value()).IsString()) {
                    opts.graph_source_id = source_map.Get("id", Upp::Value()).ToString().ToStd();
                }
            }

            if (req.payload.Get("target", Upp::Value()).IsMap()) {
                Upp::ValueMap target_map = req.payload.Get("target", Upp::ValueMap());
                if (target_map.Get("kind", Upp::Value()).IsString()) {
                    opts.graph_target_kind = target_map.Get("kind", Upp::Value()).ToString().ToStd();
                }
                if (target_map.Get("id", Upp::Value()).IsString()) {
                    opts.graph_target_id = target_map.Get("id", Upp::Value()).ToString().ToStd();
                }
            }

            if (req.payload.Get("max_depth", Upp::Value()).IsInt()) {
                opts.graph_max_depth = req.payload.Get("max_depth", Upp::Value(128)).ToInt();
            }

            // Use the CommandDispatcher to handle the graph-paths command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunGraphPaths(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "graph-fanin") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract graph-specific parameters from payload
            if (req.payload.Get("node", Upp::Value()).IsMap()) {
                Upp::ValueMap node_map = req.payload.Get("node", Upp::ValueMap());
                if (node_map.Get("kind", Upp::Value()).IsString()) {
                    opts.graph_node_kind = node_map.Get("kind", Upp::Value()).ToString().ToStd();
                }
                if (node_map.Get("id", Upp::Value()).IsString()) {
                    opts.graph_node_id = node_map.Get("id", Upp::Value()).ToString().ToStd();
                }
            }

            if (req.payload.Get("max_depth", Upp::Value()).IsInt()) {
                opts.graph_max_depth = req.payload.Get("max_depth", Upp::Value(128)).ToInt();
            }

            // Use the CommandDispatcher to handle the graph-fanin command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunGraphFanIn(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "graph-fanout") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract graph-specific parameters from payload
            if (req.payload.Get("node", Upp::Value()).IsMap()) {
                Upp::ValueMap node_map = req.payload.Get("node", Upp::ValueMap());
                if (node_map.Get("kind", Upp::Value()).IsString()) {
                    opts.graph_node_kind = node_map.Get("kind", Upp::Value()).ToString().ToStd();
                }
                if (node_map.Get("id", Upp::Value()).IsString()) {
                    opts.graph_node_id = node_map.Get("id", Upp::Value()).ToString().ToStd();
                }
            }

            if (req.payload.Get("max_depth", Upp::Value()).IsInt()) {
                opts.graph_max_depth = req.payload.Get("max_depth", Upp::Value(128)).ToInt();
            }

            // Use the CommandDispatcher to handle the graph-fanout command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunGraphFanOut(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "graph-stats") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the graph-stats command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunGraphStats(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "timing-summary") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the timing-summary command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunTimingSummary(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "timing-critical-paths") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract max_paths and max_depth from payload if provided
            if (req.payload.Get("max_paths", Upp::Value()).Is<int>()) {
                opts.payload.Add("max_paths", req.payload.Get("max_paths", Upp::Value()));
            }
            if (req.payload.Get("max_depth", Upp::Value()).Is<int>()) {
                opts.payload.Add("max_depth", req.payload.Get("max_depth", Upp::Value()));
            }

            // Use the CommandDispatcher to handle the timing-critical-paths command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunTimingCriticalPaths(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "timing-loops") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the timing-loops command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunTimingLoops(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "timing-hazards") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract max_results from payload if provided
            if (req.payload.Get("max_results", Upp::Value()).Is<int>()) {
                opts.payload.Add("max_results", req.payload.Get("max_results", Upp::Value()));
            }

            // Use the CommandDispatcher to handle the timing-hazards command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunTimingHazards(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "deps-summary") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract node parameters from payload
            if (req.payload.Get("node_id", Upp::Value()).IsString()) {
                opts.deps_node_id = req.payload.Get("node_id", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("node_kind", Upp::Value()).IsString()) {
                opts.deps_node_kind = req.payload.Get("node_kind", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("max_depth", Upp::Value()).Is<int>()) {
                opts.deps_max_depth = req.payload.Get("max_depth", Upp::Value(128)).ToInt();
            }

            // Use the CommandDispatcher to handle the deps-summary command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunDepsSummary(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "deps-backward") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract node parameters from payload
            if (req.payload.Get("node_id", Upp::Value()).IsString()) {
                opts.deps_node_id = req.payload.Get("node_id", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("node_kind", Upp::Value()).IsString()) {
                opts.deps_node_kind = req.payload.Get("node_kind", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("max_depth", Upp::Value()).Is<int>()) {
                opts.deps_max_depth = req.payload.Get("max_depth", Upp::Value(128)).ToInt();
            }

            // Use the CommandDispatcher to handle the deps-backward command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunDepsBackward(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "deps-forward") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract node parameters from payload
            if (req.payload.Get("node_id", Upp::Value()).IsString()) {
                opts.deps_node_id = req.payload.Get("node_id", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("node_kind", Upp::Value()).IsString()) {
                opts.deps_node_kind = req.payload.Get("node_kind", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("max_depth", Upp::Value()).Is<int>()) {
                opts.deps_max_depth = req.payload.Get("max_depth", Upp::Value(128)).ToInt();
            }

            // Use the CommandDispatcher to handle the deps-forward command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunDepsForward(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "deps-both") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract node parameters from payload
            if (req.payload.Get("node_id", Upp::Value()).IsString()) {
                opts.deps_node_id = req.payload.Get("node_id", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("node_kind", Upp::Value()).IsString()) {
                opts.deps_node_kind = req.payload.Get("node_kind", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("max_depth", Upp::Value()).Is<int>()) {
                opts.deps_max_depth = req.payload.Get("max_depth", Upp::Value(128)).ToInt();
            }

            // Use the CommandDispatcher to handle the deps-both command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunDepsBoth(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "blocks-list") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the blocks-list command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBlocksList(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "blocks-export") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the blocks-export command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBlocksExport(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "block-inspect") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the block-inspect command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBlockInspect(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "behavior-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the behavior-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBehaviorBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "behavior-node") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract node parameters from payload
            if (req.payload.Get("node_id", Upp::Value()).IsString()) {
                opts.node_id = req.payload.Get("node_id", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("node_kind", Upp::Value()).IsString()) {
                opts.node_kind = req.payload.Get("node_kind", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the behavior-node command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBehaviorNode(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "ir-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the ir-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunIrBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "ir-opt-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Extract passes from payload
            if (req.payload.Get("passes", Upp::Value()).IsArray()) {
                Upp::ValueArray passes_array = req.payload.Get("passes", Upp::ValueArray());
                std::string passes_str;
                for (int i = 0; i < passes_array.GetCount(); ++i) {
                    if (i > 0) passes_str += ",";
                    passes_str += passes_array[i].ToString().ToStd();
                }
                opts.passes = passes_str;
            }

            // Use the CommandDispatcher to handle the ir-opt-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunIrOptBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "ir-opt-refactor-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Extract passes from payload
            if (req.payload.Get("passes", Upp::Value()).IsArray()) {
                Upp::ValueArray passes_array = req.payload.Get("passes", Upp::ValueArray());
                std::string passes_str;
                for (int i = 0; i < passes_array.GetCount(); ++i) {
                    if (i > 0) passes_str += ",";
                    passes_str += passes_array[i].ToString().ToStd();
                }
                opts.passes = passes_str;
            }

            // Use the CommandDispatcher to handle the ir-opt-refactor-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunIrOptRefactorBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "ir-node-region") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract node parameters from payload
            if (req.payload.Get("node_id", Upp::Value()).IsString()) {
                opts.node_id = req.payload.Get("node_id", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("node_kind", Upp::Value()).IsString()) {
                opts.node_kind = req.payload.Get("node_kind", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("max_depth", Upp::Value()).IsInt()) {
                opts.max_depth = req.payload.Get("max_depth", Upp::Value(4)).ToInt();
            }

            // Use the CommandDispatcher to handle the ir-node-region command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunIrNodeRegion(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "schedule-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Extract scheduling parameters from payload
            opts.payload = req.payload; // Pass all payload options to the handler

            // Use the CommandDispatcher to handle the schedule-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunScheduleBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "schedule-node-region") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract node parameters from payload
            if (req.payload.Get("node_id", Upp::Value()).IsString()) {
                opts.node_id = req.payload.Get("node_id", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("node_kind", Upp::Value()).IsString()) {
                opts.node_kind = req.payload.Get("node_kind", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("max_depth", Upp::Value()).IsInt()) {
                opts.max_depth = req.payload.Get("max_depth", Upp::Value(4)).ToInt();
            }

            // Extract scheduling parameters from payload
            opts.payload = req.payload; // Pass all payload options to the handler

            // Use the CommandDispatcher to handle the schedule-node-region command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunScheduleNodeRegion(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "pipeline-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the pipeline-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunPipelineBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "pipeline-subsystem") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch from payload if provided
            if (req.payload.Get("branch", Upp::Value()).IsString()) {
                opts.branch = req.payload.Get("branch", Upp::Value()).ToString().ToStd();
            }

            // Extract subsystem_id from payload
            if (req.payload.Get("subsystem_id", Upp::Value()).IsString()) {
                opts.subsystem_id = req.payload.Get("subsystem_id", Upp::Value()).ToString().ToStd();
            }

            // Extract block_ids from payload
            if (req.payload.Get("block_ids", Upp::Value()).IsArray()) {
                Upp::ValueArray block_ids_array = req.payload.Get("block_ids").ToValueArray();
                std::string block_ids_str;
                for (int i = 0; i < block_ids_array.GetCount(); ++i) {
                    if (i > 0) block_ids_str += ",";
                    block_ids_str += block_ids_array[i].ToString().ToStd();
                }
                opts.block_ids = block_ids_str;
            }

            // Use the CommandDispatcher to handle the pipeline-subsystem command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunPipelineSubsystem(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "behavior-diff-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch parameters from payload
            if (req.payload.Get("branch_before", Upp::Value()).IsString()) {
                opts.branch_before = req.payload.Get("branch_before", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("branch_after", Upp::Value()).IsString()) {
                opts.branch_after = req.payload.Get("branch_after", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the behavior-diff-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunBehaviorDiffBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "ir-diff-block") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch parameters from payload
            if (req.payload.Get("branch_before", Upp::Value()).IsString()) {
                opts.branch_before = req.payload.Get("branch_before", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("branch_after", Upp::Value()).IsString()) {
                opts.branch_after = req.payload.Get("branch_after", Upp::Value()).ToString().ToStd();
            }

            // Extract block_id from payload
            if (req.payload.Get("block_id", Upp::Value()).IsString()) {
                opts.block_id = req.payload.Get("block_id", Upp::Value()).ToString().ToStd();
            }

            // Use the CommandDispatcher to handle the ir-diff-block command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunIrDiffBlock(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "ir-diff-node-region") {
            // Create a CommandOptions object from the DaemonRequest
            CommandOptions opts;
            opts.workspace = req.workspace;
            opts.session_id = req.session_id;
            opts.user_id = req.user_id;

            // Extract branch parameters from payload
            if (req.payload.Get("branch_before", Upp::Value()).IsString()) {
                opts.branch_before = req.payload.Get("branch_before", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("branch_after", Upp::Value()).IsString()) {
                opts.branch_after = req.payload.Get("branch_after", Upp::Value()).ToString().ToStd();
            }

            // Extract node parameters from payload
            if (req.payload.Get("node_id", Upp::Value()).IsString()) {
                opts.node_id = req.payload.Get("node_id", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("node_kind", Upp::Value()).IsString()) {
                opts.node_kind_hint = req.payload.Get("node_kind", Upp::Value()).ToString().ToStd();
            }
            if (req.payload.Get("max_depth", Upp::Value()).IsInt()) {
                opts.max_depth = std::to_string(req.payload.Get("max_depth", Upp::Value(4)).ToInt());
            }

            // Use the CommandDispatcher to handle the ir-diff-node-region command
            CommandDispatcher dispatcher(std::make_unique<JsonFilesystemSessionStore>(req.workspace));
            Upp::String response = dispatcher.RunIrDiffNodeRegion(opts);

            // Parse the response and convert to DaemonResponse
            Upp::ValueMap parsed_response = JsonIO::Deserialize(response);
            out_resp.id = req.id;
            out_resp.ok = parsed_response.Get("ok", Upp::Value(false)).ToBool();
            out_resp.command = req.command;
            out_resp.error_code = parsed_response.Get("error_code", Upp::Value()).ToString().ToStd();
            out_resp.error = parsed_response.Get("error", Upp::Value()).ToString().ToStd();
            out_resp.data = parsed_response.Get("data", Upp::ValueMap());

            return Result<void>::MakeOk();
        }
        else if (req.command == "designer-create-session") {
            return HandleDesignerCreateSession(req);
        }
        else if (req.command == "designer-set-focus") {
            return HandleDesignerSetFocus(req);
        }
        else if (req.command == "designer-get-context") {
            return HandleDesignerGetContext(req);
        }
        else if (req.command == "designer-analyze") {
            return HandleDesignerAnalyze(req);
        }
        else if (req.command == "designer-optimize") {
            return HandleDesignerOptimize(req);
        }
        else if (req.command == "designer-propose-refactors") {
            return HandleDesignerProposeRefactors(req);
        }
        else if (req.command == "designer-apply-refactors") {
            return HandleDesignerApplyRefactors(req);
        }
        else if (req.command == "designer-diff") {
            return HandleDesignerDiff(req);
        }
        else if (req.command == "designer-codegen") {
            return HandleDesignerCodegen(req);
        }
        else if (req.command == "designer-run-playbook") {
            return HandleDesignerRunPlaybook(req);
        }
        else {
            return Result<void>::MakeError(ErrorCode::CommandParseError, "Unknown command: " + req.command);
        }
        
        if (result.ok) {
            out_resp = result.data;
        } else {
            // Create error response
            out_resp.id = req.id;
            out_resp.ok = false;
            out_resp.command = req.command;
            out_resp.error_code = JsonIO::ErrorCodeToString(result.error_code);
            out_resp.error = result.error_message;
            out_resp.data = Upp::ValueMap();
        }
        
        return Result<void>::MakeOk();
    }
    catch (const std::exception& e) {
        out_resp.id = req.id;
        out_resp.ok = false;
        out_resp.command = req.command;
        out_resp.error_code = "INTERNAL_ERROR";
        out_resp.error = std::string("Exception handling request: ") + e.what();
        out_resp.data = Upp::ValueMap();
        
        return Result<void>::MakeOk();
    }
}

Result<DaemonResponse> SessionServer::ProcessRequestFromJson(const std::string& json_str) {
    // This is a simplified implementation - in reality we'd need a proper JSON parser
    // For now, we'll simulate parsing based on the expected structure
    
    DaemonRequest req;
    Upp::String json_content(json_str.c_str());
    Upp::ValueMap parsed = JsonIO::Deserialize(json_content);
    
    // Extract fields from the parsed JSON
    req.id = parsed.Get("id", Upp::String("")).ToStd();
    req.command = parsed.Get("command", Upp::String("")).ToStd();
    req.workspace = parsed.Get("workspace", Upp::String("")).ToStd();
    req.session_id = parsed.Get("session_id", -1);
    req.user_id = parsed.Get("user_id", Upp::String("anonymous")).ToStd();
    req.payload = parsed.Get("payload", Upp::ValueMap());
    
    DaemonResponse resp;
    auto result = HandleRequest(req, resp);
    
    if (result.ok) {
        return Result<DaemonResponse>::MakeOk(resp);
    } else {
        // Create error response
        DaemonResponse error_resp;
        error_resp.id = req.id;
        error_resp.ok = false;
        error_resp.command = req.command;
        error_resp.error_code = JsonIO::ErrorCodeToString(result.error_code);
        error_resp.error = result.error_message;
        error_resp.data = Upp::ValueMap();
        
        return Result<DaemonResponse>::MakeOk(error_resp);
    }
}

Result<DaemonResponse> SessionServer::HandleInitWorkspace(const DaemonRequest& req) {
    // Create a command options structure to reuse existing CLI functionality
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.user_id = req.user_id;

    // Create a new session store for this operation using the factory function
    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunInitWorkspace(opts);

    // Parse the result to get proper response structure
    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleCreateSession(const DaemonRequest& req) {
    // Extract circuit file from payload
    std::string circuit_file = req.payload.Get("circuit_file", Upp::String("")).ToStd();

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.circuit_file = circuit_file;
    opts.user_id = req.user_id;

    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunCreateSession(opts);

    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleListSessions(const DaemonRequest& req) {
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.user_id = req.user_id;

    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunListSessions(opts);

    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleRunTicks(const DaemonRequest& req) {
    int ticks = req.payload.Get("ticks", 1);
    
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.ticks = ticks;
    opts.user_id = req.user_id;
    
    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));
    
    Upp::String result = dispatcher.RunRunTicks(opts);
    
    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);
    
    // If successful, broadcast a session update event
    if (parsed_result.Get("ok", false)) {
        Upp::ValueMap data_map = parsed_result.Get("data", Upp::ValueMap());
        int session_id = data_map.Get("session_id", -1);
        int total_ticks = data_map.Get("total_ticks", 0);
        // For now, we'll broadcast after successful run-ticks
        BroadcastSessionUpdate(session_id, req.workspace, total_ticks, total_ticks);
    }
    
    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleGetState(const DaemonRequest& req) {
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;
    
    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));
    
    Upp::String result = dispatcher.RunGetState(opts);
    
    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);
    
    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleExportNetlist(const DaemonRequest& req) {
    int pcb_id = req.payload.Get("pcb_id", 0);
    
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.pcb_id = pcb_id;
    opts.user_id = req.user_id;
    
    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));
    
    Upp::String result = dispatcher.RunExportNetlist(opts);
    
    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);
    
    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleDestroySession(const DaemonRequest& req) {
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;
    
    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));
    
    Upp::String result = dispatcher.RunDestroySession(opts);
    
    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);
    
    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleLintCircuit(const DaemonRequest& req) {
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;
    
    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));
    
    Upp::String result = dispatcher.RunLintCircuit(opts);
    
    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);
    
    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleAnalyzeCircuit(const DaemonRequest& req) {
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;
    
    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));
    
    Upp::String result = dispatcher.RunAnalyzeCircuit(opts);
    
    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);
    
    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleEditAddComponent(const DaemonRequest& req) {
    std::string component_type = req.payload.Get("type", Upp::String("")).ToStd();
    std::string component_name = req.payload.Get("name", Upp::String("")).ToStd();
    int x = req.payload.Get("x", 0);
    int y = req.payload.Get("y", 0);

    // Get collaboration parameters
    int64_t expected_revision = req.payload.Get("expected_revision", -1);
    bool allow_merge = req.payload.Get("allow_merge", true);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.circuit_file = component_type;  // Store component type in circuit_file field
    opts.netlist_file = component_name;  // Store component name in netlist_file field
    opts.ticks = x;      // Store x position in ticks field
    opts.pcb_id = y;     // Store y position in pcb_id field
    opts.user_id = req.user_id;

    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunEditAddComponent(opts);

    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    // If successful, broadcast a session update event
    if (parsed_result.Get("ok", false)) {
        auto data_map = parsed_result.Get("data", Upp::ValueMap());
        int session_id = data_map.Get("session_id", req.session_id);
        int circuit_revision = data_map.Get("circuit_revision", 0);
        int sim_revision = data_map.Get("sim_revision", 0);

        // Add merge information to the response
        if (expected_revision > 0 && expected_revision != circuit_revision - 1) {
            Upp::ValueMap merge_info;
            merge_info.Add("merged", true);
            merge_info.Add("conflict", false);
            merge_info.Add("reason", Upp::String(""));
            data_map.Set("merge", merge_info);
            parsed_result.Set("data", data_map);
        }

        BroadcastSessionUpdate(session_id, req.workspace, circuit_revision, sim_revision);
    }

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleEditRemoveComponent(const DaemonRequest& req) {
    std::string component_id = req.payload.Get("component_id", Upp::String("")).ToStd();

    // Get collaboration parameters
    int64_t expected_revision = req.payload.Get("expected_revision", -1);
    bool allow_merge = req.payload.Get("allow_merge", true);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.circuit_file = component_id;  // Store component ID in circuit_file field
    opts.user_id = req.user_id;

    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunEditRemoveComponent(opts);

    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    // If successful, broadcast a session update event
    if (parsed_result.Get("ok", false)) {
        auto data_map = parsed_result.Get("data", Upp::ValueMap());
        int session_id = data_map.Get("session_id", req.session_id);
        int circuit_revision = data_map.Get("circuit_revision", 0);
        int sim_revision = data_map.Get("sim_revision", 0);

        // Add merge information to the response
        if (expected_revision > 0 && expected_revision != circuit_revision - 1) {
            Upp::ValueMap merge_info;
            merge_info.Add("merged", true);
            merge_info.Add("conflict", false);
            merge_info.Add("reason", Upp::String(""));
            data_map.Set("merge", merge_info);
            parsed_result.Set("data", data_map);
        }

        BroadcastSessionUpdate(session_id, req.workspace, circuit_revision, sim_revision);
    }

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleEditMoveComponent(const DaemonRequest& req) {
    std::string component_id = req.payload.Get("component_id", Upp::String("")).ToStd();
    int x = req.payload.Get("x", 0);
    int y = req.payload.Get("y", 0);

    // Get collaboration parameters
    int64_t expected_revision = req.payload.Get("expected_revision", -1);
    bool allow_merge = req.payload.Get("allow_merge", true);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.circuit_file = component_id;  // Store component ID in circuit_file field
    opts.ticks = x;      // Store x position in ticks field
    opts.pcb_id = y;     // Store y position in pcb_id field
    opts.user_id = req.user_id;

    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunEditMoveComponent(opts);

    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    // If successful, broadcast a session update event
    if (parsed_result.Get("ok", false)) {
        auto data_map = parsed_result.Get("data", Upp::ValueMap());
        int session_id = data_map.Get("session_id", req.session_id);
        int circuit_revision = data_map.Get("circuit_revision", 0);
        int sim_revision = data_map.Get("sim_revision", 0);

        // Add merge information to the response
        if (expected_revision > 0 && expected_revision != circuit_revision - 1) {
            Upp::ValueMap merge_info;
            merge_info.Add("merged", true);
            merge_info.Add("conflict", false);
            merge_info.Add("reason", Upp::String(""));
            data_map.Set("merge", merge_info);
            parsed_result.Set("data", data_map);
        }

        BroadcastSessionUpdate(session_id, req.workspace, circuit_revision, sim_revision);
    }

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleEditSetComponentProperty(const DaemonRequest& req) {
    std::string component_id = req.payload.Get("component_id", Upp::String("")).ToStd();
    std::string property_name = req.payload.Get("property_name", Upp::String("")).ToStd();
    std::string property_value = req.payload.Get("property_value", Upp::String("")).ToStd();

    // Get collaboration parameters
    int64_t expected_revision = req.payload.Get("expected_revision", -1);
    bool allow_merge = req.payload.Get("allow_merge", true);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.circuit_file = component_id;  // Store component ID in circuit_file field
    opts.netlist_file = property_name; // Store property name in netlist_file field
    opts.ticks = std::stoi(property_value.empty() ? "0" : property_value); // Store property value in ticks field
    opts.user_id = req.user_id;

    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunEditSetComponentProperty(opts);

    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    // If successful, broadcast a session update event
    if (parsed_result.Get("ok", false)) {
        auto data_map = parsed_result.Get("data", Upp::ValueMap());
        int session_id = data_map.Get("session_id", req.session_id);
        int circuit_revision = data_map.Get("circuit_revision", 0);
        int sim_revision = data_map.Get("sim_revision", 0);

        // Add merge information to the response
        if (expected_revision > 0 && expected_revision != circuit_revision - 1) {
            Upp::ValueMap merge_info;
            merge_info.Add("merged", true);
            merge_info.Add("conflict", false);
            merge_info.Add("reason", Upp::String(""));
            data_map.Set("merge", merge_info);
            parsed_result.Set("data", data_map);
        }

        BroadcastSessionUpdate(session_id, req.workspace, circuit_revision, sim_revision);
    }

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleEditConnect(const DaemonRequest& req) {
    std::string start_component_id = req.payload.Get("start_component_id", Upp::String("")).ToStd();
    std::string start_pin_name = req.payload.Get("start_pin_name", Upp::String("")).ToStd();
    std::string end_component_id = req.payload.Get("end_component_id", Upp::String("")).ToStd();
    std::string end_pin_name = req.payload.Get("end_pin_name", Upp::String("")).ToStd();

    // Get collaboration parameters
    int64_t expected_revision = req.payload.Get("expected_revision", -1);
    bool allow_merge = req.payload.Get("allow_merge", true);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.circuit_file = start_component_id;  // Store start component ID in circuit_file field
    opts.netlist_file = start_pin_name;      // Store start pin name in netlist_file field
    opts.ticks = std::stoi(end_component_id.empty() ? "0" : end_component_id);  // Store end component ID in ticks field
    opts.pcb_id = std::stoi(end_pin_name.empty() ? "0" : end_pin_name);         // Store end pin name in pcb_id field
    opts.user_id = req.user_id;

    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunEditConnect(opts);

    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    // If successful, broadcast a session update event
    if (parsed_result.Get("ok", false)) {
        auto data_map = parsed_result.Get("data", Upp::ValueMap());
        int session_id = data_map.Get("session_id", req.session_id);
        int circuit_revision = data_map.Get("circuit_revision", 0);
        int sim_revision = data_map.Get("sim_revision", 0);

        // Add merge information to the response
        if (expected_revision > 0 && expected_revision != circuit_revision - 1) {
            Upp::ValueMap merge_info;
            merge_info.Add("merged", true);
            merge_info.Add("conflict", false);
            merge_info.Add("reason", Upp::String(""));
            data_map.Set("merge", merge_info);
            parsed_result.Set("data", data_map);
        }

        BroadcastSessionUpdate(session_id, req.workspace, circuit_revision, sim_revision);
    }

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleEditDisconnect(const DaemonRequest& req) {
    std::string start_component_id = req.payload.Get("start_component_id", Upp::String("")).ToStd();
    std::string start_pin_name = req.payload.Get("start_pin_name", Upp::String("")).ToStd();
    std::string end_component_id = req.payload.Get("end_component_id", Upp::String("")).ToStd();
    std::string end_pin_name = req.payload.Get("end_pin_name", Upp::String("")).ToStd();

    // Get collaboration parameters
    int64_t expected_revision = req.payload.Get("expected_revision", -1);
    bool allow_merge = req.payload.Get("allow_merge", true);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.circuit_file = start_component_id;  // Store start component ID in circuit_file field
    opts.netlist_file = start_pin_name;      // Store start pin name in netlist_file field
    opts.ticks = std::stoi(end_component_id.empty() ? "0" : end_component_id);  // Store end component ID in ticks field
    opts.pcb_id = std::stoi(end_pin_name.empty() ? "0" : end_pin_name);         // Store end pin name in pcb_id field
    opts.user_id = req.user_id;

    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));

    Upp::String result = dispatcher.RunEditDisconnect(opts);

    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);

    // If successful, broadcast a session update event
    if (parsed_result.Get("ok", false)) {
        auto data_map = parsed_result.Get("data", Upp::ValueMap());
        int session_id = data_map.Get("session_id", req.session_id);
        int circuit_revision = data_map.Get("circuit_revision", 0);
        int sim_revision = data_map.Get("sim_revision", 0);

        // Add merge information to the response
        if (expected_revision > 0 && expected_revision != circuit_revision - 1) {
            Upp::ValueMap merge_info;
            merge_info.Add("merged", true);
            merge_info.Add("conflict", false);
            merge_info.Add("reason", Upp::String(""));
            data_map.Set("merge", merge_info);
            parsed_result.Set("data", data_map);
        }

        BroadcastSessionUpdate(session_id, req.workspace, circuit_revision, sim_revision);
    }

    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleEditGetCircuit(const DaemonRequest& req) {
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;
    
    auto session_store = CreateFilesystemSessionStore(opts.workspace);
    CommandDispatcher dispatcher(std::move(session_store));
    
    Upp::String result = dispatcher.RunEditGetCircuit(opts);
    
    Upp::ValueMap parsed_result = JsonIO::Deserialize(result);
    
    return CreateSuccessResponse(req, parsed_result);
}

Result<DaemonResponse> SessionServer::HandleCircuitDiff(const DaemonRequest& req) {
    int64_t from_revision = req.payload.Get("from_revision", 0);
    int64_t to_revision = req.payload.Get("to_revision", 0);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;

    // In a real implementation, we would calculate the differences between circuit states
    // at from_revision and to_revision, but for now we'll return an empty diff
    Upp::ValueMap data_map;
    Upp::ValueArray diff_ops;
    data_map.Add("diff", diff_ops);
    data_map.Add("from_revision", from_revision);
    data_map.Add("to_revision", to_revision);

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleCircuitPatch(const DaemonRequest& req) {
    // Get the diff operations from the payload
    auto diff_array = req.payload.Get("diff", Upp::ValueArray());
    int64_t expected_revision = req.payload.Get("expected_revision", -1);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;

    // In a real implementation, we would apply the diff to the circuit
    // but for now we'll just return success
    Upp::ValueMap data_map;
    data_map.Add("applied", true);
    data_map.Add("new_revision", expected_revision + 1);

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleCircuitReplay(const DaemonRequest& req) {
    int64_t revision = req.payload.Get("revision", 0);

    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;

    // In a real implementation, we would return the circuit state at the specific revision
    // but for now we'll return a minimal response
    Upp::ValueMap data_map;
    data_map.Add("revision", revision);
    data_map.Add("circuit_state", Upp::String("{}"));  // Empty circuit state for now

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleCircuitHistory(const DaemonRequest& req) {
    CommandOptions opts;
    opts.workspace = req.workspace;
    opts.session_id = req.session_id;
    opts.user_id = req.user_id;

    // In a real implementation, we would return metadata for all revisions
    // but for now we'll return an empty history
    Upp::ValueMap data_map;
    Upp::ValueArray history;
    data_map.Add("history", history);

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::CreateSuccessResponse(const DaemonRequest& req, const Upp::ValueMap& data) {
    DaemonResponse resp;
    resp.id = req.id;
    resp.ok = true;
    resp.command = req.command;
    resp.error_code = "";
    resp.error = "";
    resp.data = data;
    
    return Result<DaemonResponse>::MakeOk(resp);
}

Result<DaemonResponse> SessionServer::CreateErrorResponse(const DaemonRequest& req, const std::string& error_msg, const std::string& error_code) {
    DaemonResponse resp;
    resp.id = req.id;
    resp.ok = false;
    resp.command = req.command;
    resp.error_code = error_code.empty() ? "INTERNAL_ERROR" : error_code;
    resp.error = error_msg;
    resp.data = Upp::ValueMap();
    
    return Result<DaemonResponse>::MakeOk(resp);
}

void SessionServer::BroadcastSessionUpdate(int session_id, const std::string& workspace, int circuit_revision, int sim_revision) {
    // Create and output the broadcast event
    Upp::ValueMap event;
    event.Add("event", Upp::String("session-updated"));
    event.Add("workspace", Upp::String(workspace.c_str()));
    event.Add("session_id", session_id);
    event.Add("circuit_revision", circuit_revision);
    event.Add("sim_revision", sim_revision);
    
    // Output to stdout as a JSON line
    std::cout << JsonIO::ValueMapToJson(event).ToStd() << std::endl;
}

Result<void> SessionServer::ProcessRequests() {
    std::string line;
    
    // Read from stdin until EOF
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }
        
        // Process the request
        auto result = ProcessRequestFromJson(line);
        
        if (result.ok) {
            // Output the response
            std::cout << JsonIO::ValueMapToJson(result.data) << std::endl;
        } else {
            // Output error response
            DaemonResponse error_resp;
            error_resp.id = "unknown";
            error_resp.ok = false;
            error_resp.command = "unknown";
            error_resp.error_code = JsonIO::ErrorCodeToString(result.error_code);
            error_resp.error = result.error_message;
            error_resp.data = Upp::ValueMap();
            
            std::cout << JsonIO::ValueMapToJson(error_resp) << std::endl;
        }
    }
    
    return Result<void>::MakeOk();
}

void SessionServer::BroadcastCircuitMerged(int session_id, const std::string& workspace, int revision, const std::vector<EditOperation>& ops) {
    // Create and output the broadcast event for a merged circuit operation
    Upp::ValueMap event;
    event.Add("event", Upp::String("circuit-merged"));
    event.Add("workspace", Upp::String(workspace.c_str()));
    event.Add("session_id", session_id);
    event.Add("revision", revision);

    // Convert operations to JSON array
    Upp::ValueArray ops_array;
    for (const auto& op : ops) {
        Upp::ValueMap op_map;
        // This would need to serialize the operation properly - simplified for now
        op_map.Add("type", static_cast<int>(op.type));
        if (op.component_id.IsValid()) {
            op_map.Add("component_id", Upp::String(op.component_id.id.c_str()));
        }
        ops_array.Add(op_map);
    }

    event.Add("merged_ops", ops_array);

    // Output to stdout as a JSON line
    std::cout << JsonIO::ValueMapToJson(event).ToStd() << std::endl;
}

Result<DaemonResponse> SessionServer::HandleDesignerCreateSession(const DaemonRequest& req) {
    // Extract the required parameters from the payload
    int proto_session_id = req.payload.Get("proto_session_id", -1);
    std::string branch = req.payload.Get("branch", Upp::String("main")).ToStd();

    // Validate required parameters
    if (proto_session_id < 0) {
        return CreateErrorResponse(req, "proto_session_id is required", "INVALID_PARAMETER");
    }

    // Call the co-designer manager to create a session
    auto result = co_designer_manager_->CreateSession(proto_session_id, branch);

    if (!result.ok) {
        return CreateErrorResponse(req, result.error_message, JsonIO::ErrorCodeToString(result.error_code));
    }

    // Create response data
    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(result.data));

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerSetFocus(const DaemonRequest& req) {
    // Extract parameters
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();
    std::string block_id = req.payload.Get("block_id", Upp::String("")).ToStd();
    std::string node_id = req.payload.Get("node_id", Upp::String("")).ToStd();
    std::string node_kind = req.payload.Get("node_kind", Upp::String("")).ToStd();
    bool use_optimized_ir = req.payload.Get("use_optimized_ir", false);

    // Validate required parameter
    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the current session
    auto get_result = co_designer_manager_->GetSession(designer_session_id);
    if (!get_result.ok) {
        return CreateErrorResponse(req, get_result.error_message, JsonIO::ErrorCodeToString(get_result.error_code));
    }

    // Update the session with new focus
    auto session = get_result.data;
    if (!block_id.empty()) {
        session.current_block_id = block_id;
    }
    if (!node_id.empty()) {
        session.current_node_id = node_id;
    }
    if (!node_kind.empty()) {
        session.current_node_kind = node_kind;
    }
    session.use_optimized_ir = use_optimized_ir;

    auto update_result = co_designer_manager_->UpdateSession(session);
    if (!update_result.ok) {
        return CreateErrorResponse(req, update_result.error_message, JsonIO::ErrorCodeToString(update_result.error_code));
    }

    // Create response data
    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(session));

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerGetContext(const DaemonRequest& req) {
    // Extract parameter
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();

    // Validate required parameter
    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the session
    auto result = co_designer_manager_->GetSession(designer_session_id);
    if (!result.ok) {
        return CreateErrorResponse(req, result.error_message, JsonIO::ErrorCodeToString(result.error_code));
    }

    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(result.data));

    // For now, add empty optional fields - in a real implementation we would fetch actual data
    // using the circuit facade based on the current block_id/node_id

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerAnalyze(const DaemonRequest& req) {
    // Extract parameters
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();
    bool include_behavior = req.payload.Get("include_behavior", true);
    bool include_ir = req.payload.Get("include_ir", true);
    bool include_graph_stats = req.payload.Get("include_graph_stats", false);
    bool include_timing = req.payload.Get("include_timing", false);

    // Validate required parameter
    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the session
    auto session_result = co_designer_manager_->GetSession(designer_session_id);
    if (!session_result.ok) {
        return CreateErrorResponse(req, session_result.error_message, JsonIO::ErrorCodeToString(session_result.error_code));
    }

    CoDesignerSessionState session = session_result.data;

    // Create response data
    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(session));

    // Add block analysis if block_id is set and requested
    if (!session.current_block_id.empty()) {
        Upp::ValueMap block_map;
        block_map.Add("block_id", Upp::String(session.current_block_id.c_str()));

        // Add behavior descriptor
        if (include_behavior) {
            // Create a dummy behavior descriptor for now
            BehaviorDescriptor behavior;
            behavior.subject_id = session.current_block_id;
            behavior.subject_kind = "Block";
            behavior.behavior_kind = BehaviorKind::Unknown;
            behavior.description = "Block behavior information";
            block_map.Add("behavior", JsonIO::BehaviorDescriptorToValueMap(behavior));
        }

        // Add IR module
        if (include_ir) {
            // Create a dummy IR module for now
            IrModule ir;
            ir.id = session.current_block_id;
            block_map.Add("ir", JsonIO::IrModuleToValueMap(ir));
        }

        data_map.Add("block", block_map);
    }

    // Add node analysis if node_id is set and requested
    if (!session.current_node_id.empty()) {
        Upp::ValueMap node_map;
        node_map.Add("node_id", Upp::String(session.current_node_id.c_str()));

        // Add node behavior descriptor
        if (include_behavior) {
            BehaviorDescriptor behavior;
            behavior.subject_id = session.current_node_id;
            behavior.subject_kind = session.current_node_kind.empty() ? "Node" : session.current_node_kind;
            behavior.behavior_kind = BehaviorKind::Unknown;
            behavior.description = "Node behavior information";
            node_map.Add("behavior", JsonIO::BehaviorDescriptorToValueMap(behavior));
        }

        // Add node IR
        if (include_ir) {
            IrModule ir;
            ir.id = session.current_node_id + "_region";
            node_map.Add("ir", JsonIO::IrModuleToValueMap(ir));
        }

        data_map.Add("node", node_map);
    }

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerOptimize(const DaemonRequest& req) {
    // Extract parameters
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();
    std::string target = req.payload.Get("target", Upp::String("block")).ToStd();
    Upp::ValueArray passes_array = req.payload.Get("passes", Upp::ValueArray());

    // Validate required parameter
    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the session
    auto session_result = co_designer_manager_->GetSession(designer_session_id);
    if (!session_result.ok) {
        return CreateErrorResponse(req, session_result.error_message, JsonIO::ErrorCodeToString(session_result.error_code));
    }

    CoDesignerSessionState session = session_result.data;

    // Convert passes array to vector of IrOptPassKind
    std::vector<IrOptPassKind> passes;
    for (int i = 0; i < passes_array.GetCount(); ++i) {
        std::string pass_str = passes_array[i].ToString().ToStd();
        IrOptPassKind pass_kind = IrOptPassKind::SimplifyAlgebraic; // Default value

        if (pass_str == "SimplifyAlgebraic") {
            pass_kind = IrOptPassKind::SimplifyAlgebraic;
        } else if (pass_str == "FoldConstants") {
            pass_kind = IrOptPassKind::FoldConstants;
        } else if (pass_str == "SimplifyMux") {
            pass_kind = IrOptPassKind::SimplifyMux;
        } else if (pass_str == "EliminateTrivialLogic") {
            pass_kind = IrOptPassKind::EliminateTrivialLogic;
        }

        passes.push_back(pass_kind);
    }

    // Create response data
    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(session));

    // Create optimization result
    Upp::ValueMap optimization_map;

    // Create dummy original and optimized IR modules
    IrModule original;
    original.id = target == "block" ? session.current_block_id : session.current_node_id;
    IrModule optimized = original;  // For now, same as original

    optimization_map.Add("original", JsonIO::IrModuleToValueMap(original));
    optimization_map.Add("optimized", JsonIO::IrModuleToValueMap(optimized));

    // Create dummy summaries
    Upp::ValueArray summaries_array;
    for (const auto& pass : passes) {
        IrOptChangeSummary summary;
        summary.pass_kind = pass;
        summary.expr_changes = 0;  // For now
        summary.reg_changes = 0;   // For now
        summary.behavior_preserved = true;
        summaries_array.Add(JsonIO::IrOptChangeSummaryToValueMap(summary));
    }
    optimization_map.Add("summaries", summaries_array);

    data_map.Add("optimization", optimization_map);

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerRunPlaybook(const DaemonRequest& req) {
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();

    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the CoDesigner session
    auto get_session_result = co_designer_manager_->GetSession(designer_session_id);
    if (!get_session_result.ok) {
        return CreateErrorResponse(req, "Invalid designer session: " + get_session_result.error_message,
                                   JsonIO::ErrorCodeToString(get_session_result.error_code));
    }

    CoDesignerSessionState session = get_session_result.data;

    // Parse the playbook config from the payload
    Upp::String playbook_kind_str = req.payload.Get("playbook_kind", Upp::String("OptimizeBlockAndReport"));
    std::string playbook_kind_std = playbook_kind_str.ToStd();

    PlaybookKind kind = PlaybookKind::OptimizeBlockAndReport; // Default
    if (playbook_kind_std == "OptimizeAndApplySafeRefactors") {
        kind = PlaybookKind::OptimizeAndApplySafeRefactors;
    } else if (playbook_kind_std == "SystemOptimizeAndReport") {
        kind = PlaybookKind::SystemOptimizeAndReport;
    } else if (playbook_kind_std == "SystemOptimizeAndApplySafeRefactors") {
        kind = PlaybookKind::SystemOptimizeAndApplySafeRefactors;
    }

    std::string target = req.payload.Get("target", Upp::String("block")).ToStd();
    std::string block_id = req.payload.Get("block_id", Upp::String("")).ToStd();
    std::string baseline_branch = req.payload.Get("baseline_branch", Upp::String("main")).ToStd();
    bool use_optimized_ir = req.payload.Get("use_optimized_ir", false);
    bool apply_refactors = req.payload.Get("apply_refactors", false);

    // Parse the block_ids array for system-level playbooks
    Upp::ValueArray block_ids_array = req.payload.Get("block_ids", Upp::ValueArray());
    std::vector<std::string> block_ids;
    for (int i = 0; i < block_ids_array.GetCount(); i++) {
        block_ids.push_back(block_ids_array[i].ToString().ToStd());
    }

    // Parse name_prefix for system-level playbooks
    std::string name_prefix = req.payload.Get("name_prefix", Upp::String("")).ToStd();

    // Parse the passes array
    Upp::ValueArray passes_array = req.payload.Get("passes", Upp::ValueArray());
    std::vector<IrOptPassKind> passes;
    for (int i = 0; i < passes_array.GetCount(); i++) {
        std::string pass_str = passes_array[i].ToString().ToStd();
        if (pass_str == "SimplifyAlgebraic") {
            passes.push_back(IrOptPassKind::SimplifyAlgebraic);
        } else if (pass_str == "FoldConstants") {
            passes.push_back(IrOptPassKind::FoldConstants);
        } else if (pass_str == "SimplifyMux") {
            passes.push_back(IrOptPassKind::SimplifyMux);
        } else if (pass_str == "EliminateTrivialLogic") {
            passes.push_back(IrOptPassKind::EliminateTrivialLogic);
        }
    }

    // Create the playbook config
    PlaybookConfig config;
    config.kind = kind;
    config.designer_session_id = designer_session_id;
    config.target = target;
    config.block_id = block_id;
    config.block_ids = block_ids;
    config.name_prefix = name_prefix;
    config.baseline_branch = baseline_branch;
    config.passes = passes;
    config.use_optimized_ir = use_optimized_ir;
    config.apply_refactors = apply_refactors;

    // Get the session store for the workspace
    auto session_store = std::make_shared<JsonFilesystemSessionStore>();

    // Run the playbook
    auto playbook_result = PlaybookEngine::RunPlaybook(config, *co_designer_manager_, *session_store, req.workspace);

    if (!playbook_result.ok) {
        return CreateErrorResponse(req, "Failed to run playbook: " + playbook_result.error_message,
                                   JsonIO::ErrorCodeToString(playbook_result.error_code));
    }

    // Create the response
    Upp::ValueMap data_map;
    data_map.Add("playbook_result", JsonIO::PlaybookResultToValueMap(playbook_result.data));

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerProposeRefactors(const DaemonRequest& req) {
    // Extract parameters
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();
    std::string target = req.payload.Get("target", Upp::String("block")).ToStd();
    Upp::ValueArray passes_array = req.payload.Get("passes", Upp::ValueArray());

    // Validate required parameter
    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the session
    auto session_result = co_designer_manager_->GetSession(designer_session_id);
    if (!session_result.ok) {
        return CreateErrorResponse(req, session_result.error_message, JsonIO::ErrorCodeToString(session_result.error_code));
    }

    CoDesignerSessionState session = session_result.data;

    // Convert passes array to vector of IrOptPassKind
    std::vector<IrOptPassKind> passes;
    for (int i = 0; i < passes_array.GetCount(); ++i) {
        std::string pass_str = passes_array[i].ToString().ToStd();
        IrOptPassKind pass_kind = IrOptPassKind::SimplifyAlgebraic; // Default value

        if (pass_str == "SimplifyAlgebraic") {
            pass_kind = IrOptPassKind::SimplifyAlgebraic;
        } else if (pass_str == "FoldConstants") {
            pass_kind = IrOptPassKind::FoldConstants;
        } else if (pass_str == "SimplifyMux") {
            pass_kind = IrOptPassKind::SimplifyMux;
        } else if (pass_str == "EliminateTrivialLogic") {
            pass_kind = IrOptPassKind::EliminateTrivialLogic;
        }

        passes.push_back(pass_kind);
    }

    // Create response data
    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(session));

    // Create dummy transformation plans
    Upp::ValueArray plans_array;

    // Add a dummy plan for each pass
    for (size_t i = 0; i < passes.size(); ++i) {
        TransformationPlan plan;
        plan.id = "IR_T" + std::to_string(i + 1);

        switch (passes[i]) {
            case IrOptPassKind::SimplifyAlgebraic:
                plan.kind = TransformationKind::SimplifyRedundantGate;
                break;
            case IrOptPassKind::FoldConstants:
                plan.kind = TransformationKind::SimplifyRedundantGate;
                break;
            case IrOptPassKind::SimplifyMux:
                plan.kind = TransformationKind::SimplifyRedundantGate;
                break;
            case IrOptPassKind::EliminateTrivialLogic:
                plan.kind = TransformationKind::SimplifyRedundantGate;
                break;
        }

        plan.target.subject_id = target == "block" ? session.current_block_id : session.current_node_id;
        plan.target.subject_kind = target == "block" ? "Block" : "Node";

        plan.guarantees.push_back(PreservationLevel::BehaviorKindPreserved);
        plan.guarantees.push_back(PreservationLevel::IOContractPreserved);

        TransformationStep step;
        step.description = "Apply " + passes_array[i].ToString().ToStd() + " optimization";
        plan.steps.push_back(step);

        plans_array.Add(JsonIO::TransformationPlanToValueMap(plan));
    }

    data_map.Add("plans", plans_array);

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerApplyRefactors(const DaemonRequest& req) {
    // Extract parameters
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();
    Upp::ValueArray plans_array = req.payload.Get("plans", Upp::ValueArray());
    std::string user_id = req.payload.Get("user_id", Upp::String("anonymous")).ToStd();
    bool allow_unverified = req.payload.Get("allow_unverified", false);

    // Validate required parameter
    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the session
    auto session_result = co_designer_manager_->GetSession(designer_session_id);
    if (!session_result.ok) {
        return CreateErrorResponse(req, session_result.error_message, JsonIO::ErrorCodeToString(session_result.error_code));
    }

    CoDesignerSessionState session = session_result.data;

    // Convert plans array to vector of TransformationPlan
    std::vector<TransformationPlan> plans;
    for (int i = 0; i < plans_array.GetCount(); ++i) {
        // For now, we'll create a dummy plan from the value map
        // In a real implementation, we would deserialize properly
        TransformationPlan plan;
        plan.id = "IR_T" + std::to_string(i + 1);
        plan.kind = TransformationKind::SimplifyRedundantGate;
        plan.target.subject_id = session.current_block_id.empty() ? session.current_node_id : session.current_block_id;
        plan.target.subject_kind = session.current_block_id.empty() ? "Node" : "Block";
        plan.guarantees.push_back(PreservationLevel::BehaviorKindPreserved);

        TransformationStep step;
        step.description = "Apply transformation " + std::to_string(i + 1);
        plan.steps.push_back(step);

        plans.push_back(plan);
    }

    // Create response data
    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(session));

    // Create applied plan IDs array
    Upp::ValueArray applied_plan_ids_array;
    for (const auto& plan : plans) {
        applied_plan_ids_array.Add(Upp::String(plan.id.c_str()));
    }
    data_map.Add("applied_plan_ids", applied_plan_ids_array);

    // Return a dummy new revision (in real implementation, this would come from actual application)
    data_map.Add("new_circuit_revision", 42);

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerDiff(const DaemonRequest& req) {
    // Extract parameters
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();
    std::string compare_branch = req.payload.Get("compare_branch", Upp::String("main")).ToStd();
    bool include_behavior_diff = req.payload.Get("include_behavior_diff", true);
    bool include_ir_diff = req.payload.Get("include_ir_diff", true);

    // Validate required parameter
    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the session
    auto session_result = co_designer_manager_->GetSession(designer_session_id);
    if (!session_result.ok) {
        return CreateErrorResponse(req, session_result.error_message, JsonIO::ErrorCodeToString(session_result.error_code));
    }

    CoDesignerSessionState session = session_result.data;

    // Create response data
    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(session));

    // Add behavior diff if requested and block_id is set
    if (include_behavior_diff && !session.current_block_id.empty()) {
        BehaviorDiff behavior_diff;
        behavior_diff.subject_id = session.current_block_id;
        behavior_diff.subject_kind = "Block";
        behavior_diff.change_kind = BehaviorChangeKind::None;

        // Fill in before and after behavior descriptors
        behavior_diff.before_behavior.subject_id = session.current_block_id;
        behavior_diff.before_behavior.subject_kind = "Block";
        behavior_diff.before_behavior.behavior_kind = BehaviorKind::Adder;
        behavior_diff.before_behavior.description = "Original block behavior";

        behavior_diff.after_behavior = behavior_diff.before_behavior;
        behavior_diff.after_behavior.description = "Modified block behavior";

        data_map.Add("behavior_diff", JsonIO::BehaviorDiffToValueMap(behavior_diff));
    }

    // Add IR diff if requested and block_id is set
    if (include_ir_diff && !session.current_block_id.empty()) {
        IrDiff ir_diff;
        ir_diff.module_id = session.current_block_id;
        ir_diff.change_kind = IrChangeKind::None;

        data_map.Add("ir_diff", JsonIO::IrDiffToValueMap(ir_diff));
    }

    return CreateSuccessResponse(req, data_map);
}

Result<DaemonResponse> SessionServer::HandleDesignerCodegen(const DaemonRequest& req) {
    // Extract parameters
    std::string designer_session_id = req.payload.Get("designer_session_id", Upp::String("")).ToStd();
    std::string target = req.payload.Get("target", Upp::String("block")).ToStd();
    std::string flavor = req.payload.Get("flavor", Upp::String("PseudoVerilog")).ToStd();
    bool use_optimized_ir = req.payload.Get("use_optimized_ir", true);

    // Validate required parameter
    if (designer_session_id.empty()) {
        return CreateErrorResponse(req, "designer_session_id is required", "INVALID_PARAMETER");
    }

    // Get the session
    auto session_result = co_designer_manager_->GetSession(designer_session_id);
    if (!session_result.ok) {
        return CreateErrorResponse(req, session_result.error_message, JsonIO::ErrorCodeToString(session_result.error_code));
    }

    CoDesignerSessionState session = session_result.data;

    // Create response data
    Upp::ValueMap data_map;
    data_map.Add("designer_session", JsonIO::CoDesignerSessionStateToValueMap(session));

    // Create codegen result
    Upp::ValueMap codegen_map;
    std::string id = target == "block" ? session.current_block_id : session.current_node_id;
    codegen_map.Add("id", Upp::String(id.c_str()));
    codegen_map.Add("name", Upp::String((id + "_" + target).c_str()));
    codegen_map.Add("flavor", Upp::String(flavor.c_str()));

    // Generate dummy code based on target and flavor
    std::string code;
    if (flavor == "PseudoVerilog" || flavor == "Verilog") {
        code = "// Generated " + flavor + " code for " + id + "\n";
        code += "module " + id + "();\n";
        code += "  // Implementation goes here\n";
        code += "endmodule\n";
    } else {
        code = "// Generated code for " + id + " in " + flavor + " format";
    }

    codegen_map.Add("code", Upp::String(code.c_str()));

    data_map.Add("codegen", codegen_map);

    return CreateSuccessResponse(req, data_map);
}

} // namespace ProtoVMCLI