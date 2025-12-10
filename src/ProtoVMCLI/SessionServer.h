#ifndef _ProtoVM_SessionServer_h_
#define _ProtoVM_SessionServer_h_

#include "SessionTypes.h"
#include "SessionStore.h"
#include "CircuitFacade.h"
#include "EngineFacade.h"
#include "CircuitAnalysis.h"
#include "CircuitData.h"
#include "CircuitOps.h"
#include "CollaborationTypes.h"
#include "BranchOperations.h"
#include "IrOptimization.h"
#include "JsonFilesystemSessionStore.h"
#include "DiffAnalysis.h"
#include "CoDesigner.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>
#include <future>

namespace ProtoVMCLI {

struct InMemorySessionState {
    SessionMetadata metadata;
    CircuitData circuit;                 // or a wrapper (per current branch)
    std::unique_ptr<Machine> machine;    // via EngineFacade (per current branch)
    bool dirty;                          // indicates unsaved changes
    std::string current_branch;          // current branch in memory
    std::unordered_map<std::string, CircuitData> branch_circuits;  // circuit data per branch
    std::unordered_map<std::string, std::unique_ptr<Machine>> branch_machines;  // machines per branch
};

struct DaemonRequest {
    std::string id;
    std::string command;
    std::string workspace;
    int session_id = -1;
    std::string user_id;
    Upp::ValueMap payload;
};

struct DaemonResponse {
    std::string id;
    bool ok = false;
    std::string command;
    std::string error_code;
    std::string error;
    Upp::ValueMap data;
};

class SessionServer {
public:
    SessionServer();
    ~SessionServer();

    Result<void> HandleRequest(const DaemonRequest& req, DaemonResponse& out_resp);
    
    // Process requests from a stream (e.g., stdin)
    Result<void> ProcessRequests();
    
    // Process a single request from JSON string
    Result<DaemonResponse> ProcessRequestFromJson(const std::string& json_str);

private:
    std::unordered_map<std::string, std::unique_ptr<InMemorySessionState>> session_cache_;
    std::mutex cache_mutex_;
    std::shared_ptr<CoDesignerManager> co_designer_manager_;
    
    // Session management methods
    Result<InMemorySessionState*> GetOrLoadSession(int session_id, const std::string& workspace);
    Result<bool> SaveSessionToDisk(int session_id, const std::string& workspace, const InMemorySessionState& state);
    
    // Command handler methods
    Result<DaemonResponse> HandleInitWorkspace(const DaemonRequest& req);
    Result<DaemonResponse> HandleCreateSession(const DaemonRequest& req);
    Result<DaemonResponse> HandleListSessions(const DaemonRequest& req);
    Result<DaemonResponse> HandleRunTicks(const DaemonRequest& req);
    Result<DaemonResponse> HandleGetState(const DaemonRequest& req);
    Result<DaemonResponse> HandleExportNetlist(const DaemonRequest& req);
    Result<DaemonResponse> HandleDestroySession(const DaemonRequest& req);
    Result<DaemonResponse> HandleLintCircuit(const DaemonRequest& req);
    Result<DaemonResponse> HandleAnalyzeCircuit(const DaemonRequest& req);
    
    // Circuit edit handlers
    Result<DaemonResponse> HandleEditAddComponent(const DaemonRequest& req);
    Result<DaemonResponse> HandleEditRemoveComponent(const DaemonRequest& req);
    Result<DaemonResponse> HandleEditMoveComponent(const DaemonRequest& req);
    Result<DaemonResponse> HandleEditSetComponentProperty(const DaemonRequest& req);
    Result<DaemonResponse> HandleEditConnect(const DaemonRequest& req);
    Result<DaemonResponse> HandleEditDisconnect(const DaemonRequest& req);
    Result<DaemonResponse> HandleEditGetCircuit(const DaemonRequest& req);

    // Collaboration utility handlers
    Result<DaemonResponse> HandleCircuitDiff(const DaemonRequest& req);
    Result<DaemonResponse> HandleCircuitPatch(const DaemonRequest& req);
    Result<DaemonResponse> HandleCircuitReplay(const DaemonRequest& req);
    Result<DaemonResponse> HandleCircuitHistory(const DaemonRequest& req);

    // Timing analysis command handlers
    Result<DaemonResponse> HandleTimingSummary(const DaemonRequest& req);
    Result<DaemonResponse> HandleTimingCriticalPaths(const DaemonRequest& req);
    Result<DaemonResponse> HandleTimingLoops(const DaemonRequest& req);
    Result<DaemonResponse> HandleTimingHazards(const DaemonRequest& req);

    // Functional dependency analysis command handlers
    Result<DaemonResponse> HandleDepsSummary(const DaemonRequest& req);
    Result<DaemonResponse> HandleDepsBackward(const DaemonRequest& req);
    Result<DaemonResponse> HandleDepsForward(const DaemonRequest& req);
    Result<DaemonResponse> HandleDepsBoth(const DaemonRequest& req);

    // Block analysis command handlers
    Result<DaemonResponse> HandleBlocksList(const DaemonRequest& req);
    Result<DaemonResponse> HandleBlocksExport(const DaemonRequest& req);
    Result<DaemonResponse> HandleBlockInspect(const DaemonRequest& req);

    // HLS IR command handlers
    Result<DaemonResponse> HandleIrBlock(const DaemonRequest& req);
    Result<DaemonResponse> HandleIrNodeRegion(const DaemonRequest& req);

    // IR optimization command handlers
    Result<DaemonResponse> HandleIrOptBlock(const DaemonRequest& req);
    Result<DaemonResponse> HandleIrOptRefactorBlock(const DaemonRequest& req);

    // Refactoring command handlers
    Result<DaemonResponse> HandleRefactorSuggest(const DaemonRequest& req);
    Result<DaemonResponse> HandleRefactorSuggestBlock(const DaemonRequest& req);
    Result<DaemonResponse> HandleRefactorApply(const DaemonRequest& req);

    // CoDesigner command handlers
    Result<DaemonResponse> HandleDesignerCreateSession(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerSetFocus(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerGetContext(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerAnalyze(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerOptimize(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerProposeRefactors(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerApplyRefactors(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerDiff(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerCodegen(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerCodegenBlockC(const DaemonRequest& req);
    Result<DaemonResponse> HandleDesignerCodegenOscDemo(const DaemonRequest& req);

    // Utility methods
    Result<DaemonResponse> CreateSuccessResponse(const DaemonRequest& req, const Upp::ValueMap& data = Upp::ValueMap());
    Result<DaemonResponse> CreateErrorResponse(const DaemonRequest& req, const std::string& error_msg, const std::string& error_code = "");

    // Broadcast event methods
    void BroadcastSessionUpdate(int session_id, const std::string& workspace, int circuit_revision, int sim_revision);
    void BroadcastCircuitMerged(int session_id, const std::string& workspace, int revision, const std::vector<EditOperation>& ops);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_SessionServer_h_