#ifndef _ProtoVM_CoDesigner_h_
#define _ProtoVM_CoDesigner_h_

#include "SessionTypes.h"
#include "CircuitFacade.h"
#include "HlsIr.h"  // For IrModule
#include "HlsIrInference.h"  // For IR generation
#include "Transformations.h"  // For transformation plans
#include "DiffAnalysis.h"  // For diff operations
#include "Codegen.h"  // For code generation
#include "BehavioralAnalysis.h"  // For behavioral analysis
#include "RetimingModel.h"  // For retiming analysis
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

struct DesignerAnalyzeResponse {
    CoDesignerSessionState designer_session;
    std::optional<struct BlockAnalysisResult> block;  // Contains behavior and IR if block_id is set
    std::optional<struct NodeAnalysisResult> node;    // Contains behavior and IR if node_id is set
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