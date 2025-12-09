#include "Scheduling.h"
#include "HlsIr.h"
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <queue>

namespace ProtoVMCLI {

// Helper function to compute depth from an expression graph
static int ComputeDepthForExpression(const IrExpr& expr, const std::map<std::string, int>& depths) {
    int max_input_depth = 0;
    
    for (const auto& arg : expr.args) {
        if (!arg.is_literal && depths.count(arg.name) > 0) {
            max_input_depth = std::max(max_input_depth, depths.at(arg.name));
        }
    }
    
    return max_input_depth + 1; // Add 1 for this operation
}

Result<std::vector<int>> SchedulingEngine::ComputeTimingDepths(
    const IrModule& ir,
    const TimingAnalysis* timing,          // optional pointer
    const CircuitGraph* graph             // optional pointer
) {
    std::vector<int> depths(ir.comb_assigns.size(), 0);
    
    // If timing analysis is available and useful, use it
    if (timing != nullptr) {
        // For now, we'll implement a basic algorithm since we don't have full timing analysis integration
        // This is a placeholder that will be extended with real timing analysis later
    }
    
    // Build a depth map for all values
    std::map<std::string, int> value_depths;
    
    // Initialize input depths to 0
    for (const auto& input : ir.inputs) {
        value_depths[input.name] = 0;
    }
    
    // Process combinational assignments in topological order
    std::set<std::string> processed;
    bool changed = true;
    
    while (changed) {
        changed = false;
        
        for (size_t i = 0; i < ir.comb_assigns.size(); ++i) {
            const auto& expr = ir.comb_assigns[i];
            const std::string& target_name = expr.target.name;
            
            if (processed.count(target_name)) {
                continue;
            }
            
            // Check if all arguments have been processed
            bool ready = true;
            for (const auto& arg : expr.args) {
                if (!arg.is_literal && value_depths.count(arg.name) == 0) {
                    ready = false;
                    break;
                }
            }
            
            if (ready) {
                int depth = ComputeDepthForExpression(expr, value_depths);
                value_depths[target_name] = depth;
                depths[i] = depth;
                processed.insert(target_name);
                changed = true;
            }
        }
    }
    
    return Result<std::vector<int>>::Ok(depths);
}

Result<std::vector<StageIndex>> SchedulingEngine::AssignStages(
    const std::vector<int>& depths,
    int num_stages,
    const SchedulingConfig& config
) {
    std::vector<StageIndex> stages(depths.size());
    int max_depth = 0;
    
    if (!depths.empty()) {
        max_depth = *std::max_element(depths.begin(), depths.end());
    }
    
    if (max_depth == 0) {
        // All operations in stage 0
        std::fill(stages.begin(), stages.end(), 0);
        return Result<std::vector<StageIndex>>::Ok(stages);
    }
    
    for (size_t i = 0; i < depths.size(); ++i) {
        StageIndex stage = 0;
        
        switch (config.strategy) {
            case SchedulingStrategy::SingleStage:
                stage = 0;
                break;
                
            case SchedulingStrategy::DepthBalancedStages: {
                // Calculate stage based on depth and number of stages
                stage = (depths[i] * num_stages) / (max_depth + 1);
                stage = std::min(stage, num_stages - 1); // Ensure we don't exceed num_stages - 1
                break;
            }
                
            case SchedulingStrategy::FixedStageCount: {
                // Calculate stage based on depth and requested number of stages
                stage = (depths[i] * config.requested_stages) / (max_depth + 1);
                stage = std::min(stage, config.requested_stages - 1); // Ensure we don't exceed requested_stages - 1
                break;
            }
        }
        
        stages[i] = stage;
    }
    
    return Result<std::vector<StageIndex>>::Ok(stages);
}

Result<ScheduledModule> SchedulingEngine::BuildSchedule(
    const IrModule& ir,
    const TimingAnalysis* timing,          // optional pointer
    const CircuitGraph* graph,            // optional pointer
    const SchedulingConfig& config
) {
    // Compute timing depths for all expressions
    auto depth_result = ComputeTimingDepths(ir, timing, graph);
    if (!depth_result.ok) {
        return Result<ScheduledModule>::Error(depth_result.error_code, depth_result.error_message);
    }
    
    const std::vector<int>& depths = depth_result.data;
    
    // Determine number of stages based on strategy
    int num_stages = 1;
    if (config.strategy == SchedulingStrategy::DepthBalancedStages) {
        int max_depth = 0;
        if (!depths.empty()) {
            max_depth = *std::max_element(depths.begin(), depths.end());
        }
        // Use requested_stages as upper bound, but default to max_depth if not specified
        num_stages = std::min((config.requested_stages > 0) ? config.requested_stages : max_depth + 1, max_depth + 1);
        num_stages = std::max(1, num_stages); // Ensure at least 1 stage
    } else if (config.strategy == SchedulingStrategy::FixedStageCount) {
        num_stages = std::max(1, config.requested_stages);
    } else { // SingleStage
        num_stages = 1;
    }
    
    // Assign stages to expressions
    auto stage_result = AssignStages(depths, num_stages, config);
    if (!stage_result.ok) {
        return Result<ScheduledModule>::Error(stage_result.error_code, stage_result.error_message);
    }
    
    const std::vector<StageIndex>& stages = stage_result.data;
    
    // Create scheduled expressions
    std::vector<ScheduledExpr> scheduled_comb_ops;
    for (size_t i = 0; i < ir.comb_assigns.size(); ++i) {
        scheduled_comb_ops.emplace_back(ir.comb_assigns[i], stages[i]);
    }
    
    // Create scheduled register assignments
    std::vector<ScheduledRegAssign> scheduled_reg_ops;
    for (const auto& reg_assign : ir.reg_assigns) {
        // For now, place all register assignments in the last stage
        StageIndex reg_stage = num_stages - 1;
        scheduled_reg_ops.emplace_back(reg_assign, reg_stage);
    }
    
    // Create the scheduled module
    ScheduledModule scheduled_module;
    scheduled_module.id = ir.id;
    scheduled_module.inputs = ir.inputs;
    scheduled_module.outputs = ir.outputs;
    scheduled_module.num_stages = num_stages;
    scheduled_module.comb_ops = scheduled_comb_ops;
    scheduled_module.reg_ops = scheduled_reg_ops;
    
    return Result<ScheduledModule>::Ok(scheduled_module);
}

} // namespace ProtoVMCLI