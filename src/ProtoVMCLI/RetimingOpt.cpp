#include "RetimingOpt.h"
#include "CircuitFacade.h"
#include <algorithm>
#include <cmath>

namespace ProtoVMCLI {

static double CalculateCost(const RetimingPlanScore& score, const RetimingObjective& objective) {
    double cost = 0.0;

    switch (objective.kind) {
        case RetimingObjectiveKind::MinimizeMaxDepth: {
            // Base cost is the resulting depth
            cost = static_cast<double>(score.estimated_max_depth_after);
            
            // Penalty for suspicious moves
            cost += score.suspicious_move_count * 10.0;
            
            // Large penalty for forbidden moves
            cost += score.forbidden_move_count * 1000.0;
            
            break;
        }
        
        case RetimingObjectiveKind::MinimizeDepthWithBudget: {
            // Base cost is the resulting depth
            cost = static_cast<double>(score.estimated_max_depth_after);
            
            // Check constraints and apply penalties if violated
            if (objective.target_max_depth > 0 && score.estimated_max_depth_after > objective.target_max_depth) {
                cost += 1000.0; // Large penalty for exceeding target depth
            }
            
            if (objective.max_moves > 0 && score.applied_move_count > objective.max_moves) {
                cost += 500.0; // Penalty for exceeding move budget
            }
            
            if (objective.max_extra_registers > 0 && 
                score.estimated_register_count_after > score.estimated_register_count_before + objective.max_extra_registers) {
                cost += 500.0; // Penalty for exceeding register budget
            }
            
            // Penalty for suspicious moves
            cost += score.suspicious_move_count * 10.0;
            
            // Large penalty for forbidden moves
            cost += score.forbidden_move_count * 1000.0;
            
            break;
        }
        
        case RetimingObjectiveKind::BalanceStages: {
            // For balancing stages, we'd typically need stage-specific depth information
            // Since this is a simplified implementation, we'll use the max depth as primary factor
            // with a secondary cost factor for overall balance
            cost = static_cast<double>(score.estimated_max_depth_after);
            
            // Penalty for suspicious moves
            cost += score.suspicious_move_count * 10.0;
            
            // Large penalty for forbidden moves
            cost += score.forbidden_move_count * 1000.0;
            
            break;
        }
    }

    return cost;
}

static bool CheckObjectiveConstraints(const RetimingPlanScore& score, const RetimingObjective& objective) {
    bool meets_constraints = true;

    switch (objective.kind) {
        case RetimingObjectiveKind::MinimizeMaxDepth: {
            // This objective kind doesn't have explicit constraints beyond being safe
            meets_constraints = (score.forbidden_move_count == 0);
            break;
        }
        
        case RetimingObjectiveKind::MinimizeDepthWithBudget: {
            // Check all constrained values
            if (objective.target_max_depth > 0 && score.estimated_max_depth_after > objective.target_max_depth) {
                meets_constraints = false;
            }
            
            if (objective.max_moves > 0 && score.applied_move_count > objective.max_moves) {
                meets_constraints = false;
            }
            
            if (objective.max_extra_registers > 0 && 
                score.estimated_register_count_after > score.estimated_register_count_before + objective.max_extra_registers) {
                meets_constraints = false;
            }
            
            // Also, require that no forbidden moves are present
            if (score.forbidden_move_count > 0) {
                meets_constraints = false;
            }
            
            break;
        }
        
        case RetimingObjectiveKind::BalanceStages: {
            // Similar to minimize depth but with focus on balance
            meets_constraints = (score.forbidden_move_count == 0);
            break;
        }
    }

    return meets_constraints;
}

Result<RetimingOptimizationResult> RetimingOptimizer::EvaluateRetimingPlans(
    const String& target_id,
    const Vector<RetimingPlan>& plans,
    const RetimingObjective& objective,
    const PipelineMap* pipeline,
    const TimingAnalysis* timing,
    const CdcReport* cdc_report
) {
    RetimingOptimizationResult result;
    result.target_id = target_id;
    result.objective = objective;

    for (const auto& plan : plans) {
        RetimingPlanScore score;
        score.plan_id = plan.id;
        score.estimated_max_depth_before = plan.estimated_max_depth_before;
        score.estimated_max_depth_after = plan.estimated_max_depth_after;
        score.respects_cdc_fences = plan.respects_cdc_fences;

        // Count move types
        for (const auto& move : plan.moves) {
            switch (move.safety) {
                case RetimingMoveSafety::SafeIntraDomain:
                    score.safe_move_count++;
                    score.applied_move_count++; // Assuming safe moves would be applied
                    break;
                case RetimingMoveSafety::Suspicious:
                    score.suspicious_move_count++;
                    break;
                case RetimingMoveSafety::Forbidden:
                    score.forbidden_move_count++;
                    break;
            }
        }

        // Estimate register count change (this is a simplification, a more accurate
        // estimate would analyze the specific register additions/removals)
        // For now, we'll leave these as -1 since we don't have that detailed information
        score.estimated_register_count_before = -1;
        score.estimated_register_count_after = -1;

        // Calculate cost and check if objective is met
        score.cost = CalculateCost(score, objective);
        score.meets_objective = CheckObjectiveConstraints(score, objective);

        result.plan_scores.Add(score);
    }

    // Sort plans by cost (ascending)
    std::sort(result.plan_scores.Begin(), result.plan_scores.End(),
        [](const RetimingPlanScore& a, const RetimingPlanScore& b) {
            return a.cost < b.cost;
        });

    // Find the best plan (with lowest cost among those that meet the objective)
    for (const auto& score : result.plan_scores) {
        if (score.meets_objective) {
            result.best_plan_id = score.plan_id;
            break;
        }
    }

    return Result<RetimingOptimizationResult>::Success(result);
}

Result<RetimingOptimizationResult> RetimingOptimizer::EvaluateAndApplyBestPlanInBranch(
    const String& target_id,
    const Vector<RetimingPlan>& plans,
    const RetimingObjective& objective,
    const RetimingApplicationOptions& app_options,
    SessionStore& session_store,
    const SessionMetadata& session,
    const String& session_dir,
    const String& branch_name
) {
    // First, evaluate the plans
    auto eval_result = EvaluateRetimingPlans(target_id, plans, objective, nullptr, nullptr, nullptr);
    if (!eval_result.ok) {
        return Result<RetimingOptimizationResult>::Error(eval_result.error);
    }

    auto result = eval_result.value;

    // Find the best plan that meets the objective
    RetimingPlan* best_plan = nullptr;
    for (auto& plan : const_cast<Vector<RetimingPlan>&>(plans)) {  // Safe const_cast as we're looking for a match
        if (plan.id == result.best_plan_id) {
            best_plan = &plan;
            break;
        }
    }

    if (!best_plan || result.best_plan_id.IsEmpty()) {
        // No plan meets the objective, return result without applying anything
        result.applied = false;
        return Result<RetimingOptimizationResult>::Success(result);
    }

    // Apply the best plan
    auto apply_result = RetimingTransform::ApplyRetimingPlanInBranch(
        *best_plan,
        app_options,
        session_store,
        session,
        session_dir,
        branch_name
    );

    if (!apply_result.ok) {
        return Result<RetimingOptimizationResult>::Error(apply_result.error);
    }

    result.applied = true;
    result.application_result = apply_result.value;

    return Result<RetimingOptimizationResult>::Success(result);
}

} // namespace ProtoVMCLI