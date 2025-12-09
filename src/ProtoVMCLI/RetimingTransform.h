#ifndef _ProtoVM_RetimingTransform_h_
#define _ProtoVM_RetimingTransform_h_

#include "RetimingModel.h"
#include "Transformations.h"
#include "SessionTypes.h"
#include "SessionStore.h"
#include "ProtoVM.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

struct RetimingApplicationOptions {
    bool apply_only_safe_moves = true;  // ignore non-SafeIntraDomain moves
    bool allow_suspicious_moves = false; // if true, include Suspicious in generated plans
    int  max_moves = -1;                // limit number of moves applied per plan (-1 = no limit)
};

struct RetimingApplicationResult {
    String plan_id;                 // id of the RetimingPlan
    String target_id;               // block or subsystem id

    // IDs of moves that were actually applied.
    Vector<String> applied_move_ids;
    Vector<String> skipped_move_ids; // moves skipped due to safety/limits

    // New circuit revision after applying transformations (if tracked).
    int new_circuit_revision = -1;

    // Optional summary numbers:
    int estimated_max_depth_before = -1;
    int estimated_max_depth_after = -1;

    // Whether all applied moves were SafeIntraDomain.
    bool all_moves_safe = true;
};

class RetimingTransform {
public:
    // Convert a RetimingPlan into a TransformationPlan (without applying).
    static Result<TransformationPlan> BuildTransformationPlanForRetiming(
        const RetimingPlan& retiming_plan,
        const RetimingApplicationOptions& options
    );

    // Apply a RetimingPlan directly to a given branch/session, using TransformationEngine.
    // This is a convenience wrapper that builds + applies.
    static Result<RetimingApplicationResult> ApplyRetimingPlanInBranch(
        const RetimingPlan& retiming_plan,
        const RetimingApplicationOptions& options,
        SessionStore& session_store,
        const SessionMetadata& session,
        const String& session_dir,
        const String& branch_name
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_RetimingTransform_h_