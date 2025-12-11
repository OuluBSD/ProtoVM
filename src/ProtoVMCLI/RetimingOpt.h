#ifndef _ProtoVM_RetimingOpt_h_
#define _ProtoVM_RetimingOpt_h_

#include "RetimingModel.h"
#include "PipelineModel.h"
#include "TimingAnalysis.h"
#include "CdcModel.h"
#include "RetimingTransform.h"
#include "SessionTypes.h"
#include "SessionStore.h"
#include <ProtoVM/ProtoVM.h>
#include <string>
#include <vector>

namespace ProtoVMCLI {

// Objective types for retiming optimization
enum class RetimingObjectiveKind {
    MinimizeMaxDepth,        // primary: reduce critical combinational depth
    MinimizeDepthWithBudget, // reduce depth, but respect register/move budgets
    BalanceStages            // improve stage balance (spread depth more evenly)
};

// Configuration for optimization objectives
struct RetimingObjective {
    RetimingObjectiveKind kind;

    // Optional constraints / budgets:
    int max_extra_registers = -1; // <=0 means no added registers allowed or no limit depending on kind
    int max_moves = -1;           // limit number of moves applied
    int target_max_depth = -1;    // desired upper bound on critical depth (optional)
};

// Score for a single retiming plan
struct RetimingPlanScore {
    String plan_id;

    int estimated_max_depth_before = -1;
    int estimated_max_depth_after = -1;

    int applied_move_count = 0;
    int safe_move_count = 0;
    int suspicious_move_count = 0;
    int forbidden_move_count = 0; // normally 0 if plan is valid

    int estimated_register_count_before = -1; // optional, may be approximate
    int estimated_register_count_after = -1;

    bool respects_cdc_fences = true;
    bool meets_objective = false; // whether this plan satisfies the objective under given constraints

    // A simple scalar cost for ranking (lower is better).
    double cost = 0.0;
};

// Result of retiming optimization
struct RetimingOptimizationResult {
    String target_id; // block or subsystem id

    RetimingObjective objective;

    // Evaluated plans and their scores, sorted by cost ascending.
    Vector<RetimingPlanScore> plan_scores;

    // ID of the recommended plan (if any).
    String best_plan_id;

    // Optional, if auto-apply was performed:
    bool applied = false;
    RetimingApplicationResult application_result; // from RetimingTransform
};

class RetimingOptimizer {
public:
    // Evaluate retiming plans for a block/subsystem without applying them.
    static Result<RetimingOptimizationResult> EvaluateRetimingPlans(
        const String& target_id,
        const Vector<RetimingPlan>& plans,
        const RetimingObjective& objective,
        const PipelineMap* pipeline,         // optional
        const TimingAnalysis* timing,        // optional
        const CdcReport* cdc_report          // optional
    );

    // Optionally: choose best plan and auto-apply.
    static Result<RetimingOptimizationResult> EvaluateAndApplyBestPlanInBranch(
        const String& target_id,
        const Vector<RetimingPlan>& plans,
        const RetimingObjective& objective,
        const RetimingApplicationOptions& app_options,
        ISessionStore& session_store,
        const SessionMetadata& session,
        const String& session_dir,
        const String& branch_name
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_RetimingOpt_h_