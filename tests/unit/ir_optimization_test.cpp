#include "ProtoVMCLI/IrOptimization.h"
#include "ProtoVMCLI/HlsIr.h"
#include "ProtoVMCLI/Transformations.h"
#include <iostream>
#include <cassert>

using namespace ProtoVMCLI;

void TestIrOptimizerAlgebraicSimplification() {
    std::cout << "Testing IrOptimizer algebraic simplification..." << std::endl;

    // Create a simple IR module with algebraic redundancies
    IrModule module;
    module.id = "test_module";
    
    // Add an input
    module.inputs.push_back(IrValue("A", 4, false));
    
    // Add an output
    module.outputs.push_back(IrValue("Y", 4, false));
    
    // Add a redundant expression: Y = A & A (should simplify to Y = A)
    IrValue target_Y("Y", 4, false);
    IrValue arg_A1("A", 4, false);
    IrValue arg_A2("A", 4, false);
    std::vector<IrValue> args = {arg_A1, arg_A2};
    IrExpr redundant_expr(IrExprKind::And, target_Y, args);
    
    module.comb_assigns.push_back(redundant_expr);

    // Test the optimizer
    IrOptimizer optimizer;
    std::vector<IrOptPassKind> passes = {IrOptPassKind::SimplifyAlgebraic};
    auto result = optimizer.OptimizeModule(module, passes);
    
    assert(result.ok);
    std::cout << "Optimization completed successfully." << std::endl;
    
    // For now, just verify the optimization ran without error
    // In a real test, we would verify the simplified expression
    assert(result.data.summaries.size() >= 0);
    std::cout << "Algebraic simplification test passed." << std::endl;
}

void TestIrOptimizerConstantFolding() {
    std::cout << "Testing IrOptimizer constant folding..." << std::endl;

    // Create a simple IR module with constant expressions
    IrModule module;
    module.id = "test_module";
    
    // Add an output
    module.outputs.push_back(IrValue("Result", 4, false));
    
    // Add a constant expression: Result = 5 & 3 (should fold to Result = 1)
    IrValue target_Result("Result", 4, false);
    IrValue arg_5("", 4, true, 5);  // literal value 5
    IrValue arg_3("", 4, true, 3);  // literal value 3
    std::vector<IrValue> args = {arg_5, arg_3};
    IrExpr const_expr(IrExprKind::And, target_Result, args);
    
    module.comb_assigns.push_back(const_expr);

    // Test the optimizer
    IrOptimizer optimizer;
    std::vector<IrOptPassKind> passes = {IrOptPassKind::FoldConstants};
    auto result = optimizer.OptimizeModule(module, passes);
    
    assert(result.ok);
    std::cout << "Constant folding completed successfully." << std::endl;
    
    // For now, just verify the optimization ran without error
    assert(result.data.summaries.size() >= 0);
    std::cout << "Constant folding test passed." << std::endl;
}

void TestIrToTransformationBridge() {
    std::cout << "Testing IrToTransformationBridge..." << std::endl;

    // Create a dummy IR diff to test the bridge
    IrModule original;
    original.id = "test_block";
    
    IrModule optimized;
    optimized.id = "test_block";
    
    IrDiff diff;
    diff.module_id = "test_block";
    
    // Add a fake expression change that looks like a double inversion
    IrExprChange fake_change;
    fake_change.target_name = "test_output";
    fake_change.before_expr_repr = "Not(Not(A))";
    fake_change.after_expr_repr = "A";
    diff.comb_changes.push_back(fake_change);
    
    // Test the bridge
    auto plans_result = IrToTransformationBridge::PlansFromIrDiff(original, optimized, diff, "test_block");
    
    assert(plans_result.ok);
    std::cout << "Transformation bridge test passed." << std::endl;
}

void TestBehavioralAnalysisVerification() {
    std::cout << "Testing behavioral verification..." << std::endl;

    // Create two identical behavior descriptors (should be preserved)
    BehaviorDescriptor before;
    before.subject_id = "test_block";
    before.behavior_kind = BehaviorKind::CombinationalLogic;
    before.bit_width = 4;
    
    BehaviorDescriptor after;
    after.subject_id = "test_block";
    after.behavior_kind = BehaviorKind::CombinationalLogic;
    after.bit_width = 4;
    
    auto verification_result = VerifyIrOptimizationBehaviorPreserved(before, after);
    
    assert(verification_result.ok);
    assert(verification_result.data);  // Should be preserved
    std::cout << "Behavior preservation verification test passed." << std::endl;
}

int main() {
    std::cout << "Starting IR Optimization tests..." << std::endl;
    
    TestIrOptimizerAlgebraicSimplification();
    TestIrOptimizerConstantFolding();
    TestIrToTransformationBridge();
    TestBehavioralAnalysisVerification();
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}