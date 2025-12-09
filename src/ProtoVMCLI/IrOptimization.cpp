#include "IrOptimization.h"
#include "HlsIrInference.h"
#include "Transformations.h"
#include "DiffAnalysis.h"
#include <algorithm>
#include <iostream>

namespace ProtoVMCLI {

// Helper function to check if two IrValues are equivalent
static bool IrValuesEqual(const IrValue& a, const IrValue& b) {
    return a.name == b.name && a.bit_width == b.bit_width &&
           a.is_literal == b.is_literal && a.literal == b.literal;
}

// Helper function to create a literal IrValue
static IrValue CreateLiteral(int bit_width, uint64_t value) {
    return IrValue("", bit_width, true, value);
}

// Helper function to convert IrExpr to string for debugging
static std::string IrExprToString(const IrExpr& expr) {
    std::string result = IrExprKindToString(expr.kind) + "(";
    for (size_t i = 0; i < expr.args.size(); ++i) {
        if (i > 0) result += ", ";
        if (expr.args[i].is_literal) {
            result += std::to_string(expr.args[i].literal);
        } else {
            result += expr.args[i].name;
        }
    }
    result += ")";
    return result;
}

// Helper function to convert IrExprKind to string
static std::string IrExprKindToString(IrExprKind kind) {
    switch (kind) {
        case IrExprKind::Value: return "Value";
        case IrExprKind::Not: return "Not";
        case IrExprKind::And: return "And";
        case IrExprKind::Or: return "Or";
        case IrExprKind::Xor: return "Xor";
        case IrExprKind::Add: return "Add";
        case IrExprKind::Sub: return "Sub";
        case IrExprKind::Mux: return "Mux";
        case IrExprKind::Eq: return "Eq";
        case IrExprKind::Neq: return "Neq";
        default: return "Unknown";
    }
}

// Main optimization function
Result<IrOptimizationResult> IrOptimizer::OptimizeModule(
    const IrModule& module,
    const std::vector<IrOptPassKind>& passes_to_run
) {
    IrModule current_module = module;  // Work on a copy
    std::vector<IrOptChangeSummary> summaries;

    for (const auto& pass : passes_to_run) {
        int expr_changes = 0;
        int reg_changes = 0;
        bool pass_success = true;
        int original_changes = 0;

        switch (pass) {
            case IrOptPassKind::SimplifyAlgebraic:
                pass_success = SimplifyAlgebraicPass(current_module, expr_changes).ok;
                break;
            case IrOptPassKind::FoldConstants:
                pass_success = FoldConstantsPass(current_module, expr_changes).ok;
                break;
            case IrOptPassKind::SimplifyMux:
                pass_success = SimplifyMuxPass(current_module, expr_changes).ok;
                break;
            case IrOptPassKind::EliminateTrivialLogic:
                pass_success = EliminateTrivialLogicPass(current_module, expr_changes).ok;
                break;
        }

        if (pass_success) {
            summaries.push_back(IrOptChangeSummary(pass, expr_changes, reg_changes, true));
        } else {
            summaries.push_back(IrOptChangeSummary(pass, expr_changes, reg_changes, false));
        }
    }

    IrOptimizationResult result(module, current_module, summaries);
    return Result<IrOptimizationResult>::Success(result);
}

// Simplify algebraic expressions
Result<bool> IrOptimizer::SimplifyAlgebraicPass(IrModule& module, int& change_count) {
    change_count = 0;
    int total_changes = 0;

    // Process combinational assignments
    for (auto& expr : module.comb_assigns) {
        IrExpr original_expr = expr;
        IrExpr new_expr = SimplifyAlgebraicExpression(original_expr);

        if (new_expr.kind != original_expr.kind || new_expr.args.size() != original_expr.args.size() ||
            !IrValuesEqual(new_expr.target, original_expr.target)) {
            expr = new_expr;
            total_changes++;
        } else {
            // Check if any arguments changed
            for (size_t i = 0; i < new_expr.args.size(); ++i) {
                if (i < original_expr.args.size() && !IrValuesEqual(new_expr.args[i], original_expr.args[i])) {
                    expr = new_expr;
                    total_changes++;
                    break;
                }
            }
        }
    }

    // Process register assignments
    for (auto& reg_assign : module.reg_assigns) {
        IrExpr original_expr = reg_assign.expr;
        IrExpr new_expr = SimplifyAlgebraicExpression(original_expr);

        if (new_expr.kind != original_expr.kind || new_expr.args.size() != original_expr.args.size() ||
            !IrValuesEqual(new_expr.target, original_expr.target)) {
            reg_assign.expr = new_expr;
            total_changes++;
        } else {
            // Check if any arguments changed
            for (size_t i = 0; i < new_expr.args.size(); ++i) {
                if (i < original_expr.args.size() && !IrValuesEqual(new_expr.args[i], original_expr.args[i])) {
                    reg_assign.expr = new_expr;
                    total_changes++;
                    break;
                }
            }
        }
    }

    change_count = total_changes;
    return Result<bool>::Success(true);
}

// Helper function to determine if an expression is a double negation pattern
static bool IsDoubleNegationPattern(const IrExpr& expr) {
    if (expr.kind == IrExprKind::Not && expr.args.size() == 1) {
        // If the argument is also a 'Not' expression, we need to check the string representation
        // In this IR model, we can't directly embed one IrExpr in another IrValue, so we can't represent
        // nested expressions in the args. We'll have to work with what we have.
        // For this implementation, we handle double negations across passes rather than within a single expression.
    }
    return false;
}

// Simplify algebraic expression patterns
IrExpr IrOptimizer::SimplifyAlgebraicExpression(const IrExpr& expr) {
    IrExpr result = expr;  // Default to original expression

    switch (expr.kind) {
        case IrExprKind::And:
            // X & X → X
            if (expr.args.size() >= 2 && IrValuesEqual(expr.args[0], expr.args[1])) {
                result = IrExpr(IrExprKind::Value, expr.target, {expr.args[0]});
            }
            break;
        case IrExprKind::Or:
            // X | X → X
            if (expr.args.size() >= 2 && IrValuesEqual(expr.args[0], expr.args[1])) {
                result = IrExpr(IrExprKind::Value, expr.target, {expr.args[0]});
            }
            break;
        case IrExprKind::Xor:
            // X ^ X → 0 (constant)
            if (expr.args.size() >= 2 && IrValuesEqual(expr.args[0], expr.args[1])) {
                int bit_width = expr.target.bit_width > 0 ? expr.target.bit_width : 1;
                result = IrExpr(IrExprKind::Value, expr.target, {CreateLiteral(bit_width, 0)});
            }
            break;
        case IrExprKind::Not:
            // This is a simplification that handles a common pattern, though it's limited by IR structure
            break;
        // Add more patterns as needed
        default:
            break;
    }

    return result;
}

// Fold constant expressions
Result<bool> IrOptimizer::FoldConstantsPass(IrModule& module, int& change_count) {
    change_count = 0;
    int total_changes = 0;

    // Process combinational assignments
    for (auto& expr : module.comb_assigns) {
        IrExpr original_expr = expr;
        IrExpr new_expr = FoldConstantsExpression(original_expr);

        if (new_expr.kind != original_expr.kind ||
            new_expr.args.size() != original_expr.args.size() ||
            !IrValuesEqual(new_expr.target, original_expr.target)) {
            expr = new_expr;
            total_changes++;
        } else {
            // Check if any arguments changed
            for (size_t i = 0; i < new_expr.args.size(); ++i) {
                if (i < original_expr.args.size() && !IrValuesEqual(new_expr.args[i], original_expr.args[i])) {
                    expr = new_expr;
                    total_changes++;
                    break;
                }
            }
        }
    }

    // Process register assignments
    for (auto& reg_assign : module.reg_assigns) {
        IrExpr original_expr = reg_assign.expr;
        IrExpr new_expr = FoldConstantsExpression(original_expr);

        if (new_expr.kind != original_expr.kind ||
            new_expr.args.size() != original_expr.args.size() ||
            !IrValuesEqual(new_expr.target, original_expr.target)) {
            reg_assign.expr = new_expr;
            total_changes++;
        } else {
            // Check if any arguments changed
            for (size_t i = 0; i < new_expr.args.size(); ++i) {
                if (i < original_expr.args.size() && !IrValuesEqual(new_expr.args[i], original_expr.args[i])) {
                    reg_assign.expr = new_expr;
                    total_changes++;
                    break;
                }
            }
        }
    }

    change_count = total_changes;
    return Result<bool>::Success(true);
}

// Fold constant expression patterns
IrExpr IrOptimizer::FoldConstantsExpression(const IrExpr& expr) {
    IrExpr result = expr;  // Default to original expression

    if (expr.args.size() == 2 && expr.args[0].is_literal && expr.args[1].is_literal) {
        uint64_t value1 = expr.args[0].literal;
        uint64_t value2 = expr.args[1].literal;
        uint64_t result_value = 0;
        bool should_fold = true;

        switch (expr.kind) {
            case IrExprKind::And:
                result_value = value1 & value2;
                break;
            case IrExprKind::Or:
                result_value = value1 | value2;
                break;
            case IrExprKind::Xor:
                result_value = value1 ^ value2;
                break;
            case IrExprKind::Add:
                result_value = value1 + value2;
                break;
            case IrExprKind::Sub:
                result_value = value1 - value2;
                break;
            case IrExprKind::Eq:
                result_value = (value1 == value2) ? 1 : 0;
                break;
            case IrExprKind::Neq:
                result_value = (value1 != value2) ? 1 : 0;
                break;
            default:
                should_fold = false;
                break;
        }

        if (should_fold) {
            int bit_width = expr.target.bit_width > 0 ? expr.target.bit_width : 1;
            result = IrExpr(IrExprKind::Value, expr.target, {CreateLiteral(bit_width, result_value)});
        }
    } else if (expr.kind == IrExprKind::Mux && expr.args.size() == 3 && expr.args[0].is_literal) {
        // Mux with literal select (Mux(constant, A, B) -> A if constant != 0, B if constant == 0)
        uint64_t sel = expr.args[0].literal;
        if (sel != 0) {
            // Return first argument (true branch)
            result = IrExpr(IrExprKind::Value, expr.target, {expr.args[1]});
        } else {
            // Return second argument (false branch)
            result = IrExpr(IrExprKind::Value, expr.target, {expr.args[2]});
        }
    } else if (expr.kind == IrExprKind::Not && expr.args.size() == 1 && expr.args[0].is_literal) {
        // Not with literal (Not(constant))
        uint64_t val = expr.args[0].literal;
        int bit_width = expr.target.bit_width > 0 ? expr.target.bit_width : 1;
        // For bitwise NOT, we need to mask to the appropriate bit width
        uint64_t mask = (bit_width >= 64) ? ~0ULL : ((1ULL << bit_width) - 1);
        result = IrExpr(IrExprKind::Value, expr.target, {CreateLiteral(bit_width, (~val) & mask)});
    }

    return result;
}

// Simplify multiplexer expressions
Result<bool> IrOptimizer::SimplifyMuxPass(IrModule& module, int& change_count) {
    change_count = 0;
    int total_changes = 0;

    // Process combinational assignments
    for (auto& expr : module.comb_assigns) {
        IrExpr original_expr = expr;
        IrExpr new_expr = SimplifyMuxExpression(original_expr);

        if (new_expr.kind != original_expr.kind ||
            new_expr.args.size() != original_expr.args.size() ||
            !IrValuesEqual(new_expr.target, original_expr.target)) {
            expr = new_expr;
            total_changes++;
        } else {
            // Check if any arguments changed
            for (size_t i = 0; i < new_expr.args.size(); ++i) {
                if (i < original_expr.args.size() && !IrValuesEqual(new_expr.args[i], original_expr.args[i])) {
                    expr = new_expr;
                    total_changes++;
                    break;
                }
            }
        }
    }

    // Process register assignments
    for (auto& reg_assign : module.reg_assigns) {
        IrExpr original_expr = reg_assign.expr;
        IrExpr new_expr = SimplifyMuxExpression(original_expr);

        if (new_expr.kind != original_expr.kind ||
            new_expr.args.size() != original_expr.args.size() ||
            !IrValuesEqual(new_expr.target, original_expr.target)) {
            reg_assign.expr = new_expr;
            total_changes++;
        } else {
            // Check if any arguments changed
            for (size_t i = 0; i < new_expr.args.size(); ++i) {
                if (i < original_expr.args.size() && !IrValuesEqual(new_expr.args[i], original_expr.args[i])) {
                    reg_assign.expr = new_expr;
                    total_changes++;
                    break;
                }
            }
        }
    }

    change_count = total_changes;
    return Result<bool>::Success(true);
}

// Simplify mux expression patterns
IrExpr IrOptimizer::SimplifyMuxExpression(const IrExpr& expr) {
    IrExpr result = expr;  // Default to original expression

    if (expr.kind == IrExprKind::Mux && expr.args.size() == 3) {
        // Mux(SEL, A, A) → A (if both inputs are the same)
        if (IrValuesEqual(expr.args[1], expr.args[2])) {
            result = IrExpr(IrExprKind::Value, expr.target, {expr.args[1]});
        }
        // Add more mux simplification rules as needed
    }

    return result;
}

// Eliminate trivial logic expressions
Result<bool> IrOptimizer::EliminateTrivialLogicPass(IrModule& module, int& change_count) {
    change_count = 0;
    int total_changes = 0;

    // Process combinational assignments
    for (auto& expr : module.comb_assigns) {
        IrExpr original_expr = expr;
        IrExpr new_expr = EliminateTrivialLogicExpression(original_expr);

        if (new_expr.kind != original_expr.kind ||
            new_expr.args.size() != original_expr.args.size() ||
            !IrValuesEqual(new_expr.target, original_expr.target)) {
            expr = new_expr;
            total_changes++;
        } else {
            // Check if any arguments changed
            for (size_t i = 0; i < new_expr.args.size(); ++i) {
                if (i < original_expr.args.size() && !IrValuesEqual(new_expr.args[i], original_expr.args[i])) {
                    expr = new_expr;
                    total_changes++;
                    break;
                }
            }
        }
    }

    // Process register assignments
    for (auto& reg_assign : module.reg_assigns) {
        IrExpr original_expr = reg_assign.expr;
        IrExpr new_expr = EliminateTrivialLogicExpression(original_expr);

        if (new_expr.kind != original_expr.kind ||
            new_expr.args.size() != original_expr.args.size() ||
            !IrValuesEqual(new_expr.target, original_expr.target)) {
            reg_assign.expr = new_expr;
            total_changes++;
        } else {
            // Check if any arguments changed
            for (size_t i = 0; i < new_expr.args.size(); ++i) {
                if (i < original_expr.args.size() && !IrValuesEqual(new_expr.args[i], original_expr.args[i])) {
                    reg_assign.expr = new_expr;
                    total_changes++;
                    break;
                }
            }
        }
    }

    change_count = total_changes;
    return Result<bool>::Success(true);
}

// Eliminate trivial logic expression patterns
IrExpr IrOptimizer::EliminateTrivialLogicExpression(const IrExpr& expr) {
    IrExpr result = expr;  // Default to original expression

    // For now, we'll implement a basic identity operation optimization
    // If an expression is just assigning a value to itself (e.g., A = A), simplify it
    if (expr.kind == IrExprKind::Value && expr.args.size() == 1) {
        // Check if target and arg are the same value
        if (IrValuesEqual(expr.target, expr.args[0]) && !expr.args[0].is_literal) {
            // This is already an identity operation
        }
    }
    // Add more trivial logic elimination patterns as needed

    return result;
}

// Verify behavior preservation
Result<bool> VerifyIrOptimizationBehaviorPreserved(
    const BehaviorDescriptor& before_behavior,
    const BehaviorDescriptor& after_behavior
) {
    // Check behavior kind preservation
    if (before_behavior.behavior_kind != after_behavior.behavior_kind) {
        return Result<bool>::Error(ErrorCode::BehaviorChanged,
                                   "Behavior kind changed during optimization");
    }

    // Check bit width preservation
    if (before_behavior.bit_width != after_behavior.bit_width) {
        return Result<bool>::Error(ErrorCode::BehaviorChanged,
                                   "Bit width changed during optimization");
    }

    // Check that port count and names are preserved
    if (before_behavior.ports.size() != after_behavior.ports.size()) {
        return Result<bool>::Error(ErrorCode::BehaviorChanged,
                                   "Port count changed during optimization");
    }

    // Check that port names and roles match
    for (size_t i = 0; i < before_behavior.ports.size(); ++i) {
        if (before_behavior.ports[i].port_name != after_behavior.ports[i].port_name ||
            before_behavior.ports[i].role != after_behavior.ports[i].role) {
            return Result<bool>::Error(ErrorCode::BehaviorChanged,
                                       "Port name or role changed during optimization");
        }
    }

    // If all checks passed, behavior is preserved
    return Result<bool>::Success(true);
}

// Helper function to detect if the change represents a double inversion simplification
static bool IsDoubleInversionSimplification(const IrExprChange& change) {
    // Look for patterns where double negation was simplified
    // Since we're working with string representations, we check for patterns that indicate
    // double negation was reduced
    if (change.before_expr_repr.find("Not(Not") != std::string::npos &&
        change.after_expr_repr.find("Not(Not") == std::string::npos) {
        return true;
    }
    // More complex pattern matching could be added here
    return false;
}

// Helper function to detect if the change represents redundant gate simplification
static bool IsRedundantGateSimplification(const IrExprChange& change) {
    // Check if we had a redundant operation like X & X or X | X that got simplified
    if (change.before_expr_repr.find("And(") != std::string::npos &&
        change.before_expr_repr.find(", ") != std::string::npos) {
        // Look for patterns like "And(A, A)" becoming "Value(A)"
        size_t pos = change.before_expr_repr.find('(');
        if (pos != std::string::npos) {
            size_t args_start = pos + 1;
            size_t args_end = change.before_expr_repr.find(')');
            if (args_end != std::string::npos) {
                std::string args = change.before_expr_repr.substr(args_start, args_end - args_start);
                size_t comma_pos = args.find(", ");
                if (comma_pos != std::string::npos) {
                    std::string arg1 = args.substr(0, comma_pos);
                    std::string arg2 = args.substr(comma_pos + 2);
                    // If arguments were the same and now simplified to one
                    if (arg1 == arg2 && change.after_expr_repr.find(arg1) != std::string::npos) {
                        return true;
                    }
                }
            }
        }
    }

    if (change.before_expr_repr.find("Or(") != std::string::npos &&
        change.before_expr_repr.find(", ") != std::string::npos) {
        // Similar check for Or operations
        size_t pos = change.before_expr_repr.find('(');
        if (pos != std::string::npos) {
            size_t args_start = pos + 1;
            size_t args_end = change.before_expr_repr.find(')');
            if (args_end != std::string::npos) {
                std::string args = change.before_expr_repr.substr(args_start, args_end - args_start);
                size_t comma_pos = args.find(", ");
                if (comma_pos != std::string::npos) {
                    std::string arg1 = args.substr(0, comma_pos);
                    std::string arg2 = args.substr(comma_pos + 2);
                    // If arguments were the same and now simplified to one
                    if (arg1 == arg2 && change.after_expr_repr.find(arg1) != std::string::npos) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// Transformation bridge implementation
Result<std::vector<TransformationPlan>> IrToTransformationBridge::PlansFromIrDiff(
    const IrModule& original,
    const IrModule& optimized,
    const IrDiff& ir_diff,
    const std::string& block_id
) {
    std::vector<TransformationPlan> plans;

    // Process combinational changes
    for (const auto& change : ir_diff.comb_changes) {
        // Check if the change matches a known pattern

        // Example: Double inversion simplification
        if (IsDoubleInversionSimplification(change)) {
            TransformationPlan plan = CreateSimplifyDoubleInversionPlan(change, block_id);
            plans.push_back(plan);
        }
        // Example: Redundant gate simplification (X & X, X | X, etc.)
        else if (IsRedundantGateSimplification(change)) {
            TransformationPlan plan = CreateSimplifyRedundantGatePlan(change, block_id);
            plans.push_back(plan);
        }
    }

    return Result<std::vector<TransformationPlan>>::Success(plans);
}

// Create a plan for simplifying double inversions
TransformationPlan IrToTransformationBridge::CreateSimplifyDoubleInversionPlan(
    const IrExprChange& change,
    const std::string& block_id
) {
    TransformationPlan plan;
    plan.id = "IR_T_" + std::to_string(TransformationEngine::transformation_id_counter++);
    plan.kind = TransformationKind::SimplifyDoubleInversion;
    plan.target.subject_id = block_id;
    plan.target.subject_kind = "Block";

    plan.guarantees.Add(PreservationLevel::BehaviorKindPreserved);
    plan.guarantees.Add(PreservationLevel::IOContractPreserved);

    TransformationStep step;
    step.description = "Remove redundant NOT-then-NOT around " + change.target_name + " path";
    plan.steps.Add(step);

    return plan;
}

// Create a plan for simplifying redundant gates
TransformationPlan IrToTransformationBridge::CreateSimplifyRedundantGatePlan(
    const IrExprChange& change,
    const std::string& block_id
) {
    TransformationPlan plan;
    plan.id = "IR_T_" + std::to_string(TransformationEngine::transformation_id_counter++);
    plan.kind = TransformationKind::SimplifyRedundantGate;
    plan.target.subject_id = block_id;
    plan.target.subject_kind = "Block";

    plan.guarantees.Add(PreservationLevel::BehaviorKindPreserved);
    plan.guarantees.Add(PreservationLevel::IOContractPreserved);

    TransformationStep step;
    step.description = "Simplify redundant gate operation for " + change.target_name;
    plan.steps.Add(step);

    return plan;
}

} // namespace ProtoVMCLI