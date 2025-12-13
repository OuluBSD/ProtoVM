#ifndef _ProtoVM_CoDesigner_h_
#define _ProtoVM_CoDesigner_h_

#include "SessionTypes.h"
#include "CircuitFacade.h"
#include "HlsIr.h"  // For IrModule
#include "HlsIrInference.h"  // For IR generation
#include "Transformations.h"  // For transformation plans
#include "DiffAnalysis.h"  // For diff operations
#include "CodegenIr.h"  // For code generation
#include "BehavioralAnalysis.h"  // For behavioral analysis
#include "RetimingModel.h"  // For retiming analysis
#include "GlobalPipeline.h"      // For global pipeline structures
#include "GlobalPipelining.h"    // For global pipelining structures
#include "StructuralSynthesis.h" // For structural synthesis analysis
#include "AnalogModel.h"         // For analog model structures
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace ProtoVMCLI {

// CoDesigner session state representing a high-level design session
struct CoDesignerSessionState {
    std::string designer_session_id;
    int proto_session_id;
    std::string branch;
    std::string current_block_id;
    std::string current_node_id;
    std::string current_node_kind;  // "Pin" / "Component" / "Net" / empty
    bool use_optimized_ir;
    
    CoDesignerSessionState() 
        : proto_session_id(-1), 
          use_optimized_ir(false) {}
};

// Request parameters for various designer commands
struct DesignerCreateSessionRequest {
    int proto_session_id;
    std::string branch;
};

struct DesignerSetFocusRequest {
    std::string designer_session_id;
    std::string block_id;
    std::string node_id;
    std::string node_kind;
    bool use_optimized_ir;
};

struct DesignerGetContextRequest {
    std::string designer_session_id;
};

struct DesignerAnalyzeRequest {
    std::string designer_session_id;
    bool include_behavior;
    bool include_ir;
    bool include_graph_stats;
    bool include_timing;
};

struct DesignerOptimizeRequest {
    std::string designer_session_id;
    std::string target;  // "block" or "node"
    std::vector<IrOptPassKind> passes;
};

struct DesignerProposeRefactorsRequest {
    std::string designer_session_id;
    std::string target;  // "block" or "node"
    std::vector<IrOptPassKind> passes;
};

struct DesignerApplyRefactorsRequest {
    std::string designer_session_id;
    std::vector<TransformationPlan> plans;
    std::string user_id;
    bool allow_unverified;
};

struct DesignerDiffRequest {
    std::string designer_session_id;
    std::string compare_branch;  // or other reference branch
    bool include_behavior_diff;
    bool include_ir_diff;
};

struct DesignerCodegenRequest {
    std::string designer_session_id;
    std::string target;  // "block" or "node"
    std::string flavor;  // "PseudoVerilog", etc.
    bool use_optimized_ir;
};

// New codegen request structures
struct DesignerCodegenBlockCRequest {
    std::string designer_session_id;
    std::string block_id;
    CodegenTargetLanguage lang;
    bool emit_state_struct;
    std::string state_struct_name;
    std::string function_name;
};

struct DesignerCodegenOscDemoRequest {
    std::string designer_session_id;
    std::string block_id;
    CodegenTargetLanguage lang;
    std::string state_struct_name;
    std::string step_function_name;
    std::string render_function_name;
};

// Response structures for designer commands
struct DesignerCreateSessionResponse {
    CoDesignerSessionState designer_session;
};

struct DesignerSetFocusResponse {
    CoDesignerSessionState designer_session;
};

struct DesignerGetContextResponse {
    CoDesignerSessionState designer_session;
    std::optional<BehaviorDescriptor> block_behavior;
    std::optional<BehaviorDescriptor> node_behavior;
};

struct BlockAnalysisResult {
    std::string block_id;
    BehaviorDescriptor behavior;
    IrModule ir;
};

struct NodeAnalysisResult {
    std::string node_id;
    BehaviorDescriptor behavior;
    IrModule ir;
};

struct DesignerAnalyzeResponse {
    CoDesignerSessionState designer_session;
    std::optional<BlockAnalysisResult> block;  // Contains behavior and IR if block_id is set
    std::optional<NodeAnalysisResult> node;    // Contains behavior and IR if node_id is set
};

struct DesignerOptimizeResponse {
    CoDesignerSessionState designer_session;
    struct OptimizationResult {
        IrModule original;
        IrModule optimized;
        std::vector<IrOptChangeSummary> summaries;
    } optimization;
};

struct DesignerProposeRefactorsResponse {
    CoDesignerSessionState designer_session;
    std::vector<TransformationPlan> plans;
};

struct DesignerApplyRefactorsResponse {
    CoDesignerSessionState designer_session;
    std::vector<std::string> applied_plan_ids;
    int new_circuit_revision;
};

struct DesignerDiffResponse {
    CoDesignerSessionState designer_session;
    std::optional<BehaviorDiff> behavior_diff;
    std::optional<IrDiff> ir_diff;
};

struct DesignerCodegenResponse {
    CoDesignerSessionState designer_session;
    struct CodegenResult {
        std::string id;
        std::string name;
        std::string flavor;
        std::string code;
    } codegen;
};

// New codegen response structures
struct DesignerCodegenBlockCResponse {
    CoDesignerSessionState designer_session;
    struct CodegenBlockCResult {
        std::string block_id;
        CodegenTargetLanguage lang;
        std::string code;
        std::string state_struct_name;
        std::string function_name;
    } result;
};

struct DesignerCodegenOscDemoResponse {
    CoDesignerSessionState designer_session;
    struct CodegenOscDemoResult {
        std::string block_id;
        CodegenTargetLanguage lang;
        std::string osc_code;
    } result;
};

// Designer retiming request structures
struct DesignerRetimeRequest {
    std::string designer_session_id;
    std::string target;  // "block" or "subsystem"
    std::string block_id;  // for block target
    std::string subsystem_id;  // for subsystem target
    std::vector<std::string> block_ids;  // for subsystem target
    int min_depth;  // minimum depth threshold for paths to consider
    int max_plans;  // maximum number of plans to return
};

struct DesignerRetimeResponse {
    CoDesignerSessionState designer_session;
    std::vector<RetimingPlan> retiming_plans;

    // Constructor
    DesignerRetimeResponse() = default;
};

// Designer retiming application request/response structures
struct DesignerRetimeApplyRequest {
    std::string designer_session_id;
    std::string target;  // "block" or "subsystem"
    std::string plan_id;  // ID of the plan to apply
    bool apply_only_safe = true;  // Apply only safe moves (default true)
    bool allow_suspicious = false;  // Allow suspicious moves (default false)
    int max_moves = -1;  // Max number of moves to apply (-1 for no limit)
};

struct DesignerRetimeApplyResponse {
    CoDesignerSessionState designer_session;
    RetimingApplicationResult application_result;

    // Constructor
    DesignerRetimeApplyResponse() = default;
};

// Designer retiming optimization request/response structures
struct DesignerRetimeOptRequest {
    std::string designer_session_id;
    std::string target;  // "block" or "subsystem"
    std::string block_id;  // for block target
    std::string subsystem_id;  // for subsystem target
    std::vector<std::string> block_ids;  // for subsystem target
    RetimingObjective objective;
    bool apply = false;  // Whether to auto-apply the best plan
    bool apply_only_safe = true;  // Apply only safe moves (default true)
    bool allow_suspicious = false;  // Allow suspicious moves (default false)
};

struct DesignerRetimeOptResponse {
    CoDesignerSessionState designer_session;
    RetimingOptimizationResult optimization_result;

    // Constructor
    DesignerRetimeOptResponse() = default;
};

// Designer global pipelining request/response structures
struct DesignerGlobalPipelineRequest {
    std::string designer_session_id;
    std::string target;  // "subsystem"
    std::string subsystem_id;  // for subsystem target
    std::vector<std::string> block_ids;  // for subsystem target
    bool analyze_only = true;  // Whether to only analyze (true) or propose optimizations (false)
};

struct DesignerGlobalPipelineResponse {
    CoDesignerSessionState designer_session;
    std::optional<GlobalPipelineMap> global_pipeline;
    std::optional<std::vector<GlobalPipeliningPlan>> global_plans;

    // Constructor
    DesignerGlobalPipelineResponse() = default;
};

struct DesignerGlobalPipelineOptRequest {
    std::string designer_session_id;
    std::string target;  // "subsystem"
    std::string subsystem_id;  // for subsystem target
    std::vector<std::string> block_ids;  // for subsystem target
    GlobalPipeliningObjective objective;
    bool apply = false;  // Whether to auto-apply the best plan
    bool apply_only_safe = true;  // Apply only safe moves (default true)
    bool allow_suspicious = false;  // Allow suspicious moves (default false)
};

struct DesignerGlobalPipelineApplyRequest {
    std::string designer_session_id;
    std::string plan_id;  // ID of the plan to apply
    bool apply_only_safe = true;  // Apply only safe moves (default true)
    bool allow_suspicious = false;  // Allow suspicious moves (default false)
    int max_moves = -1;  // Max number of moves to apply (-1 for no limit)
};

struct DesignerGlobalPipelineApplyResponse {
    CoDesignerSessionState designer_session;
    GlobalPipeliningPlan application_result;

    // Constructor
    DesignerGlobalPipelineApplyResponse() = default;
};

// Designer structural synthesis request/response structures
struct DesignerStructAnalyzeRequest {
    std::string designer_session_id;
    std::string target;  // "block" (currently only block is supported)
    std::string block_id;  // for block target
};

struct DesignerStructAnalyzeResponse {
    CoDesignerSessionState designer_session;
    std::optional<StructuralRefactorPlan> structural_refactor_plan;

    // Constructor
    DesignerStructAnalyzeResponse() = default;
};

struct DesignerStructApplyRequest {
    std::string designer_session_id;
    std::string plan_id;  // ID of the plan to apply
    bool apply_only_safe = true;  // Apply only safe moves (default true)
    bool allow_suspicious = false;  // Allow suspicious moves (default false)
};

struct DesignerStructApplyResponse {
    CoDesignerSessionState designer_session;
    RetimingApplicationResult application_result;

    // Constructor
    DesignerStructApplyResponse() = default;
};

// Designer DSP graph request/response structures
struct DesignerDspGraphInspectRequest {
    std::string designer_session_id;
    std::string target;  // "block"
    std::string block_id;
    double freq_hz;
    double pan_lfo_hz;
    double sample_rate;
    double duration_sec;
};

struct DesignerDspGraphInspectResponse {
    CoDesignerSessionState designer_session;
    DspGraph dsp_graph;

    // Constructor
    DesignerDspGraphInspectResponse() = default;
};

struct DesignerDspRenderOscRequest {
    std::string designer_session_id;
    std::string target;  // "block"
    std::string block_id;
    double freq_hz;
    double pan_lfo_hz;
    double sample_rate;
    double duration_sec;
};

struct DesignerDspRenderOscResponse {
    CoDesignerSessionState designer_session;
    std::vector<float> left_samples;
    std::vector<float> right_samples;
    struct RenderStats {
        double sample_rate_hz;
        double duration_sec;
        double left_rms;
        double right_rms;
        double left_min;
        double left_max;
        double right_min;
        double right_max;
        int total_samples;
    } render_stats;

    // Constructor
    DesignerDspRenderOscResponse() = default;
};

// Designer Analog Model request/response structures
struct DesignerAnalogModelInspectRequest {
    std::string designer_session_id;
    std::string target;  // "block"
    std::string block_id;
};

struct DesignerAnalogModelInspectResponse {
    CoDesignerSessionState designer_session;
    AnalogBlockModel analog_model;

    // Constructor
    DesignerAnalogModelInspectResponse() = default;
};

struct DesignerAnalogRenderOscRequest {
    std::string designer_session_id;
    std::string target;  // "block"
    std::string block_id;
    double sample_rate_hz;
    double duration_sec;
    double pan_lfo_hz;
};

struct DesignerAnalogRenderOscResponse {
    CoDesignerSessionState designer_session;
    std::vector<float> left_samples;
    std::vector<float> right_samples;
    struct RenderStats {
        double sample_rate_hz;
        double duration_sec;
        double estimated_freq_hz;
        double pan_lfo_hz;
        double left_rms;
        double right_rms;
        double left_min;
        double left_max;
        double right_min;
        double right_max;
        int total_samples;
    } render_stats;

    // Constructor
    DesignerAnalogRenderOscResponse() = default;
};

// Request/response structures for hybrid instrument commands
struct DesignerBuildHybridInstrumentRequest {
    std::string designer_session_id;
    std::string instrument_id;
    std::string analog_block_id;
    std::string digital_block_id;
    int voice_count;
    double sample_rate_hz;
    double duration_sec;
    double base_freq_hz;
    double detune_spread_cents;
    double pan_lfo_hz;
    bool use_analog_primary;
};

struct DesignerRenderHybridInstrumentRequest {
    std::string designer_session_id;
    std::string instrument_id;
    std::string analog_block_id;
    std::string digital_block_id;
    int voice_count;
    double sample_rate_hz;
    double duration_sec;
    double base_freq_hz;
    double detune_spread_cents;
    double pan_lfo_hz;
    bool use_analog_primary;
};

struct DesignerHybridInstrumentResponse {
    CoDesignerSessionState designer_session;
    InstrumentGraph instrument;
    std::vector<float> left_preview;  // First 1000 samples for preview
    std::vector<float> right_preview;
    double left_rms;
    double right_rms;
    int sample_rate_hz;
    int voice_count;
    double duration_sec;
};

// Request/response structures for instrument export commands
struct DesignerInstrumentExportCppRequest {
    std::string designer_session_id;
    std::string instrument_id;
    std::string analog_block_id;
    std::string digital_block_id;
    int voice_count;
    double sample_rate_hz;
    double duration_sec;
    double base_freq_hz;
    double detune_spread_cents;
    double pan_lfo_hz;
    bool use_analog_primary;
    // Export options
    std::string program_name;
    std::string namespace_name;
    std::string wav_filename;
    bool include_wav_writer;
    bool emit_comment_banner;
};

struct DesignerInstrumentExportCppResponse {
    CoDesignerSessionState designer_session;
    std::string instrument_id;
    std::string program_name;
    std::string cpp_source;
};

// Request/response structures for plugin skeleton export commands
struct DesignerInstrumentExportPluginSkeletonRequest {
    std::string designer_session_id;
    std::string instrument_id;
    std::string analog_block_id;
    std::string digital_block_id;
    int voice_count;
    double sample_rate_hz;
    double duration_sec;
    double base_freq_hz;
    double detune_spread_cents;
    double pan_lfo_hz;
    bool use_analog_primary;
    // Plugin skeleton options
    std::string plugin_target;            // Target plugin format (vst3, lv2, clap, ladspa)
    std::string plugin_name;              // Plugin name
    std::string plugin_id;                // Plugin ID
    std::string vendor;                   // Plugin vendor name
};

struct DesignerInstrumentExportPluginSkeletonResponse {
    CoDesignerSessionState designer_session;
    std::string instrument_id;
    std::string plugin_target;
    std::string plugin_name;
    std::string plugin_id;
    std::string skeleton_source;
};

// Request/response structures for plugin project export commands
struct DesignerInstrumentExportPluginProjectRequest {
    std::string designer_session_id;
    std::string instrument_id;
    std::string analog_block_id;
    std::string digital_block_id;
    int voice_count;
    double sample_rate_hz;
    double duration_sec;
    double base_freq_hz;
    double detune_spread_cents;
    double pan_lfo_hz;
    bool use_analog_primary;
    // Plugin project export options
    std::string plugin_target;            // Target plugin format (vst3, lv2, clap, ladspa)
    std::string plugin_name;              // Plugin name
    std::string plugin_id;                // Plugin ID
    std::string vendor;                   // Plugin vendor name
    std::string version;                  // Plugin version (e.g., "1.0.0")
    std::string output_dir;               // Output directory for project
};

struct DesignerInstrumentExportPluginProjectResponse {
    CoDesignerSessionState designer_session;
    std::string instrument_id;
    std::string plugin_target;
    std::string plugin_name;
    std::string plugin_id;
    std::string output_dir;
    std::string status;
};

class CoDesignerManager {
public:
    explicit CoDesignerManager(std::shared_ptr<CircuitFacade> circuit_facade)
        : circuit_facade_(circuit_facade) {}

    Result<CoDesignerSessionState> CreateSession(int proto_session_id, const std::string& branch);
    Result<CoDesignerSessionState> GetSession(const std::string& designer_session_id);
    Result<void> UpdateSession(const CoDesignerSessionState& updated);
    Result<void> DestroySession(const std::string& designer_session_id);
    Result<DesignerRetimeResponse> RetimeDesign(const DesignerRetimeRequest& request);
    Result<DesignerRetimeApplyResponse> ApplyRetimeDesign(const DesignerRetimeApplyRequest& request);
    Result<DesignerRetimeOptResponse> OptimizeRetimeDesign(const DesignerRetimeOptRequest& request);

    // Codegen methods
    Result<DesignerCodegenBlockCResponse> CodegenBlockC(const DesignerCodegenBlockCRequest& request);
    Result<DesignerCodegenOscDemoResponse> CodegenOscDemo(const DesignerCodegenOscDemoRequest& request);

    // Global pipelining methods
    Result<DesignerGlobalPipelineResponse> AnalyzeGlobalPipeline(const DesignerGlobalPipelineRequest& request);
    Result<DesignerGlobalPipelineResponse> OptimizeGlobalPipeline(const DesignerGlobalPipelineOptRequest& request);
    Result<DesignerGlobalPipelineApplyResponse> ApplyGlobalPipeline(const DesignerGlobalPipelineApplyRequest& request);

    // Structural synthesis methods
    Result<DesignerStructAnalyzeResponse> AnalyzeStructural(const DesignerStructAnalyzeRequest& request);
    Result<DesignerStructApplyResponse> ApplyStructural(const DesignerStructApplyRequest& request);

    // DSP graph methods
    Result<DesignerDspGraphInspectResponse> InspectDspGraph(const DesignerDspGraphInspectRequest& request);
    Result<DesignerDspRenderOscResponse> RenderDspOsc(const DesignerDspRenderOscRequest& request);

    // Analog model methods
    Result<DesignerAnalogModelInspectResponse> InspectAnalogModel(const DesignerAnalogModelInspectRequest& request);
    Result<DesignerAnalogRenderOscResponse> RenderAnalogOsc(const DesignerAnalogRenderOscRequest& request);

    // Hybrid instrument methods
    Result<DesignerHybridInstrumentResponse> BuildHybridInstrument(const DesignerBuildHybridInstrumentRequest& request);
    Result<DesignerHybridInstrumentResponse> RenderHybridInstrument(const DesignerRenderHybridInstrumentRequest& request);

    // Instrument export methods
    Result<DesignerInstrumentExportCppResponse> ExportInstrumentAsCpp(const DesignerInstrumentExportCppRequest& request);

    // Plugin skeleton export methods
    Result<DesignerInstrumentExportPluginSkeletonResponse> ExportInstrumentAsPluginSkeleton(const DesignerInstrumentExportPluginSkeletonRequest& request);

    // Plugin project export methods
    Result<DesignerInstrumentExportPluginProjectResponse> ExportInstrumentAsPluginProject(const DesignerInstrumentExportPluginProjectRequest& request);

private:
    // Helper method to generate unique designer session IDs
    std::string GenerateDesignerSessionId();

private:
    std::shared_ptr<CircuitFacade> circuit_facade_;
    std::unordered_map<std::string, CoDesignerSessionState> sessions_;
    std::mutex sessions_mutex_;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CoDesigner_h_