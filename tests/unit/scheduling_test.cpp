#include "Scheduling.h"
#include "ScheduledIr.h"
#include "HlsIr.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace ProtoVMCLI;

void testSingleStageScheduling() {
    std::cout << "Testing SingleStage Scheduling..." << std::endl;

    // Create a simple IR module with some operations
    IrValue input1("A", 4);
    IrValue input2("B", 4);
    IrValue output("SUM", 4);
    std::vector<IrValue> inputs = {input1, input2};
    std::vector<IrValue> outputs = {output};

    IrExpr add_expr(IrExprKind::Add, output, inputs);
    std::vector<IrExpr> comb_assigns = {add_expr};
    std::vector<IrRegAssign> reg_assigns = {};

    IrModule ir_module("TEST_ADD", inputs, outputs, comb_assigns, reg_assigns);

    // Configure scheduling for single stage
    SchedulingConfig config;
    config.strategy = SchedulingStrategy::SingleStage;
    config.requested_stages = 1;

    // Perform scheduling
    auto result = SchedulingEngine::BuildSchedule(ir_module, nullptr, nullptr, config);
    assert(result.ok);
    
    ScheduledModule scheduled_module = result.data;
    assert(scheduled_module.num_stages == 1);
    assert(scheduled_module.comb_ops.size() == 1);
    assert(scheduled_module.comb_ops[0].stage == 0);  // All ops in stage 0 for SingleStage

    std::cout << "  ✓ SingleStage scheduling test passed" << std::endl;
}

void testFixedStageCountScheduling() {
    std::cout << "Testing FixedStageCount Scheduling..." << std::endl;

    // Create a more complex IR module with several operations
    IrValue input1("A", 4);
    IrValue input2("B", 4);
    IrValue tmp1("TMP1", 4);
    IrValue tmp2("TMP2", 4);
    IrValue output("RESULT", 4);
    std::vector<IrValue> inputs = {input1, input2};
    std::vector<IrValue> outputs = {output};

    // Create a chain of operations: A + B -> TMP1, TMP1 + A -> TMP2, TMP2 * B -> RESULT
    IrExpr add1_expr(IrExprKind::Add, tmp1, {input1, input2});  // Stage 0
    IrExpr add2_expr(IrExprKind::Add, tmp2, {tmp1, input1});    // Stage 1 (depends on TMP1)
    IrExpr mul_expr(IrExprKind::And, output, {tmp2, input2});   // Stage 2 (depends on TMP2)
    std::vector<IrExpr> comb_assigns = {add1_expr, add2_expr, mul_expr};
    std::vector<IrRegAssign> reg_assigns = {};

    IrModule ir_module("TEST_CHAIN", inputs, outputs, comb_assigns, reg_assigns);

    // Configure scheduling for fixed 3 stages
    SchedulingConfig config;
    config.strategy = SchedulingStrategy::FixedStageCount;
    config.requested_stages = 3;

    // Perform scheduling
    auto result = SchedulingEngine::BuildSchedule(ir_module, nullptr, nullptr, config);
    assert(result.ok);
    
    ScheduledModule scheduled_module = result.data;
    assert(scheduled_module.num_stages == 3);
    assert(scheduled_module.comb_ops.size() == 3);

    // With FixedStageCount, operations should be distributed across requested stages
    // The exact distribution depends on the algorithm, but all stages should be <= requested_stages - 1
    for (const auto& op : scheduled_module.comb_ops) {
        assert(op.stage < 3);  // Stage should be 0, 1, or 2
    }

    std::cout << "  ✓ FixedStageCount scheduling test passed" << std::endl;
}

void testDepthBalancedStagesScheduling() {
    std::cout << "Testing DepthBalancedStages Scheduling..." << std::endl;

    // Create an IR module with operations that have different depths
    IrValue input1("A", 4);
    IrValue input2("B", 4);
    IrValue input3("C", 4);
    IrValue tmp1("TMP1", 4);
    IrValue tmp2("TMP2", 4);
    IrValue output("RESULT", 4);
    std::vector<IrValue> inputs = {input1, input2, input3};
    std::vector<IrValue> outputs = {output};

    // Create operations with different depths:
    // Depth 1: tmp1 = A AND B
    // Depth 2: tmp2 = TMP1 OR C
    // Depth 3: RESULT = TMP2 XOR A
    IrExpr and_expr(IrExprKind::And, tmp1, {input1, input2});
    IrExpr or_expr(IrExprKind::Or, tmp2, {tmp1, input3});
    IrExpr xor_expr(IrExprKind::Xor, output, {tmp2, input1});
    std::vector<IrExpr> comb_assigns = {and_expr, or_expr, xor_expr};
    std::vector<IrRegAssign> reg_assigns = {};

    IrModule ir_module("TEST_DEPTHS", inputs, outputs, comb_assigns, reg_assigns);

    // Configure scheduling for depth-balanced stages (max 4 stages)
    SchedulingConfig config;
    config.strategy = SchedulingStrategy::DepthBalancedStages;
    config.requested_stages = 4;  // Upper bound

    // Perform scheduling
    auto result = SchedulingEngine::BuildSchedule(ir_module, nullptr, nullptr, config);
    assert(result.ok);
    
    ScheduledModule scheduled_module = result.data;
    // Should have at most 4 stages but possibly fewer based on actual depth
    assert(scheduled_module.num_stages <= 4);
    assert(scheduled_module.comb_ops.size() == 3);

    // Verify that deeper operations are not in earlier stages than their dependencies
    // Find the stages of each operation
    int and_stage = -1, or_stage = -1, xor_stage = -1;
    for (const auto& op : scheduled_module.comb_ops) {
        if (op.expr.target.name == "TMP1") {
            and_stage = op.stage;
        } else if (op.expr.target.name == "TMP2") {
            or_stage = op.stage;
        } else if (op.expr.target.name == "RESULT") {
            xor_stage = op.stage;
        }
    }
    
    // Verify dependencies are respected: AND -> OR -> XOR should have non-decreasing stages
    assert(and_stage <= or_stage);
    assert(or_stage <= xor_stage);

    std::cout << "  ✓ DepthBalancedStages scheduling test passed" << std::endl;
}

void testSchedulingWithRegisters() {
    std::cout << "Testing scheduling with registers..." << std::endl;

    // Create an IR module with both combinational and register operations
    IrValue input1("A", 4);
    IrValue input2("B", 4);
    IrValue output("SUM", 4);
    IrValue reg_input("REG_IN", 4);
    IrValue reg_output("Q", 4);
    std::vector<IrValue> inputs = {input1, input2};
    std::vector<IrValue> outputs = {output, reg_output};

    IrExpr add_expr(IrExprKind::Add, output, {input1, input2});
    std::vector<IrExpr> comb_assigns = {add_expr};
    
    IrExpr reg_expr(IrExprKind::Value, reg_input, {output});  // register input comes from SUM
    IrRegAssign reg_assign(reg_output, reg_expr, "CLK", "RST");
    std::vector<IrRegAssign> reg_assigns = {reg_assign};

    IrModule ir_module("TEST_REGS", inputs, outputs, comb_assigns, reg_assigns);

    // Configure scheduling with 2 stages
    SchedulingConfig config;
    config.strategy = SchedulingStrategy::FixedStageCount;
    config.requested_stages = 2;

    // Perform scheduling
    auto result = SchedulingEngine::BuildSchedule(ir_module, nullptr, nullptr, config);
    assert(result.ok);
    
    ScheduledModule scheduled_module = result.data;
    assert(scheduled_module.num_stages == 2);
    assert(scheduled_module.comb_ops.size() == 1);
    assert(scheduled_module.reg_ops.size() == 1);

    // The register assignment should typically be in the last stage
    assert(scheduled_module.reg_ops[0].stage == 1);  // Last stage (stage 1 of 0-1)

    std::cout << "  ✓ Scheduling with registers test passed" << std::endl;
}

void runAllTests() {
    std::cout << "Running Scheduling Engine tests..." << std::endl;
    std::cout << std::endl;

    testSingleStageScheduling();
    testFixedStageCountScheduling();
    testDepthBalancedStagesScheduling();
    testSchedulingWithRegisters();

    std::cout << std::endl;
    std::cout << "All Scheduling Engine tests passed! ✓" << std::endl;
}

int main() {
    runAllTests();
    return 0;
}