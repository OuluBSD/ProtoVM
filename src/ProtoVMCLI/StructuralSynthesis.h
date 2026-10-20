#pragma once

#include "SessionTypes.h"  // For Result<T>
#include "CircuitGraph.h"
#include "FunctionalAnalysis.h"
#include "HlsIr.h"
#include "CdcAnalysis.h"  // For CDC fences
#include "Transformations.h"  // For TransformationPlan

#include <Core/Core.h>
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

struct StructuralPattern : public Upp::Moveable<StructuralPattern> {
    StructuralPattern() = default;
    StructuralPattern(const StructuralPattern& p)
        : Upp::Moveable<StructuralPattern>()
        , pattern_id(p.pattern_id)
        , kind(p.kind)
        , description(p.description)
    {
        for (int i = 0; i < p.node_ids.GetCount(); i++)
            node_ids.Add(p.node_ids[i]);
    }

    String pattern_id;

    StructuralPatternKind kind;

    // Nodes / components involved in this pattern (e.g. gate or block IDs).
    Vector<String> node_ids;

    // Optional explanatory metadata.
    String description;
};

struct StructuralRefactorMove : public Upp::Moveable<StructuralRefactorMove> {
    StructuralRefactorMove() = default;
    StructuralRefactorMove(const StructuralRefactorMove& p)
        : Upp::Moveable<StructuralRefactorMove>()
        , move_id(p.move_id)
        , target_block_id(p.target_block_id)
        , kind(p.kind)
        , safety(p.safety)
        , safety_reason(p.safety_reason)
        , transform_hint(p.transform_hint)
    {
        for (int i = 0; i < p.affected_node_ids.GetCount(); i++)
            affected_node_ids.Add(p.affected_node_ids[i]);
    }

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

struct StructuralRefactorPlan : public Upp::Moveable<StructuralRefactorPlan> {
    StructuralRefactorPlan()
        : gate_count_before(-1)
        , gate_count_after_estimate(-1)
        , depth_before(-1)
        , depth_after_estimate(-1)
        , respects_cdc_fences(true) {}
    StructuralRefactorPlan(const StructuralRefactorPlan& p)
        : Upp::Moveable<StructuralRefactorPlan>()
        , id(p.id)
        , target_block_id(p.target_block_id)
        , gate_count_before(p.gate_count_before)
        , gate_count_after_estimate(p.gate_count_after_estimate)
        , depth_before(p.depth_before)
        , depth_after_estimate(p.depth_after_estimate)
        , respects_cdc_fences(p.respects_cdc_fences)
    {
        for (const auto& pt : p.patterns)
            patterns.Add(pt);
        for (const auto& mv : p.moves)
            moves.Add(mv);
    }

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
    static ProtoVMCLI::Result<StructuralRefactorPlan> AnalyzeBlockStructure(
        const String& block_id,
        const ProtoVMCLI::CircuitGraph& graph,
        const ProtoVMCLI::FunctionalAnalysis* functional = nullptr,  // optional
        const ProtoVMCLI::IrModule* ir_module = nullptr,          // optional: IR view of the block
        const CdcReport* cdc_report = nullptr            // optional: do not cross CDC fences
    );

    // Helper methods for pattern detection
    static Vector<StructuralPattern> DetectRedundantLogic(
        const String& block_id,
        const ProtoVMCLI::CircuitGraph& graph,
        const ProtoVMCLI::FunctionalAnalysis* functional
    );

    static Vector<StructuralPattern> DetectCommonSubexpressions(
        const String& block_id,
        const ProtoVMCLI::CircuitGraph& graph,
        const ProtoVMCLI::FunctionalAnalysis* functional,
        const ProtoVMCLI::IrModule* ir_module
    );

    static Vector<StructuralPattern> DetectCanonicalForms(
        const String& block_id,
        const ProtoVMCLI::CircuitGraph& graph,
        const ProtoVMCLI::IrModule* ir_module
    );

    static Vector<StructuralPattern> DetectConstantPropagation(
        const String& block_id,
        const ProtoVMCLI::CircuitGraph& graph,
        const ProtoVMCLI::IrModule* ir_module
    );

    static Vector<StructuralPattern> DetectDeadLogic(
        const String& block_id,
        const ProtoVMCLI::CircuitGraph& graph,
        const ProtoVMCLI::FunctionalAnalysis* functional
    );

    // Assign safety levels to refactor moves
    static StructuralRefactorSafety AssessSafety(
        const StructuralRefactorMove& move,
        const CdcReport* cdc_report
    );
};