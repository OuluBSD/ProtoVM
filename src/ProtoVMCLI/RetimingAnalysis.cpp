#include "RetimingAnalysis.h"
#include "CircuitGraph.h"
#include "CircuitGraphQueries.h"
#include <algorithm>
#include <set>

namespace ProtoVMCLI {

Result<Vector<RetimingPlan>> RetimingAnalysis::AnalyzeRetimingForBlock(
    const PipelineMap& pipeline,
    const CdcReport& cdc_report,
    const TimingAnalysis* timing,
    const ScheduledModule* scheduled_ir
) {
    return IdentifyCandidatePaths(pipeline, cdc_report, timing, scheduled_ir, pipeline.id);
}

Result<Vector<RetimingPlan>> RetimingAnalysis::AnalyzeRetimingForSubsystem(
    const PipelineMap& pipeline,
    const CdcReport& cdc_report,
    const TimingAnalysis* timing,
    const ScheduledModule* scheduled_ir
) {
    return IdentifyCandidatePaths(pipeline, cdc_report, timing, scheduled_ir, pipeline.id);
}

Result<Vector<RetimingPlan>> RetimingAnalysis::IdentifyCandidatePaths(
    const PipelineMap& pipeline,
    const CdcReport& cdc_report,
    const TimingAnalysis* timing,
    const ScheduledModule* scheduled_ir,
    const String& target_id
) {
    Vector<RetimingPlan> plans;
    
    // Identify registers that are part of CDC crossings - these are "anchored"
    Vector<String> cdc_anchored_regs;
    for (const auto& crossing : cdc_report.crossings) {
        cdc_anchored_regs.Add(crossing.src.reg_id);
        cdc_anchored_regs.Add(crossing.dst.reg_id);
    }

    // Process each register-to-register path for potential retiming opportunities
    for (const auto& path : pipeline.reg_paths) {
        // Only consider paths within the same domain (intra-domain)
        if (path.crosses_clock_domain) {
            continue;
        }

        // For MVP, consider paths with significant combinatorial depth (> threshold)
        if (path.comb_depth_estimate > 4) {
            auto moves_result = GenerateMovesForPath(path, pipeline, cdc_report, scheduled_ir);
            if (!moves_result.ok()) {
                continue; // Skip if there was an error generating moves
            }
            
            Vector<RetimingMove> moves = moves_result.value();
            
            if (!moves.IsEmpty()) {
                // Create a plan for this path
                RetimingPlan plan;
                plan.id = "RTP_" + target_id + "_" + String(plan.id.GetLength()); // Simple plan ID generation
                plan.target_id = target_id;
                plan.description = "Retiming plan for path from " + path.src_reg_id + " to " + path.dst_reg_id;
                plan.moves = moves;
                
                // Calculate aggregate metrics
                plan.estimated_max_depth_before = path.comb_depth_estimate;
                int min_after_depth = path.comb_depth_estimate;
                for (const auto& move : moves) {
                    if (move.after_comb_depth_est < min_after_depth) {
                        min_after_depth = move.after_comb_depth_est;
                    }
                }
                plan.estimated_max_depth_after = min_after_depth;
                
                // Check if the plan respects CDC fences (no moves that violate CDC)
                plan.respects_cdc_fences = true;
                for (const auto& move : moves) {
                    if (move.safety == RetimingMoveSafety::Forbidden) {
                        plan.respects_cdc_fences = false;
                        break;
                    }
                }
                
                plans.Add(plan);
            }
        }
    }

    return Result<Vector<RetimingPlan>>::Success(plans);
}

Result<Vector<RetimingMove>> RetimingAnalysis::GenerateMovesForPath(
    const RegToRegPathInfo& path,
    const PipelineMap& pipeline,
    const CdcReport& cdc_report,
    const ScheduledModule* scheduled_ir
) {
    Vector<RetimingMove> moves;
    
    // Identify registers that are part of CDC crossings - these cannot be moved
    Vector<String> cdc_anchored_regs;
    for (const auto& crossing : cdc_report.crossings) {
        cdc_anchored_regs.Add(crossing.src.reg_id);
        cdc_anchored_regs.Add(crossing.dst.reg_id);
    }
    
    // Check if this path involves any CDC-anchored registers
    bool involves_cdc_anchored_reg = (cdc_anchored_regs.Find(path.src_reg_id) >= 0 || 
                                      cdc_anchored_regs.Find(path.dst_reg_id) >= 0);

    if (involves_cdc_anchored_reg) {
        // If either register is part of a CDC crossing, this path cannot be retimed safely
        return Result<Vector<RetimingMove>>::Success(moves);
    }

    // Create a forward move (moving register closer to the output)
    RetimingMove forward_move;
    forward_move.move_id = "RTM_FWD_" + String(moves.GetCount() + 1); // Simple move ID generation
    forward_move.src_reg_id = path.src_reg_id;
    forward_move.dst_reg_id = path.dst_reg_id;
    forward_move.direction = RetimingMoveDirection::Forward;
    forward_move.domain_id = path.domain_id;
    forward_move.before_comb_depth = path.comb_depth_estimate;
    forward_move.after_comb_depth_est = EstimateDepthAfterMove(path.comb_depth_estimate, RetimingMoveDirection::Forward);
    
    // Find stage indices for the registers
    int src_stage_idx = -1, dst_stage_idx = -1;
    for (int i = 0; i < pipeline.stages.size(); ++i) {
        const auto& stage = pipeline.stages[i];
        if (stage.registers_out.Find(path.src_reg_id) >= 0) {
            src_stage_idx = i;
        }
        if (stage.registers_in.Find(path.dst_reg_id) >= 0) {
            dst_stage_idx = i;
        }
    }
    
    forward_move.src_stage_index = src_stage_idx;
    forward_move.dst_stage_index = dst_stage_idx;
    
    // Determine safety based on CDC considerations
    forward_move.safety = DetermineSafety(path, cdc_report, cdc_anchored_regs);
    if (forward_move.safety == RetimingMoveSafety::SafeIntraDomain) {
        forward_move.safety_reason = "Intra-domain, no CDC crossings, internal path";
    } else if (forward_move.safety == RetimingMoveSafety::Suspicious) {
        forward_move.safety_reason = "Heuristically suspicious; requires review";
    } else {
        forward_move.safety_reason = "Forbidden due to CDC concerns or other constraints";
    }
    
    // For MVP, we'll add the affected operations based on pipeline stages between src and dst
    if (scheduled_ir) {
        // Attempt to identify operations between the stages
        int start_stage = std::min(src_stage_idx, dst_stage_idx);
        int end_stage = std::max(src_stage_idx, dst_stage_idx);
        
        for (int stage_idx = start_stage + 1; stage_idx < end_stage; ++stage_idx) {
            if (stage_idx < pipeline.stages.size()) {
                const auto& stage = pipeline.stages[stage_idx];
                // This is a simplified approach - in a real implementation, we'd need to map
                // from the pipeline stage to actual scheduled operations in the scheduled_ir
            }
        }
    }
    
    moves.Add(forward_move);

    // Create a backward move (moving register closer to the input) - only if stage indices allow
    if (dst_stage_idx > src_stage_idx) {
        RetimingMove backward_move = forward_move;
        backward_move.move_id = "RTM_BWD_" + String(moves.GetCount() + 1);
        backward_move.direction = RetimingMoveDirection::Backward;
        backward_move.after_comb_depth_est = EstimateDepthAfterMove(path.comb_depth_estimate, RetimingMoveDirection::Backward);
        
        if (backward_move.safety == RetimingMoveSafety::SafeIntraDomain) {
            backward_move.safety_reason = "Intra-domain, no CDC crossings, internal path";
        }
        
        moves.Add(backward_move);
    }

    return Result<Vector<RetimingMove>>::Success(moves);
}

RetimingMoveSafety RetimingAnalysis::DetermineSafety(
    const RegToRegPathInfo& path,
    const CdcReport& cdc_report,
    const Vector<String>& cdc_anchored_regs
) {
    // Check if either register in the path is part of a CDC crossing
    if (cdc_anchored_regs.Find(path.src_reg_id) >= 0 || 
        cdc_anchored_regs.Find(path.dst_reg_id) >= 0) {
        return RetimingMoveSafety::Forbidden;
    }
    
    // Check if the path crosses a clock domain (shouldn't happen if we filtered properly)
    if (path.crosses_clock_domain) {
        return RetimingMoveSafety::Forbidden;
    }
    
    // If the path is within a single domain and doesn't involve CDC registers, it's safe
    return RetimingMoveSafety::SafeIntraDomain;
}

int RetimingAnalysis::EstimateDepthAfterMove(
    int before_depth,
    RetimingMoveDirection direction
) {
    // For MVP, a simple heuristic:
    // If we're moving a register, we're conceptually splitting the path
    // So if the original depth was N, we might get something like N/2 on each side
    if (direction == RetimingMoveDirection::Forward || direction == RetimingMoveDirection::Backward) {
        // Conservative estimate: reduce depth by about half, but ensure it's at least 1
        return std::max(1, before_depth / 2);
    }
    return before_depth; // If direction is unknown, return original depth
}

} // namespace ProtoVMCLI