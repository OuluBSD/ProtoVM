#ifndef _ProtoVM_GlobalPipelining_h_
#define _ProtoVM_GlobalPipelining_h_

#include "GlobalPipeline.h"
#include "RetimingModel.h"
#include "RetimingOpt.h"
#include "RetimingTransform.h"
#include "SessionTypes.h"
#include "SessionStore.h"
#include <ProtoVM/ProtoVM.h>
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class GlobalPipeliningStrategyKind {
    BalanceStages,     // try to equalize max_comb_depth across stages
    ReduceCriticalPath // reduce max end-to-end depth/latency within constraints
};

struct GlobalPipeliningObjective : public Upp::Moveable<GlobalPipeliningObjective>
{
    GlobalPipeliningStrategyKind kind;

    // Optional constraints:
    int target_stage_count = -1;      // desired number of stages (latency constraint)
    int target_max_depth = -1;        // desired max per-stage depth
    int max_extra_registers = -1;     // budget for added registers
    int max_total_moves = -1;         // total retiming moves allowed
};

struct GlobalPipeliningStep : public Upp::Moveable<GlobalPipeliningStep> {
    String block_id;
    String retiming_plan_id;   // ID of a local RetimingPlan in that block
};

struct GlobalPipeliningPlan : public Upp::Moveable<GlobalPipeliningPlan> {
    GlobalPipeliningPlan()
        : estimated_global_depth_before(-1)
        , estimated_global_depth_after(-1)
        , respects_cdc_fences(true) {}
    GlobalPipeliningPlan(const GlobalPipeliningPlan& p)
        : Upp::Moveable<GlobalPipeliningPlan>()
        , id(p.id)
        , subsystem_id(p.subsystem_id)
        , objective(p.objective)
        , estimated_global_depth_before(p.estimated_global_depth_before)
        , estimated_global_depth_after(p.estimated_global_depth_after)
        , respects_cdc_fences(p.respects_cdc_fences)
    {
        for (int i = 0; i < p.block_ids.GetCount(); i++)
            block_ids.Add(p.block_ids[i]);
        for (const auto& st : p.steps)
            steps.Add(st);
    }

    String id;                     // e.g. "GPP_ALU_PIPE_BALANCE_1"
    String subsystem_id;
    Vector<String> block_ids;

    GlobalPipeliningObjective objective;

    // The local retiming plans to apply, in order.
    Vector<GlobalPipeliningStep> steps;

    // Optional summary:
    int estimated_global_depth_before = -1;
    int estimated_global_depth_after = -1;
    bool respects_cdc_fences = true;
};

class GlobalPipeliningEngine {
public:
    // Derive a set of global pipelining plans from local retiming options.
    static Result<Vector<GlobalPipeliningPlan>> ProposeGlobalPipeliningPlans(
        const String& subsystem_id,
        const Vector<String>& block_ids,
        const GlobalPipeliningObjective& objective,
        const GlobalPipelineMap& global_pipeline,
        const Vector<RetimingOptimizationResult>& per_block_opt_results
    );

    // Apply a chosen global plan by delegating to per-block retiming application.
    static Result<GlobalPipeliningPlan> ApplyGlobalPipeliningPlanInBranch(
        const GlobalPipeliningPlan& plan,
        const RetimingApplicationOptions& default_app_options,
        ISessionStore& session_store,
        const SessionMetadata& session,
        const String& session_dir,
        const String& branch_name
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_GlobalPipelining_h_