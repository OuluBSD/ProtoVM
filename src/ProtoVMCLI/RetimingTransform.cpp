#include "RetimingTransform.h"
#include "CircuitFacade.h"
#include "EventLogger.h"
#include "JsonIO.h"
#include <algorithm>
#include <set>

namespace ProtoVMCLI {

Result<TransformationPlan> RetimingTransform::BuildTransformationPlanForRetiming(
    const RetimingPlan& retiming_plan,
    const RetimingApplicationOptions& options) {
    
    // Create transformation plan based on the retiming plan
    TransformationPlan transformation_plan;
    transformation_plan.id = String("TX_") + retiming_plan.id; // Transform ID based on retiming plan ID
    transformation_plan.kind = TransformationKind::Unknown;  // Need to define a specific kind for retiming
    
    // Set target
    transformation_plan.target.subject_id = retiming_plan.target_id;
    transformation_plan.target.subject_kind = "Block";  // Assuming block-level for now
    
    // Add guarantees about what is preserved
    transformation_plan.guarantees.Add(PreservationLevel::IOContractPreserved);
    transformation_plan.guarantees.Add(PreservationLevel::DependencyPatternPreserved);
    
    // Filter moves based on safety and options
    Vector<RetimingMove> eligible_moves;
    for (const auto& move : retiming_plan.moves) {
        bool is_eligible = false;
        
        if (move.safety == RetimingMoveSafety::SafeIntraDomain) {
            is_eligible = true;
        } else if (options.allow_suspicious_moves && move.safety == RetimingMoveSafety::Suspicious) {
            is_eligible = true;
        }
        
        if (is_eligible && options.apply_only_safe_moves && move.safety != RetimingMoveSafety::SafeIntraDomain) {
            is_eligible = false;
        }
        
        if (is_eligible) {
            eligible_moves.Add(move);
        }
    }
    
    // Apply max moves limit if specified
    if (options.max_moves >= 0 && eligible_moves.GetCount() > options.max_moves) {
        eligible_moves.SetCount(options.max_moves);
    }
    
    // Generate transformation steps for each eligible move
    for (const auto& move : eligible_moves) {
        TransformationStep step;
        step.description = String("Retiming move: ") + move.move_id + 
                          " from " + move.src_reg_id + 
                          " to direction " + 
                          (move.direction == RetimingMoveDirection::Forward ? "forward" : "backward");
        
        transformation_plan.steps.Add(step);
    }
    
    return Result<TransformationPlan>::MakeOk(transformation_plan);
}

Result<RetimingApplicationResult> RetimingTransform::ApplyRetimingPlanInBranch(
    const RetimingPlan& retiming_plan,
    const RetimingApplicationOptions& options,
    SessionStore& session_store,
    const SessionMetadata& session,
    const String& session_dir,
    const String& branch_name) {
    
    // Build the transformation plan from the retiming plan
    auto build_result = BuildTransformationPlanForRetiming(retiming_plan, options);
    if (!build_result.ok) {
        return Result<RetimingApplicationResult>::MakeError(
            build_result.error_code, build_result.error_message);
    }
    
    TransformationPlan transformation_plan = build_result.data;
    
    // Create CircuitFacade to apply the transformation
    CircuitFacade facade;
    
    // Create a copy of the session metadata for modification
    SessionMetadata modified_session = session;
    
    // Apply the transformation using the existing transformation engine
    auto apply_result = facade.ApplyTransformationPlan(
        modified_session, 
        std::string(session_dir), 
        std::string(branch_name), 
        transformation_plan, 
        "retiming-engine");
    
    if (!apply_result.ok) {
        return Result<RetimingApplicationResult>::MakeError(
            apply_result.error_code, apply_result.error_message);
    }
    
    // Construct the result object
    RetimingApplicationResult result;
    result.plan_id = retiming_plan.id;
    result.target_id = retiming_plan.target_id;
    result.estimated_max_depth_before = retiming_plan.estimated_max_depth_before;
    result.estimated_max_depth_after = retiming_plan.estimated_max_depth_after;  // Simplified - in real implementation, would re-analyze
    result.all_moves_safe = true;  // Will update based on actual moves applied
    
    // Need to identify the specific moves that were applied
    // For now, we'll assume all eligible moves from the retiming plan were applied
    for (const auto& move : retiming_plan.moves) {
        if (move.safety == RetimingMoveSafety::SafeIntraDomain) {
            result.applied_move_ids.Add(move.move_id);
        } else if (options.allow_suspicious_moves && move.safety == RetimingMoveSafety::Suspicious) {
            result.applied_move_ids.Add(move.move_id);
            result.all_moves_safe = false;
        } else if (move.safety != RetimingMoveSafety::SafeIntraDomain) {
            result.skipped_move_ids.Add(move.move_id);
            result.all_moves_safe = false;
        }
    }
    
    // Apply max moves limit if specified
    if (options.max_moves >= 0 && result.applied_move_ids.GetCount() > options.max_moves) {
        // Move excess to skipped list
        int excess_count = result.applied_move_ids.GetCount() - options.max_moves;
        for (int i = 0; i < excess_count; i++) {
            int index = result.applied_move_ids.GetCount() - 1 - i;
            result.skipped_move_ids.Add(result.applied_move_ids[index]);
        }
        result.applied_move_ids.SetCount(options.max_moves);
    }
    
    // Update the session in store with the modified metadata
    auto save_result = session_store.SaveSession(modified_session);
    
    // Get the new circuit revision (we'll assume it's updated by the ApplyTransformationPlan call)
    // In a real implementation, this would need to come from the result of applying the transformation
    result.new_circuit_revision = modified_session.circuit_revision;  // Would need to get this from ApplyTransformationPlan result
    
    return Result<RetimingApplicationResult>::MakeOk(result);
}

} // namespace ProtoVMCLI