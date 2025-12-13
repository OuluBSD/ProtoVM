#ifndef _ProtoVM_JsonIO_h_
#define _ProtoVM_JsonIO_h_

#include <ProtoVM/ProtoVM.h>  // Include U++ types
#include "SessionTypes.h"
#include "CircuitDiagnostics.h"
#include "TimingAnalysis.h"
#include "FunctionalAnalysis.h"
#include "BlockAnalysis.h"
#include "IrOptimization.h"
#include "Playbooks.h"
#include "GlobalPipeline.h"
#include "GlobalPipelining.h"
#include "StructuralSynthesis.h"
#include <string>

namespace ProtoVMCLI {

// Simple JSON utilities for the CLI using U++ types
class JsonIO {
public:
    // Create a success response with a standard envelope
    static Upp::String SuccessResponse(const std::string& command, const Upp::ValueMap& data = Upp::ValueMap());

    // Create an error response with a standard envelope
    static Upp::String ErrorResponse(const std::string& command, const std::string& error_msg, const std::string& error_code = "");

    // Create a response from a Result type with a standard envelope
    template<typename T>
    static Upp::String FromResult(const std::string& command, const Result<T>& result,
                                  std::function<Upp::ValueMap(const T&)> converter = nullptr);

    // Parse command line arguments to ValueMap
    static Upp::ValueMap ParseArgs(int argc, char** argv);

    // Serialize ValueMap to JSON string with consistent formatting
    static Upp::String Serialize(const Upp::ValueMap& obj);

    // Read ValueMap from JSON string
    static Upp::ValueMap Deserialize(const Upp::String& str);

    // Helper to convert ValueMap to JSON string (recursive)
    static Upp::String ValueMapToJson(const Upp::ValueMap& vm);

    // Helper to convert ValueArray to JSON string
    static Upp::String ValueArrayToJson(const Upp::ValueArray& va);

    // Helper to convert a single value to JSON
    static Upp::String ValueToJson(const Upp::Value& val);

    // Convert ErrorCode to string
    static std::string ErrorCodeToString(ErrorCode code);

    // Convert string to ErrorCode
    static ErrorCode StringToErrorCode(const std::string& s);

    // Serialize DiagnosticSeverity to JSON value
    static Upp::Value DiagnosticSeverityToJson(DiagnosticSeverity severity);

    // Serialize DiagnosticKind to JSON value
    static Upp::Value DiagnosticKindToJson(DiagnosticKind kind);

    // Serialize CircuitDiagnosticLocation to ValueMap
    static Upp::ValueMap CircuitDiagnosticLocationToValueMap(const CircuitDiagnosticLocation& location);

    // Serialize CircuitDiagnostic to ValueMap
    static Upp::ValueMap CircuitDiagnosticToValueMap(const CircuitDiagnostic& diagnostic);

    // Serialize vector of CircuitDiagnostic to ValueArray
    static Upp::ValueArray CircuitDiagnosticsToValueArray(const std::vector<CircuitDiagnostic>& diagnostics);

    // Serialize TimingNodeId to ValueMap
    static Upp::ValueMap TimingNodeIdToValueMap(const TimingNodeId& node_id);

    // Serialize TimingPathPoint to ValueMap
    static Upp::ValueMap TimingPathPointToValueMap(const TimingPathPoint& point);

    // Serialize TimingPath to ValueMap
    static Upp::ValueMap TimingPathToValueMap(const TimingPath& path);

    // Serialize TimingSummary to ValueMap
    static Upp::ValueMap TimingSummaryToValueMap(const TimingSummary& summary);

    // Serialize vector of TimingPath to ValueArray
    static Upp::ValueArray TimingPathsToValueArray(const std::vector<TimingPath>& paths);

    // Serialize HazardCandidate to ValueMap
    static Upp::ValueMap HazardCandidateToValueMap(const HazardCandidate& hazard);

    // Serialize vector of HazardCandidate to ValueArray
    static Upp::ValueArray HazardCandidatesToValueArray(const std::vector<HazardCandidate>& hazards);

    // Serialize TimingNodeId to ValueMap
    static Upp::ValueMap TimingNodeIdForJson(const TimingNodeId& node_id);

    // Serialize vector of TimingNodeId to ValueArray
    static Upp::ValueArray TimingNodeIdsToValueArray(const std::vector<TimingNodeId>& nodes);

    // Serialize vector of vectors of TimingNodeId to ValueArray (for loops)
    static Upp::ValueArray TimingLoopsToValueArray(const std::vector<std::vector<TimingNodeId>>& loops);

    // Serialize FunctionalNodeId to ValueMap
    static Upp::ValueMap FunctionalNodeIdToValueMap(const FunctionalNodeId& node_id);

    // Serialize ConeNode to ValueMap
    static Upp::ValueMap ConeNodeToValueMap(const ConeNode& cone_node);

    // Serialize FunctionalCone to ValueMap
    static Upp::ValueMap FunctionalConeToValueMap(const FunctionalCone& cone);

    // Serialize DependencySummary to ValueMap
    static Upp::ValueMap DependencySummaryToValueMap(const DependencySummary& summary);

    // Serialize vector of ConeNode to ValueArray
    static Upp::ValueArray ConeNodesToValueArray(const std::vector<ConeNode>& cone_nodes);

    // Serialize vector of FunctionalCone to ValueArray
    static Upp::ValueArray FunctionalConesToValueArray(const std::vector<FunctionalCone>& cones);

    // Serialize vector of DependencySummary to ValueArray
    static Upp::ValueArray DependencySummariesToValueArray(const std::vector<DependencySummary>& summaries);

    // Serialize BlockKind to JSON value
    static Upp::Value BlockKindToJson(BlockKind kind);

    // Serialize BlockPort to ValueMap
    static Upp::ValueMap BlockPortToValueMap(const BlockPort& port);

    // Serialize BlockInstance to ValueMap
    static Upp::ValueMap BlockInstanceToValueMap(const BlockInstance& block);

    // Serialize BlockGraph to ValueMap
    static Upp::ValueMap BlockGraphToValueMap(const BlockGraph& block_graph);

    // Serialize vector of BlockInstance to ValueArray
    static Upp::ValueArray BlockInstancesToValueArray(const std::vector<BlockInstance>& blocks);

    // Serialize BehaviorKind to JSON value
    static Upp::Value BehaviorKindToJson(BehaviorKind kind);

    // Serialize BehaviorPortRole to ValueMap
    static Upp::ValueMap BehaviorPortRoleToValueMap(const BehaviorPortRole& port_role);

    // Serialize BehaviorDescriptor to ValueMap
    static Upp::ValueMap BehaviorDescriptorToValueMap(const BehaviorDescriptor& descriptor);

    // Serialize vector of BehaviorPortRole to ValueArray
    static Upp::ValueArray BehaviorPortRolesToValueArray(const std::vector<BehaviorPortRole>& port_roles);

    // Serialize TransformationKind to JSON value
    static Upp::Value TransformationKindToJson(TransformationKind kind);

    // Serialize PreservationLevel to JSON value
    static Upp::Value PreservationLevelToJson(PreservationLevel level);

    // Serialize TransformationTarget to ValueMap
    static Upp::ValueMap TransformationTargetToValueMap(const TransformationTarget& target);

    // Serialize TransformationStep to ValueMap
    static Upp::ValueMap TransformationStepToValueMap(const TransformationStep& step);

    // Serialize TransformationPlan to ValueMap
    static Upp::ValueMap TransformationPlanToValueMap(const TransformationPlan& plan);

    // Serialize vector of PreservationLevel to ValueArray
    static Upp::ValueArray PreservationLevelsToValueArray(const Vector<PreservationLevel>& levels);

    // Serialize vector of TransformationStep to ValueArray
    static Upp::ValueArray TransformationStepsToValueArray(const Vector<TransformationStep>& steps);

    // Serialize vector of TransformationPlan to ValueArray
    static Upp::ValueArray TransformationPlansToValueArray(const Vector<TransformationPlan>& plans);

    // HLS IR serialization methods
    static Upp::Value IrExprKindToJson(IrExprKind kind);

    static Upp::ValueMap IrValueToValueMap(const IrValue& value);

    static Upp::ValueMap IrExprToValueMap(const IrExpr& expr);

    static Upp::ValueMap IrRegAssignToValueMap(const IrRegAssign& reg_assign);

    static Upp::ValueMap IrModuleToValueMap(const IrModule& module);

    static Upp::ValueArray IrValuesToValueArray(const std::vector<IrValue>& values);

    static Upp::ValueArray IrExprsToValueArray(const std::vector<IrExpr>& exprs);

    static Upp::ValueArray IrRegAssignsToValueArray(const std::vector<IrRegAssign>& reg_assigns);

    // Diff analysis serialization methods
    static Upp::Value BehaviorChangeKindToJson(BehaviorChangeKind kind);

    static Upp::Value IrChangeKindToJson(IrChangeKind kind);

    static Upp::ValueMap PortChangeToValueMap(const PortChange& port_change);

    static Upp::ValueMap BehaviorDiffToValueMap(const BehaviorDiff& behavior_diff);

    static Upp::ValueMap IrExprChangeToValueMap(const IrExprChange& expr_change);

    static Upp::ValueMap IrRegChangeToValueMap(const IrRegChange& reg_change);

    static Upp::ValueMap IrInterfaceChangeToValueMap(const IrInterfaceChange& iface_change);

    static Upp::ValueMap IrDiffToValueMap(const IrDiff& ir_diff);

    static Upp::ValueArray PortChangesToValueArray(const std::vector<PortChange>& port_changes);

    static Upp::ValueArray IrExprChangesToValueArray(const std::vector<IrExprChange>& expr_changes);

    static Upp::ValueArray IrRegChangesToValueArray(const std::vector<IrRegChange>& reg_changes);

    static Upp::ValueArray IrValuesToValueArraySimple(const std::vector<IrValue>& values);

    // IR Optimization serialization methods
    static Upp::Value IrOptPassKindToJson(IrOptPassKind kind);

    static Upp::ValueMap IrOptChangeSummaryToValueMap(const IrOptChangeSummary& summary);

    // Scheduling serialization methods
    static Upp::Value SchedulingStrategyToJson(SchedulingStrategy strategy);

    static Upp::ValueMap SchedulingConfigToValueMap(const SchedulingConfig& config);

    static Upp::ValueMap ScheduledExprToValueMap(const ScheduledExpr& scheduled_expr);

    static Upp::ValueMap ScheduledRegAssignToValueMap(const ScheduledRegAssign& scheduled_reg_assign);

    static Upp::ValueMap ScheduledModuleToValueMap(const ScheduledModule& scheduled_module);

    static Upp::ValueArray ScheduledExprsToValueArray(const std::vector<ScheduledExpr>& scheduled_exprs);

    static Upp::ValueArray ScheduledRegAssignsToValueArray(const std::vector<ScheduledRegAssign>& scheduled_reg_assigns);

    // Pipeline model serialization methods
    static Upp::ValueMap ClockSignalInfoToValueMap(const ClockSignalInfo& clock_signal);

    static Upp::ValueMap RegisterInfoToValueMap(const RegisterInfo& register_info);

    static Upp::ValueMap PipelineStageInfoToValueMap(const PipelineStageInfo& stage_info);

    static Upp::ValueMap RegToRegPathInfoToValueMap(const RegToRegPathInfo& path_info);

    static Upp::ValueMap PipelineMapToValueMap(const PipelineMap& pipeline_map);

    static Upp::ValueArray ClockSignalInfosToValueArray(const std::vector<ClockSignalInfo>& clock_signals);

    static Upp::ValueArray RegisterInfosToValueArray(const std::vector<RegisterInfo>& registers);

    static Upp::ValueArray PipelineStageInfosToValueArray(const std::vector<PipelineStageInfo>& stages);

    static Upp::ValueArray RegToRegPathInfosToValueArray(const std::vector<RegToRegPathInfo>& paths);

    static Upp::ValueArray IrOptChangeSummariesToValueArray(const std::vector<IrOptChangeSummary>& summaries);

    static Upp::ValueMap IrOptimizationResultToValueMap(const IrOptimizationResult& result);

    // CDC model serialization methods
    static Upp::Value CdcCrossingKindToJson(CdcCrossingKind kind);

    static Upp::Value CdcSeverityToJson(CdcSeverity severity);

    static Upp::ValueMap CdcCrossingEndpointToValueMap(const CdcCrossingEndpoint& endpoint);

    static Upp::ValueMap CdcCrossingToValueMap(const CdcCrossing& crossing);

    static Upp::ValueMap CdcIssueToValueMap(const CdcIssue& issue);

    static Upp::ValueMap CdcReportToValueMap(const CdcReport& report);

    static Upp::ValueArray CdcCrossingsToValueArray(const Vector<CdcCrossing>& crossings);

    static Upp::ValueArray CdcIssuesToValueArray(const Vector<CdcIssue>& issues);

    // Retiming model serialization methods
    static Upp::Value RetimingMoveDirectionToJson(RetimingMoveDirection direction);

    static Upp::Value RetimingMoveSafetyToJson(RetimingMoveSafety safety);

    static Upp::ValueMap RetimingMoveToValueMap(const RetimingMove& move);

    static Upp::ValueMap RetimingPlanToValueMap(const RetimingPlan& plan);

    static Upp::ValueMap RetimingApplicationResultToValueMap(const RetimingApplicationResult& result);

    static Upp::ValueArray RetimingMovesToValueArray(const Vector<RetimingMove>& moves);

    static Upp::ValueArray RetimingPlansToValueArray(const Vector<RetimingPlan>& plans);

    // Retiming optimization serialization methods
    static Upp::Value RetimingObjectiveKindToJson(RetimingObjectiveKind kind);

    static Upp::ValueMap RetimingObjectiveToValueMap(const RetimingObjective& objective);

    static Upp::ValueMap RetimingPlanScoreToValueMap(const RetimingPlanScore& score);

    static Upp::ValueMap RetimingOptimizationResultToValueMap(const RetimingOptimizationResult& result);

    static Upp::ValueArray RetimingPlanScoresToValueArray(const Vector<RetimingPlanScore>& scores);

    // Global Pipeline serialization methods
    static Upp::Value GlobalPipeliningStrategyKindToJson(GlobalPipeliningStrategyKind kind);

    static Upp::ValueMap GlobalPipelinePathToValueMap(const GlobalPipelinePath& path);

    static Upp::ValueMap GlobalPipelineStageToValueMap(const GlobalPipelineStage& stage);

    static Upp::ValueMap GlobalPipelineMapToValueMap(const GlobalPipelineMap& global_pipeline);

    static Upp::ValueMap GlobalPipeliningObjectiveToValueMap(const GlobalPipeliningObjective& objective);

    static Upp::ValueMap GlobalPipeliningStepToValueMap(const GlobalPipeliningStep& step);

    static Upp::ValueMap GlobalPipeliningPlanToValueMap(const GlobalPipeliningPlan& plan);

    static Upp::ValueArray GlobalPipelinePathsToValueArray(const std::vector<GlobalPipelinePath>& paths);

    static Upp::ValueArray GlobalPipelineStagesToValueArray(const std::vector<GlobalPipelineStage>& stages);

    static Upp::ValueArray GlobalPipeliningObjectivesToValueArray(const std::vector<GlobalPipeliningObjective>& objectives);

    static Upp::ValueArray GlobalPipeliningStepsToValueArray(const std::vector<GlobalPipeliningStep>& steps);

    static Upp::ValueArray GlobalPipeliningPlansToValueArray(const std::vector<GlobalPipeliningPlan>& plans);

    // CoDesigner serialization methods
    static Upp::ValueMap CoDesignerSessionStateToValueMap(const CoDesignerSessionState& session);

    // Playbook serialization methods
    static Upp::Value PlaybookKindToJson(PlaybookKind kind);

    static Upp::ValueMap PlaybookConfigToValueMap(const PlaybookConfig& config);

    // Codegen IR serialization methods
    static Upp::Value CodegenTargetLanguageToJson(CodegenTargetLanguage lang);
    static Upp::Value CodegenExprKindToJson(CodegenExprKind kind);
    static Upp::Value CodegenStorageKindToJson(CodegenStorageKind kind);

    static Upp::ValueMap CodegenValueToValueMap(const CodegenValue& value);
    static Upp::ValueMap CodegenExprToValueMap(const CodegenExpr& expr);
    static Upp::ValueMap CodegenAssignmentToValueMap(const CodegenAssignment& assign);
    static Upp::ValueMap CodegenModuleToValueMap(const CodegenModule& module);

    static Upp::ValueArray CodegenValuesToValueArray(const std::vector<CodegenValue>& values);
    static Upp::ValueArray CodegenExprsToValueArray(const std::vector<CodegenExpr>& exprs);
    static Upp::ValueArray CodegenAssignmentsToValueArray(const std::vector<CodegenAssignment>& assigns);

    // Audio DSL serialization methods
    static Upp::ValueMap AudioDslOscillatorToValueMap(const AudioDslOscillator& oscillator);
    static Upp::ValueMap AudioDslPanLfoToValueMap(const AudioDslPanLfo& pan_lfo);
    static Upp::ValueMap AudioDslOutputConfigToValueMap(const AudioDslOutputConfig& output_config);
    static Upp::ValueMap AudioDslGraphToValueMap(const AudioDslGraph& graph);

    static Upp::ValueMap BlockPlaybookResultToValueMap(const BlockPlaybookResult& result);

    static Upp::ValueMap PlaybookResultToValueMap(const PlaybookResult& result);

    static Upp::ValueArray TransformationPlansToValueArray(const std::vector<TransformationPlan>& plans);

    static Upp::ValueArray StringVectorToValueArray(const std::vector<std::string>& strings);

    // Structural synthesis data structure serialization functions
    static Upp::Value StructuralPatternKindToJson(StructuralPatternKind kind);
    static Upp::ValueMap StructuralPatternToValueMap(const StructuralPattern& pattern);
    static Upp::ValueArray StructuralPatternsToValueArray(const Vector<StructuralPattern>& patterns);

    static Upp::Value StructuralRefactorSafetyToJson(StructuralRefactorSafety safety);
    static Upp::ValueMap StructuralRefactorMoveToValueMap(const StructuralRefactorMove& move);
    static Upp::ValueArray StructuralRefactorMovesToValueArray(const Vector<StructuralRefactorMove>& moves);

    static Upp::ValueMap StructuralRefactorPlanToValueMap(const StructuralRefactorPlan& plan);

    // DSP graph serialization methods
    static Upp::Value DspNodeKindToJson(DspNodeKind kind);
    static Upp::Value DspPortDirectionToJson(DspPortDirection direction);
    static Upp::Value DspPortTypeToJson(DspPortType type);

    static Upp::ValueMap DspPortIdToValueMap(const DspPortId& port_id);
    static Upp::ValueMap DspNodeToValueMap(const DspNode& node);
    static Upp::ValueMap DspConnectionToValueMap(const DspConnection& connection);
    static Upp::ValueMap DspGraphToValueMap(const DspGraph& graph);

    static Upp::ValueArray DspNodesToValueArray(const std::vector<DspNode>& nodes);
    static Upp::ValueArray DspConnectionsToValueArray(const std::vector<DspConnection>& connections);

    // Analog model serialization methods
    static Upp::Value AnalogBlockKindToJson(AnalogBlockKind kind);
    static Upp::Value AnalogStateKindToJson(AnalogStateKind kind);
    static Upp::ValueMap AnalogStateVarToValueMap(const AnalogStateVar& state);
    static Upp::ValueMap AnalogParamToValueMap(const AnalogParam& param);
    static Upp::ValueMap AnalogBlockModelToValueMap(const AnalogBlockModel& model);

    static Upp::ValueArray AnalogStateVarsToValueArray(const std::vector<AnalogStateVar>& states);
    static Upp::ValueArray AnalogParamsToValueArray(const std::vector<AnalogParam>& params);

    // InstrumentGraph serialization methods
    static Upp::ValueMap NoteDescToValueMap(const NoteDesc& note);
    static Upp::ValueMap VoiceConfigToValueMap(const VoiceConfig& voice);
    static Upp::ValueMap InstrumentVoiceTemplateToValueMap(const InstrumentVoiceTemplate& template_);
    static Upp::ValueMap InstrumentGraphToValueMap(const InstrumentGraph& instrument);

    static Upp::ValueArray VoiceConfigsToValueArray(const std::vector<VoiceConfig>& voices);

    // PluginTargetKind serialization methods
    static Upp::Value PluginTargetKindToJson(PluginTargetKind kind);
};

// Template implementation needs to be in header
template<typename T>
Upp::String JsonIO::FromResult(const std::string& command, const Result<T>& result,
                               std::function<Upp::ValueMap(const T&)> converter) {
    if (result.ok) {
        Upp::ValueMap data;
        if (converter) {
            data = converter(result.data);
        } else {
            // For simple types, just return an empty object or add a value field
            data.Add("value", Upp::Value(result.data));
        }
        return SuccessResponse(command, data);
    } else {
        std::string code_str = ErrorCodeToString(result.error_code);
        return ErrorResponse(command, result.error_message, code_str);
    }
}

} // namespace ProtoVMCLI

#endif