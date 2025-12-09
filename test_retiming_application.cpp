#include "RetimingTransform.h"
#include "RetimingModel.h"
#include "Transformations.h"
#include "SessionTypes.h"
#include "SessionStore.h"
#include <iostream>
#include <cassert>

using namespace ProtoVMCLI;

int main() {
    std::cout << "Testing Retiming Application Layer..." << std::endl;

    // Test 1: Create a simple retiming plan with moves
    RetimingPlan plan;
    plan.id = "TEST_PLAN_1";
    plan.target_id = "TEST_BLOCK";
    plan.estimated_max_depth_before = 10;
    plan.estimated_max_depth_after = 6;
    plan.respects_cdc_fences = true;

    // Add a safe move
    RetimingMove safe_move;
    safe_move.move_id = "RTM_0001";
    safe_move.src_reg_id = "REG_A";
    safe_move.dst_reg_id = "REG_B";
    safe_move.direction = RetimingMoveDirection::Forward;
    safe_move.domain_id = 1;
    safe_move.src_stage_index = 2;
    safe_move.dst_stage_index = 3;
    safe_move.before_comb_depth = 8;
    safe_move.after_comb_depth_est = 5;
    safe_move.safety = RetimingMoveSafety::SafeIntraDomain;
    safe_move.safety_reason = "Intra-domain, no CDC crossings";
    
    plan.moves.Add(safe_move);

    // Add a suspicious move
    RetimingMove suspicious_move = safe_move;
    suspicious_move.move_id = "RTM_0002";
    suspicious_move.safety = RetimingMoveSafety::Suspicious;
    suspicious_move.safety_reason = "Heuristically possible issues";
    
    plan.moves.Add(suspicious_move);

    // Test 2: Create application options
    RetimingApplicationOptions options;
    options.apply_only_safe_moves = true;  // Should only apply safe moves
    options.allow_suspicious_moves = false;
    options.max_moves = -1;  // No limit

    std::cout << "Created test plan with " << plan.moves.GetCount() << " moves" << std::endl;

    // Test 3: Build transformation plan
    auto transform_result = RetimingTransform::BuildTransformationPlanForRetiming(plan, options);
    if (!transform_result.ok) {
        std::cout << "ERROR: Failed to build transformation plan: " << transform_result.error_message << std::endl;
        return 1;
    }

    std::cout << "Successfully built transformation plan with " 
              << transform_result.data.steps.GetCount() << " steps" << std::endl;

    // Test 4: Test with options that allow suspicious moves
    options.apply_only_safe_moves = false;
    options.allow_suspicious_moves = true;

    auto transform_result2 = RetimingTransform::BuildTransformationPlanForRetiming(plan, options);
    if (!transform_result2.ok) {
        std::cout << "ERROR: Failed to build transformation plan with suspicious moves: " 
                  << transform_result2.error_message << std::endl;
        return 1;
    }

    std::cout << "Successfully built transformation plan with suspicious moves, " 
              << transform_result2.data.steps.GetCount() << " steps" << std::endl;

    // Test 5: Test with limited moves
    options.max_moves = 1;  // Should limit to 1 move
    auto transform_result3 = RetimingTransform::BuildTransformationPlanForRetiming(plan, options);
    if (!transform_result3.ok) {
        std::cout << "ERROR: Failed to build transformation plan with move limit: " 
                  << transform_result3.error_message << std::endl;
        return 1;
    }

    std::cout << "Successfully built transformation plan with max moves limit, " 
              << transform_result3.data.steps.GetCount() << " steps" << std::endl;

    std::cout << "All tests passed!" << std::endl;
    return 0;
}