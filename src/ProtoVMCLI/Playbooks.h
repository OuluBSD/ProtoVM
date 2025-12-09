#ifndef _ProtoVM_Playbooks_h_
#define _ProtoVM_Playbooks_h_

#include "SessionTypes.h"
#include "CoDesigner.h"  // For CoDesignerSessionState and related types
#include "HlsIr.h"       // For IrModule, IrOptPassKind, IrOptimizationResult
#include "Transformations.h"  // For TransformationPlan
#include "DiffAnalysis.h"     // For BehaviorDiff, IrDiff
#include "BehavioralAnalysis.h"  // For BehaviorDescriptor
#include <string>
#include <vector>

namespace ProtoVMCLI {

// Enumeration of available playbook types
enum class PlaybookKind {
    OptimizeBlockAndReport,
    OptimizeAndApplySafeRefactors,
    // System-level playbooks (Phase 18)
    SystemOptimizeAndReport,
    SystemOptimizeAndApplySafeRefactors,
    // extendable in the future
};

// Configuration struct for parameterizing playbook runs
struct PlaybookConfig {
    PlaybookKind kind;

    std::string designer_session_id;    // existing CoDesigner session
    std::string target;                 // "block" (existing) or "system" (new for Phase 18)
    std::string block_id;               // required when target == "block"

    // System-level parameters (Phase 18):
    std::vector<std::string> block_ids; // explicit list for system-level playbooks
    std::string name_prefix;            // optional prefix filter for system-level playbooks

    std::string baseline_branch;        // branch name for diff comparison
    std::vector<IrOptPassKind> passes;  // IR optimization passes to run

    bool use_optimized_ir;         // hint if analysis/codegen should use optimized IR
    bool apply_refactors;          // whether to actually apply suggested refactors
};

// Per-block result structure for system-level playbooks
struct BlockPlaybookResult {
    std::string block_id;
    BehaviorDescriptor initial_behavior;
    BehaviorDescriptor final_behavior;
    IrModule initial_ir;
    IrModule final_ir;
    IrOptimizationResult optimization;
    std::vector<TransformationPlan> proposed_plans;
    std::vector<std::string> applied_plan_ids;
    int new_circuit_revision = -1;
    BehaviorDiff behavior_diff;
    IrDiff ir_diff;
    CodegenModule codegen;
};

// Result structure returned by playbook execution
struct PlaybookResult {
    PlaybookKind kind;
    PlaybookConfig config;

    // Snapshots of context
    CoDesignerSessionState designer_session;

    // Optional analysis outputs (for block-level playbooks)
    BehaviorDescriptor initial_behavior;
    BehaviorDescriptor final_behavior;

    IrModule initial_ir;
    IrModule final_ir;

    // Optimization summary (for block-level playbooks)
    IrOptimizationResult optimization; // may be empty/default if no optimization

    // Refactor plans and applied changes (for block-level playbooks)
    std::vector<TransformationPlan> proposed_plans;
    std::vector<std::string> applied_plan_ids;
    int new_circuit_revision = -1; // -1 if no changes applied

    // Diffs against baseline (for block-level playbooks)
    BehaviorDiff behavior_diff;
    IrDiff ir_diff;

    // Codegen result for final state (for block-level playbooks)
    CodegenModule codegen;

    // System-level results (for system-level playbooks)
    std::vector<BlockPlaybookResult> system_block_results;

    // Aggregated system-level metrics
    int total_blocks = 0;
    int blocks_with_changes = 0;
    int total_applied_plans = 0;
};

class PlaybookEngine {
public:
    // Main method to run a playbook according to its kind and configuration
    static Result<PlaybookResult> RunPlaybook(
        const PlaybookConfig& config,
        CoDesignerManager& designer_manager,
        SessionStore& session_store,
        const std::string& workspace_dir
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_Playbooks_h_