#ifndef _ProtoVM_CircuitFacade_h_
#define _ProtoVM_CircuitFacade_h_

#include "SessionTypes.h"
#include "CircuitData.h"  // The enhanced version with IDs
#include <ProtoVM/ProtoVM.h>      // For Upp types
#include "CollaborationTypes.h"  // For collaboration features
#include "BehavioralAnalysis.h"  // For behavioral analysis
#include "ScheduledIr.h"         // For scheduled IR
#include "Scheduling.h"          // For scheduling engine
#include "PipelineModel.h"       // For pipeline analysis
#include "PipelineAnalysis.h"    // For pipeline analysis engine
#include "CdcModel.h"            // For CDC analysis
#include "CdcAnalysis.h"         // For CDC analysis engine
#include "RetimingTransform.h"   // For retiming transformation
#include "RetimingOpt.h"         // For retiming optimization
#include "GlobalPipeline.h"      // For global pipeline analysis
#include "GlobalPipelineAnalysis.h" // For global pipeline analysis engine
#include "GlobalPipelining.h"    // For global pipelining engine
#include "StructuralSynthesis.h" // For structural synthesis analysis
#include "CodegenIr.h"           // For codegen IR structures
#include "CodegenIrInference.h"  // For codegen IR inference
#include "CodeEmitter.h"         // For code emission
#include "CodegenCpp.h"          // For C++ class options
#include "AudioDsl.h"            // For audio DSL structures
#include "DspGraph.h"            // For DSP graph structures
#include "DspRuntime.h"          // For DSP runtime
#include "AnalogModel.h"         // For analog model structures
#include "InstrumentGraph.h"     // For instrument graph structures
#include "PluginSkeletonExport.h" // For plugin skeleton export
#include <string>
#include <vector>
#include <optional>

// Forward declarations for functional analysis types
namespace ProtoVMCLI {
    struct FunctionalNodeId;
    struct FunctionalCone;
    struct DependencySummary;
}

namespace ProtoVMCLI {

// Forward declaration
struct EditOperation;

// Information about a circuit revision
struct CircuitRevisionInfo {
    int64_t revision;
};

// Exported circuit state
struct CircuitStateExport {
    int64_t revision;
    Upp::String circuit_json;  // JSON representation of the circuit
};

// Result for circuit operations
template<typename T>
struct Result;

// Forward declaration
class ISessionStore;

// Circuit facade to handle circuit operations
class CircuitFacade {
public:
    explicit CircuitFacade(std::shared_ptr<ISessionStore> session_store = nullptr)
        : session_store_(session_store) {}

    // Alternative constructor without session store - for use when session saving is handled separately
    explicit CircuitFacade() : session_store_(nullptr) {}

    // Load the current circuit state for a session:
    // - initial circuit (from .circuit or snapshot)
    // - plus all edit events up to circuit_revision.
    Result<CircuitRevisionInfo> LoadCurrentCircuit(
        const SessionMetadata& session,
        const std::string& session_dir,
        CircuitData& out_circuit
    );

    // Branch-aware version: Load circuit state for a specific branch
    Result<CircuitRevisionInfo> LoadCurrentCircuitForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        CircuitData& out_circuit
    );

    // Apply one or more editing operations to the circuit,
    // persist them as events, and bump the circuit_revision.
    Result<CircuitRevisionInfo> ApplyEditOperations(
        SessionMetadata& session,
        const std::string& session_dir,
        const std::vector<EditOperation>& ops,
        const std::string& user_id
    );

    // Branch-aware version: Apply editing operations to a specific branch
    Result<CircuitRevisionInfo> ApplyEditOperationsToBranch(
        SessionMetadata& session,
        const std::string& session_dir,
        const std::vector<EditOperation>& ops,
        const std::string& user_id,
        const std::string& branch_name
    );

    // Optional: export entire circuit state as JSON for clients.
    Result<CircuitStateExport> ExportCircuitState(
        const SessionMetadata& session,
        const std::string& session_dir
    );

    // Graph building methods
    Result<CircuitGraph> BuildGraphForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name
    );

    // Timing analysis methods
    Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>> BuildTimingGraphForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name
    );

    // Functional analysis methods
    Result<FunctionalCone> BuildBackwardConeForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

    Result<FunctionalCone> BuildForwardConeForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

    Result<DependencySummary> BuildDependencySummaryForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

    // Block analysis methods
    Result<BlockGraph> BuildBlockGraphForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name
    );

    // Behavioral analysis methods
    Result<BehaviorDescriptor> InferBehaviorForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<BehaviorDescriptor> InferBehaviorForNodeInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& node_id,
        const std::string& node_kind_hint
    );

    // HLS IR analysis methods
    Result<IrModule> BuildIrForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<IrModule> BuildIrForNodeRegionInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& node_id,
        const std::string& node_kind_hint,
        int max_depth = 4
    );

    // Diff analysis methods
    Result<BehaviorDiff> DiffBlockBehaviorBetweenBranches(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_before,
        const std::string& branch_after,
        const std::string& block_id
    );

    Result<IrDiff> DiffBlockIrBetweenBranches(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_before,
        const std::string& branch_after,
        const std::string& block_id
    );

    Result<IrDiff> DiffNodeRegionIrBetweenBranches(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_before,
        const std::string& branch_after,
        const std::string& node_id,
        const std::string& node_kind_hint,
        int max_depth = 4
    );

    // Transformation methods
    Result<std::vector<TransformationPlan>> ProposeTransformationsForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        int max_plans
    );

    Result<std::vector<TransformationPlan>> ProposeTransformationsForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        int max_plans
    );

    Result<void> ApplyTransformationPlan(
        SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const TransformationPlan& plan,
        const std::string& user_id
    );

    // IR optimization methods
    Result<IrOptimizationResult> OptimizeBlockIrInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const std::vector<IrOptPassKind>& passes_to_run
    );

    // Generate transformation plans from IR optimization for a block.
    Result<std::vector<TransformationPlan>> ProposeIrBasedTransformationsForBlock(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const std::vector<IrOptPassKind>& passes_to_run
    );

    // Scheduled IR analysis methods
    Result<ScheduledModule> BuildScheduledIrForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const SchedulingConfig& config
    );

    Result<ScheduledModule> BuildScheduledIrForNodeRegionInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& node_id,
        const std::string& node_kind_hint,
        int max_depth,
        const SchedulingConfig& config
    );

    // Pipeline analysis methods
    Result<PipelineMap> BuildPipelineMapForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<PipelineMap> BuildPipelineMapForSubsystemInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& subsystem_id,
        const std::vector<std::string>& block_ids
    );

    // CDC analysis methods
    Result<CdcReport> BuildCdcReportForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<CdcReport> BuildCdcReportForSubsystemInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& subsystem_id,
        const std::vector<std::string>& block_ids
    );

    // Retiming analysis methods
    Result<Vector<RetimingPlan>> AnalyzeRetimingForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<Vector<RetimingPlan>> AnalyzeRetimingForSubsystemInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& subsystem_id,
        const Vector<String>& block_ids
    );

    // Retiming application methods
    Result<RetimingApplicationResult> ApplyRetimingPlanForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const RetimingPlan& plan,
        const RetimingApplicationOptions& options
    );

    Result<RetimingApplicationResult> ApplyRetimingPlanForSubsystemInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const RetimingPlan& plan,
        const RetimingApplicationOptions& options
    );

    // Retiming optimization methods
    Result<RetimingOptimizationResult> OptimizeRetimingForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const RetimingObjective& objective,
        const RetimingApplicationOptions* app_options = nullptr // optional; if null, just evaluate
    );

    Result<RetimingOptimizationResult> OptimizeRetimingForSubsystemInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& subsystem_id,
        const Vector<String>& block_ids,
        const RetimingObjective& objective,
        const RetimingApplicationOptions* app_options = nullptr // optional; if null, just evaluate
    );

    // Global pipelining analysis methods
    Result<GlobalPipelineMap> BuildGlobalPipelineMapForSubsystemInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& subsystem_id,
        const Vector<String>& block_ids
    );

    // Global pipelining proposal & application.
    Result<Vector<GlobalPipeliningPlan>> ProposeGlobalPipeliningPlansInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& subsystem_id,
        const Vector<String>& block_ids,
        const GlobalPipeliningObjective& objective
    );

    Result<GlobalPipeliningPlan> ApplyGlobalPipeliningPlanInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const GlobalPipeliningPlan& plan,
        const RetimingApplicationOptions& app_options
    );

    // Structural synthesis analysis methods
    Result<StructuralRefactorPlan> AnalyzeBlockStructureInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<RetimingApplicationResult> ApplyStructuralRefactorPlanInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const StructuralRefactorPlan& plan,
        bool apply_only_safe_moves
    );

    // Codegen IR inference methods
    Result<CodegenModule> BuildCodegenModuleForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<CodegenModule> BuildCodegenModuleForNodeRegionInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const std::vector<std::string>& node_ids
    );

    // Code emission methods
    Result<std::string> EmitCodeForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        CodegenTargetLanguage lang,
        bool emit_state_struct = true,
        const std::string& state_struct_name = "BlockState",
        const std::string& function_name = "BlockStep"
    );

    // Optional oscillator demo:
    Result<std::string> EmitOscillatorDemoForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        CodegenTargetLanguage lang
    );

    // C++ class emission methods
    Result<std::string> EmitCppClassForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const CppClassOptions& options
    );

    // Audio DSL methods
    Result<AudioDslGraph> BuildAudioDslForOscillatorBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        double target_frequency_hz,
        double pan_lfo_rate_hz,
        double sample_rate_hz,
        double duration_sec
    );

    // Audio demo methods
    Result<std::string> EmitAudioDemoForOscillatorBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const CppClassOptions& class_opts,
        const AudioDslGraph& graph
    );

    // DSP graph methods
    Result<DspGraph> BuildDspGraphForOscillatorBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const AudioDslGraph& audio_graph
    );

    Result<void> RenderDspGraphForOscillatorBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const AudioDslGraph& audio_graph,
        std::vector<float>& out_left,
        std::vector<float>& out_right
    );

    // Analog model methods
    Result<AnalogBlockModel> ExtractAnalogModelForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<void> RenderAnalogBlockAsAudioInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const AudioDslGraph& audio_graph,
        std::vector<float>& out_left,
        std::vector<float>& out_right
    );

    // Hybrid instrument methods
    Result<InstrumentGraph> BuildHybridInstrumentInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const InstrumentVoiceTemplate& voice_template,
        double sample_rate_hz,
        int voice_count,
        const NoteDesc& note,
        double detune_spread_cents
    );

    Result<void> RenderHybridInstrumentInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const InstrumentGraph& instrument,
        std::vector<float>& out_left,
        std::vector<float>& out_right
    );

    // Instrument export methods
    Result<std::string> ExportInstrumentAsStandaloneCppInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const InstrumentGraph& instrument,
        const InstrumentExportOptions& options
    );

    // Plugin skeleton export methods
    Result<String> ExportPluginSkeletonForInstrumentInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const InstrumentGraph& instrument,
        const PluginSkeletonOptions& opts
    );

    // Plugin project export methods
    Result<void> ExportPluginProjectForInstrumentInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const InstrumentGraph& instrument,
        const PluginProjectExportOptions& opts
    );

private:
    // Internal helper to load circuit from initial file
    Result<bool> LoadInitialCircuit(const std::string& circuit_file_path, CircuitData& out_circuit);

    // Internal helper to apply an edit operation to a circuit
    Result<bool> ApplyEditOperation(CircuitData& circuit, const EditOperation& op);

    // Internal helper to replay circuit events and update the circuit
    Result<bool> ReplayCircuitEvents(CircuitData& circuit, const std::string& session_dir, int64_t from_revision, int64_t to_revision);

    // Internal helper to replay circuit events for a specific branch
    Result<bool> ReplayCircuitEventsForBranch(CircuitData& circuit, const std::string& session_dir, int64_t from_revision, int64_t to_revision, const std::string& branch_name);

    // Internal helper to get the latest circuit snapshot revision
    int64_t GetLatestCircuitSnapshotRevision(const std::string& session_dir);

    // Internal helper to load circuit from a snapshot
    Result<bool> LoadCircuitFromSnapshot(const std::string& session_dir, CircuitData& out_circuit);

    // Internal helper to save circuit snapshot
    Result<bool> SaveCircuitSnapshot(const CircuitData& circuit, const std::string& session_dir, int64_t revision);

    // Internal helper for retiming analysis
    Result<Vector<RetimingPlan>> PerformRetimingAnalysis(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& target_id,
        const Vector<String>& block_ids,
        bool is_subsystem
    );

public:
    std::shared_ptr<ISessionStore> GetSessionStore() const {
        return session_store_;
    }

private:
    std::shared_ptr<ISessionStore> session_store_;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CircuitFacade_h_