#pragma once

#include "Result.h"
#include "CircuitGraph.h"
#include "FunctionalAnalysis.h"
#include "HlsIr.h"
#include "CdcAnalysis.h"  // For CDC fences
#include "Transformations.h"  // For TransformationPlan

#include <Upp/Upp.h>
using namespace Upp;

enum class StructuralPatternKind {
    RedundantLogic,        // e.g., duplicated cones, X & X, X | X, etc.
    CommonSubexpression,   // shared logic reused across multiple outputs
    CanonicalMux,          // mux trees that can be normalized
    CanonicalAdder,        // adder-like structures that can be normalized
    CanonicalComparator,   // comparator-like structures
    ConstantPropagation,   // cones dominated by constants
    DeadLogic              // logic that does not influence observable outputs
};

enum class StructuralRefactorSafety {
    Safe,          // expected behavior preserved under current heuristics
    Suspicious,    // might be safe, but needs human/AI review
    Forbidden      // detected, but should not be auto-applied
};

struct StructuralPattern {
    String pattern_id;

    StructuralPatternKind kind;

    // Nodes / components involved in this pattern (e.g. gate or block IDs).
    Vector<String> node_ids;

    // Optional explanatory metadata.
    String description;
};

struct StructuralRefactorMove {
    String move_id;
    String target_block_id;        // block in which this refactor occurs

    StructuralPatternKind kind;
    Vector<String> affected_node_ids;

    StructuralRefactorSafety safety;
    String safety_reason;

    // Optional: can reference existing TransformationPlan templates,
    // or high-level description of the intended structural rewrite.
    String transform_hint;
};

struct StructuralRefactorPlan {
    String id;
    String target_block_id;

    Vector<StructuralPattern> patterns;
    Vector<StructuralRefactorMove> moves;

    // Estimated metrics:
    int gate_count_before = -1;
    int gate_count_after_estimate = -1;
    int depth_before = -1;       // optional combinational depth estimate
    int depth_after_estimate = -1;

    bool respects_cdc_fences = true;
};

class StructuralAnalysis {
public:
    // Analyze a block for structural simplification patterns.
    static Result<StructuralRefactorPlan> AnalyzeBlockStructure(
        const String& block_id,
        const CircuitGraph& graph,
        const FunctionalAnalysis* functional = nullptr,  // optional
        const HlsIrModule* ir_module = nullptr,          // optional: IR view of the block
        const CdcReport* cdc_report = nullptr            // optional: do not cross CDC fences
    );

    // Helper methods for pattern detection
    static Vector<StructuralPattern> DetectRedundantLogic(
        const String& block_id,
        const CircuitGraph& graph,
        const FunctionalAnalysis* functional
    );
    
    static Vector<StructuralPattern> DetectCommonSubexpressions(
        const String& block_id,
        const CircuitGraph& graph,
        const FunctionalAnalysis* functional,
        const HlsIrModule* ir_module
    );
    
    static Vector<StructuralPattern> DetectCanonicalForms(
        const String& block_id,
        const CircuitGraph& graph,
        const HlsIrModule* ir_module
    );
    
    static Vector<StructuralPattern> DetectConstantPropagation(
        const String& block_id,
        const CircuitGraph& graph,
        const HlsIrModule* ir_module
    );
    
    static Vector<StructuralPattern> DetectDeadLogic(
        const String& block_id,
        const CircuitGraph& graph,
        const FunctionalAnalysis* functional
    );

    // Assign safety levels to refactor moves
    static StructuralRefactorSafety AssessSafety(
        const StructuralRefactorMove& move,
        const CdcReport* cdc_report
    );
};