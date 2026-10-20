#ifndef _ProtoVM_RetimingModel_h_
#define _ProtoVM_RetimingModel_h_

#include "SessionTypes.h"  // For Result<T>
#include <ProtoVM/ProtoVM.h>       // For Upp types
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class RetimingMoveDirection {
    Forward,   // move register(s) closer to outputs / later in pipeline
    Backward   // move register(s) closer to inputs / earlier in pipeline
};

enum class RetimingMoveSafety {
    SafeIntraDomain,     // no CDC, no known hazards
    Suspicious,          // heuristically possible issues; needs human/AI review
    Forbidden            // should not be applied (documented as such)
};

struct RetimingMove : public Upp::Moveable<RetimingMove> {
    RetimingMove() = default;
    RetimingMove(const RetimingMove& p)
        : Upp::Moveable<RetimingMove>()
        , move_id(p.move_id)
        , src_reg_id(p.src_reg_id)
        , dst_reg_id(p.dst_reg_id)
        , direction(p.direction)
        , domain_id(p.domain_id)
        , src_stage_index(p.src_stage_index)
        , dst_stage_index(p.dst_stage_index)
        , before_comb_depth(p.before_comb_depth)
        , after_comb_depth_est(p.after_comb_depth_est)
        , safety(p.safety)
        , safety_reason(p.safety_reason)
    {
        for (int i = 0; i < p.affected_ops.GetCount(); i++)
            affected_ops.Add(p.affected_ops[i]);
    }

    String move_id;             // stable id for this move (e.g. "RTM_0001")

    String src_reg_id;          // register being moved or part of path
    String dst_reg_id;          // register on the other side of the path (optional; may represent region)

    RetimingMoveDirection direction;

    int domain_id;              // clock domain of the move
    int src_stage_index;        // pipeline stage index of src reg
    int dst_stage_index;        // target or related stage index

    int before_comb_depth;      // estimated comb depth on the path before move
    int after_comb_depth_est;   // estimated comb depth after move (heuristic)

    RetimingMoveSafety safety;
    String safety_reason;       // explanation (e.g. "intra-domain, no CDC crossings")

    // Optional: list of intermediate nodes/ops affected (for explanation)
    Vector<String> affected_ops; // ids or names of scheduled ops / nodes
};

struct RetimingPlan : public Upp::Moveable<RetimingPlan> {
    RetimingPlan()
        : estimated_max_depth_before(-1)
        , estimated_max_depth_after(-1)
        , respects_cdc_fences(true) {}
    RetimingPlan(const RetimingPlan& p)
        : Upp::Moveable<RetimingPlan>()
        , id(p.id)
        , target_id(p.target_id)
        , description(p.description)
        , estimated_max_depth_before(p.estimated_max_depth_before)
        , estimated_max_depth_after(p.estimated_max_depth_after)
        , respects_cdc_fences(p.respects_cdc_fences)
    {
        for (const auto& m : p.moves)
            moves.Add(m);
    }

    String id;                  // plan id, e.g. "RTP_ALU_BALANCE_1"
    String target_id;           // block or subsystem id
    String description;         // short human-readable summary

    Vector<RetimingMove> moves;

    // Aggregate metrics / estimates:
    int estimated_max_depth_before;
    int estimated_max_depth_after;
    bool respects_cdc_fences;   // true if no moves cross CDC hazards
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_RetimingModel_h_