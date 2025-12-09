#ifndef _ProtoVM_IrOptimization_h_
#define _ProtoVM_IrOptimization_h_

#include "HlsIr.h"
#include "BehavioralAnalysis.h"
#include "DiffAnalysis.h"
#include "SessionTypes.h"
#include "Transformations.h"
#include "ProtoVM.h"  // Include U++ types
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class IrOptPassKind {
    SimplifyAlgebraic,
    FoldConstants,
    SimplifyMux,
    EliminateTrivialLogic,
    // extendable
};

struct IrOptChangeSummary {
    IrOptPassKind pass_kind;
    int expr_changes;       // how many expressions were altered
    int reg_changes;        // how many reg assignments altered
    bool behavior_preserved; // heuristic decision

    IrOptChangeSummary() : pass_kind(IrOptPassKind::SimplifyAlgebraic), expr_changes(0), reg_changes(0), behavior_preserved(true) {}
    IrOptChangeSummary(IrOptPassKind kind, int expr_ch, int reg_ch, bool behavior_pres)
        : pass_kind(kind), expr_changes(expr_ch), reg_changes(reg_ch), behavior_preserved(behavior_pres) {}
};

struct IrOptimizationResult {
    IrModule original;
    IrModule optimized;
    std::vector<IrOptChangeSummary> summaries;

    IrOptimizationResult() {}
    IrOptimizationResult(const IrModule& orig, const IrModule& opt, const std::vector<IrOptChangeSummary>& summ)
        : original(orig), optimized(opt), summaries(summ) {}
};

class IrOptimizer {
public:
    // Apply a set of optimization passes to an IR module.
    Result<IrOptimizationResult> OptimizeModule(
        const IrModule& module,
        const std::vector<IrOptPassKind>& passes_to_run
    );

private:
    // Individual optimization passes
    Result<bool> SimplifyAlgebraicPass(IrModule& module, int& change_count);
    Result<bool> FoldConstantsPass(IrModule& module, int& change_count);
    Result<bool> SimplifyMuxPass(IrModule& module, int& change_count);
    Result<bool> EliminateTrivialLogicPass(IrModule& module, int& change_count);

    // Helper methods
    IrExpr SimplifyAlgebraicExpression(const IrExpr& expr);
    IrExpr FoldConstantsExpression(const IrExpr& expr);
    IrExpr SimplifyMuxExpression(const IrExpr& expr);
    IrExpr EliminateTrivialLogicExpression(const IrExpr& expr);
};

// Function to verify behavior preservation
Result<bool> VerifyIrOptimizationBehaviorPreserved(
    const BehaviorDescriptor& before_behavior,
    const BehaviorDescriptor& after_behavior
);

// Bridge from IR diffs to transformation plans
class IrToTransformationBridge {
public:
    // Generate transformation plans from IR diff between original and optimized modules.
    static Result<std::vector<TransformationPlan>> PlansFromIrDiff(
        const IrModule& original,
        const IrModule& optimized,
        const IrDiff& ir_diff,
        const std::string& block_id
    );

private:
    // Helper methods to create specific transformation plans
    static TransformationPlan CreateSimplifyDoubleInversionPlan(const IrExprChange& change, const std::string& block_id);
    static TransformationPlan CreateSimplifyRedundantGatePlan(const IrExprChange& change, const std::string& block_id);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_IrOptimization_h_