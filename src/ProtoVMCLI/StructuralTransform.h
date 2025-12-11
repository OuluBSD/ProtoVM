#pragma once

#include "StructuralSynthesis.h"
#include "SessionTypes.h"
#include "SessionStore.h"
#include "Transformations.h"
#include "RetimingTransform.h"  // For RetimingApplicationResult

#include <Core/Core.h>
using namespace Upp;

class StructuralTransform {
public:
    // Build a TransformationPlan from a structural refactor plan (without applying).
    static Result<TransformationPlan> BuildTransformationPlanForStructuralRefactor(
        const StructuralRefactorPlan& plan,
        bool apply_only_safe_moves = true
    );

    // Apply a StructuralRefactorPlan directly in a branch (build + apply).
    static Result<RetimingApplicationResult> ApplyStructuralRefactorInBranch(
        const StructuralRefactorPlan& plan,
        bool apply_only_safe_moves,
        SessionStore& session_store,
        const SessionMetadata& session,
        const String& session_dir,
        const String& branch_name
    );

    // Helper to convert StructuralRefactorMoves to TransformationSteps
    static Vector<TransformationStep> ConvertMovesToSteps(
        const Vector<StructuralRefactorMove>& moves
    );
};