#ifndef _ProtoVM_CommandDispatcher_h_
#define _ProtoVM_CommandDispatcher_h_

#include "SessionTypes.h"
#include "SessionStore.h"
#include "ProtoVM.h"
#include <memory>
#include <string>

namespace ProtoVMCLI {

class CommandDispatcher {
public:
    CommandDispatcher(std::unique_ptr<ISessionStore> store);
    
    Upp::String RunInitWorkspace(const CommandOptions& opts);
    Upp::String RunCreateSession(const CommandOptions& opts);
    Upp::String RunListSessions(const CommandOptions& opts);
    Upp::String RunRunTicks(const CommandOptions& opts);
    Upp::String RunGetState(const CommandOptions& opts);
    Upp::String RunExportNetlist(const CommandOptions& opts);
    Upp::String RunDestroySession(const CommandOptions& opts);

    // Streaming debug commands
    Upp::String RunDebugProcessLogs(int process_id);
    Upp::String RunDebugWebSocketStream(const Upp::String& ws_id);
    Upp::String RunDebugPollStream(const Upp::String& poll_id);

    // Circuit editing commands
    Upp::String RunEditAddComponent(const CommandOptions& opts);
    Upp::String RunEditRemoveComponent(const CommandOptions& opts);
    Upp::String RunEditMoveComponent(const CommandOptions& opts);
    Upp::String RunEditSetComponentProperty(const CommandOptions& opts);
    Upp::String RunEditConnect(const CommandOptions& opts);
    Upp::String RunEditDisconnect(const CommandOptions& opts);
    Upp::String RunEditGetCircuit(const CommandOptions& opts);

    // Circuit analysis and linting commands
    Upp::String RunLintCircuit(const CommandOptions& opts);
    Upp::String RunAnalyzeCircuit(const CommandOptions& opts);

    // Collaboration utility commands
    Upp::String RunCircuitDiff(const CommandOptions& opts);
    Upp::String RunCircuitPatch(const CommandOptions& opts);
    Upp::String RunCircuitReplay(const CommandOptions& opts);
    Upp::String RunCircuitHistory(const CommandOptions& opts);

    // Branch commands
    Upp::String RunBranchList(const CommandOptions& opts);
    Upp::String RunBranchCreate(const CommandOptions& opts);
    Upp::String RunBranchSwitch(const CommandOptions& opts);
    Upp::String RunBranchDelete(const CommandOptions& opts);
    Upp::String RunBranchMerge(const CommandOptions& opts);

    // Graph commands
    Upp::String RunGraphExport(const CommandOptions& opts);
    Upp::String RunGraphPaths(const CommandOptions& opts);
    Upp::String RunGraphFanIn(const CommandOptions& opts);
    Upp::String RunGraphFanOut(const CommandOptions& opts);
    Upp::String RunGraphStats(const CommandOptions& opts);

    // Timing analysis commands
    Upp::String RunTimingSummary(const CommandOptions& opts);
    Upp::String RunTimingCriticalPaths(const CommandOptions& opts);
    Upp::String RunTimingLoops(const CommandOptions& opts);
    Upp::String RunTimingHazards(const CommandOptions& opts);

    // Functional dependency analysis commands
    Upp::String RunDepsSummary(const CommandOptions& opts);
    Upp::String RunDepsBackward(const CommandOptions& opts);
    Upp::String RunDepsForward(const CommandOptions& opts);
    Upp::String RunDepsBoth(const CommandOptions& opts);

    // Block analysis commands
    Upp::String RunBlocksList(const CommandOptions& opts);
    Upp::String RunBlocksExport(const CommandOptions& opts);
    Upp::String RunBlockInspect(const CommandOptions& opts);

    // Behavioral analysis commands
    Upp::String RunBehaviorBlock(const CommandOptions& opts);
    Upp::String RunBehaviorNode(const CommandOptions& opts);

    // HLS IR commands
    Upp::String RunIrBlock(const CommandOptions& opts);
    Upp::String RunIrNodeRegion(const CommandOptions& opts);

    // Refactoring commands
    Upp::String RunRefactorSuggest(const CommandOptions& opts);
    Upp::String RunRefactorSuggestBlock(const CommandOptions& opts);
    Upp::String RunRefactorApply(const CommandOptions& opts);

    // IR optimization commands
    Upp::String RunIrOptBlock(const CommandOptions& opts);
    Upp::String RunIrOptRefactorBlock(const CommandOptions& opts);

    // Diff analysis commands
    Upp::String RunBehaviorDiffBlock(const CommandOptions& opts);
    Upp::String RunIrDiffBlock(const CommandOptions& opts);
    Upp::String RunIrDiffNodeRegion(const CommandOptions& opts);

    // CoDesigner commands
    Upp::String RunDesignerCreateSession(const CommandOptions& opts);
    Upp::String RunDesignerSetFocus(const CommandOptions& opts);
    Upp::String RunDesignerGetContext(const CommandOptions& opts);
    Upp::String RunDesignerAnalyze(const CommandOptions& opts);
    Upp::String RunDesignerOptimize(const CommandOptions& opts);
    Upp::String RunDesignerProposeRefactors(const CommandOptions& opts);
    Upp::String RunDesignerApplyRefactors(const CommandOptions& opts);
    Upp::String RunDesignerDiff(const CommandOptions& opts);
    Upp::String RunDesignerCodegen(const CommandOptions& opts);
    Upp::String RunDesignerRunPlaybook(const CommandOptions& opts);

private:
    std::unique_ptr<ISessionStore> session_store_;

    // Helper to validate workspace path
    bool ValidateWorkspace(const std::string& workspace_path);

};

} // namespace ProtoVMCLI

#endif