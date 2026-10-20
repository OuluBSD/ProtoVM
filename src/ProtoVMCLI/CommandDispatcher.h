#ifndef _ProtoVM_CommandDispatcher_h_
#define _ProtoVM_CommandDispatcher_h_

#include "SessionTypes.h"
#include "SessionStore.h"
#include <ProtoVM/ProtoVM.h>
#include <memory>
#include <string>

namespace ProtoVMCLI {

class CommandDispatcher {
public:
    CommandDispatcher(std::unique_ptr<ISessionStore> store);
    
    Upp::String RunInitWorkspace(CommandOptions& opts);
    Upp::String RunCreateSession(CommandOptions& opts);
    Upp::String RunListSessions(CommandOptions& opts);
    Upp::String RunRunTicks(CommandOptions& opts);
    Upp::String RunGetState(CommandOptions& opts);
    Upp::String RunExportNetlist(CommandOptions& opts);
    Upp::String RunDestroySession(CommandOptions& opts);

    // Streaming debug commands
    Upp::String RunDebugProcessLogs(int process_id);
    Upp::String RunDebugWebSocketStream(const Upp::String& ws_id);
    Upp::String RunDebugPollStream(const Upp::String& poll_id);

    // Circuit editing commands
    Upp::String RunEditAddComponent(CommandOptions& opts);
    Upp::String RunEditRemoveComponent(CommandOptions& opts);
    Upp::String RunEditMoveComponent(CommandOptions& opts);
    Upp::String RunEditSetComponentProperty(CommandOptions& opts);
    Upp::String RunEditConnect(CommandOptions& opts);
    Upp::String RunEditDisconnect(CommandOptions& opts);
    Upp::String RunEditGetCircuit(CommandOptions& opts);

    // Circuit analysis and linting commands
    Upp::String RunLintCircuit(CommandOptions& opts);
    Upp::String RunAnalyzeCircuit(CommandOptions& opts);

    // Collaboration utility commands
    Upp::String RunCircuitDiff(CommandOptions& opts);
    Upp::String RunCircuitPatch(CommandOptions& opts);
    Upp::String RunCircuitReplay(CommandOptions& opts);
    Upp::String RunCircuitHistory(CommandOptions& opts);

    // Branch commands
    Upp::String RunBranchList(CommandOptions& opts);
    Upp::String RunBranchCreate(CommandOptions& opts);
    Upp::String RunBranchSwitch(CommandOptions& opts);
    Upp::String RunBranchDelete(CommandOptions& opts);
    Upp::String RunBranchMerge(CommandOptions& opts);

    // Graph commands
    Upp::String RunGraphExport(CommandOptions& opts);
    Upp::String RunGraphPaths(CommandOptions& opts);
    Upp::String RunGraphFanIn(CommandOptions& opts);
    Upp::String RunGraphFanOut(CommandOptions& opts);
    Upp::String RunGraphStats(CommandOptions& opts);

    // Timing analysis commands
    Upp::String RunTimingSummary(CommandOptions& opts);
    Upp::String RunTimingCriticalPaths(CommandOptions& opts);
    Upp::String RunTimingLoops(CommandOptions& opts);
    Upp::String RunTimingHazards(CommandOptions& opts);

    // Functional dependency analysis commands
    Upp::String RunDepsSummary(CommandOptions& opts);
    Upp::String RunDepsBackward(CommandOptions& opts);
    Upp::String RunDepsForward(CommandOptions& opts);
    Upp::String RunDepsBoth(CommandOptions& opts);

    // Block analysis commands
    Upp::String RunBlocksList(CommandOptions& opts);
    Upp::String RunBlocksExport(CommandOptions& opts);
    Upp::String RunBlockInspect(CommandOptions& opts);

    // Behavioral analysis commands
    Upp::String RunBehaviorBlock(CommandOptions& opts);
    Upp::String RunBehaviorNode(CommandOptions& opts);

    // HLS IR commands
    Upp::String RunIrBlock(CommandOptions& opts);
    Upp::String RunIrNodeRegion(CommandOptions& opts);

    // Scheduled IR commands
    Upp::String RunScheduleBlock(CommandOptions& opts);
    Upp::String RunScheduleNodeRegion(CommandOptions& opts);

    // Refactoring commands
    Upp::String RunRefactorSuggest(CommandOptions& opts);
    Upp::String RunRefactorSuggestBlock(CommandOptions& opts);
    Upp::String RunRefactorApply(CommandOptions& opts);

    // IR optimization commands
    Upp::String RunIrOptBlock(CommandOptions& opts);
    Upp::String RunIrOptRefactorBlock(CommandOptions& opts);

    // Diff analysis commands
    Upp::String RunBehaviorDiffBlock(CommandOptions& opts);
    Upp::String RunIrDiffBlock(CommandOptions& opts);
    Upp::String RunIrDiffNodeRegion(CommandOptions& opts);

    // Pipeline analysis commands
    Upp::String RunPipelineBlock(CommandOptions& opts);
    Upp::String RunPipelineSubsystem(CommandOptions& opts);

    // CDC analysis commands
    Upp::String RunCdcBlock(CommandOptions& opts);
    Upp::String RunCdcSubsystem(CommandOptions& opts);

    // Retiming analysis commands
    Upp::String RunRetimeBlock(CommandOptions& opts);
    Upp::String RunRetimeSubsystem(CommandOptions& opts);

    // Retiming application commands
    Upp::String RunRetimeBlockApply(CommandOptions& opts);
    Upp::String RunRetimeSubsystemApply(CommandOptions& opts);

    // Retiming optimization commands
    Upp::String RunRetimeOptBlock(CommandOptions& opts);
    Upp::String RunRetimeOptSubsystem(CommandOptions& opts);

    // Global pipeline analysis commands
    Upp::String RunGlobalPipelineSubsystem(CommandOptions& opts);
    Upp::String RunGlobalPipelineOptSubsystem(CommandOptions& opts);
    Upp::String RunGlobalPipelineApply(CommandOptions& opts);

    // Structural synthesis commands
    Upp::String RunStructAnalyzeBlock(CommandOptions& opts);
    Upp::String RunStructApplyBlock(CommandOptions& opts);

    // Codegen commands
    Upp::String RunCodegenBlockIr(CommandOptions& opts);
    Upp::String RunCodegenBlockC(CommandOptions& opts);
    Upp::String RunCodegenBlockOscDemo(CommandOptions& opts);
    Upp::String RunCodegenBlockCppClass(CommandOptions& opts);
    Upp::String RunCodegenBlockAudioDemo(CommandOptions& opts);

    // CoDesigner commands
    Upp::String RunDesignerCreateSession(CommandOptions& opts);
    Upp::String RunDesignerSetFocus(CommandOptions& opts);
    Upp::String RunDesignerGetContext(CommandOptions& opts);
    Upp::String RunDesignerAnalyze(CommandOptions& opts);
    Upp::String RunDesignerOptimize(CommandOptions& opts);
    Upp::String RunDesignerProposeRefactors(CommandOptions& opts);
    Upp::String RunDesignerApplyRefactors(CommandOptions& opts);
    Upp::String RunDesignerDiff(CommandOptions& opts);
    Upp::String RunDesignerCodegen(CommandOptions& opts);
    Upp::String RunDesignerRunPlaybook(CommandOptions& opts);
    Upp::String RunDesignerRetime(CommandOptions& opts);
    Upp::String RunDesignerRetimeApply(CommandOptions& opts);
    Upp::String RunDesignerRetimeOpt(CommandOptions& opts);

    // Designer codegen for C++ class and audio demo
    Upp::String RunDesignerCodegenBlockCppClass(CommandOptions& opts);
    Upp::String RunDesignerCodegenBlockAudioDemo(CommandOptions& opts);

    // DSP graph commands
    Upp::String RunDspGraphInspect(CommandOptions& opts);
    Upp::String RunDspRenderOsc(CommandOptions& opts);

    // Analog model commands
    Upp::String RunAnalogModelInspect(CommandOptions& opts);
    Upp::String RunAnalogRenderOsc(CommandOptions& opts);

    // Audio QA analysis commands
    Upp::String RunDesignerQaAnalyze(CommandOptions& opts);
    Upp::String RunDesignerQaDiff(CommandOptions& opts);
    Upp::String RunDesignerQaVerify(CommandOptions& opts);

    // Hybrid instrument commands
    Upp::String RunInstrumentBuildHybrid(CommandOptions& opts);
    Upp::String RunInstrumentRenderHybrid(CommandOptions& opts);
    Upp::String RunInstrumentExportCpp(CommandOptions& opts);
    Upp::String RunInstrumentExportPluginSkeleton(CommandOptions& opts);
    Upp::String RunInstrumentExportPluginProject(CommandOptions& opts);

private:
    std::unique_ptr<ISessionStore> session_store_;

    // Helper to validate workspace path
    bool ValidateWorkspace(const std::string& workspace_path);

};

} // namespace ProtoVMCLI

#endif