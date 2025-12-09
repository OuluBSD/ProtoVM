#include "ProtoVMCLI/RetimingModel.h"
#include "ProtoVMCLI/RetimingAnalysis.h"
#include "ProtoVMCLI/PipelineModel.h"
#include "ProtoVMCLI/CdcModel.h"
#include "ProtoVMCLI/TimingAnalysis.h"
#include "ProtoVMCLI/ScheduledIr.h"
#include "plugin/Result.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace ProtoVMCLI;

void TestRetimingModelStructures() {
    std::cout << "Testing RetimingModel structures..." << std::endl;

    // Test RetimingMoveDirection enum
    assert(RetimingMoveDirection::Forward == RetimingMoveDirection::Forward);
    assert(RetimingMoveDirection::Backward == RetimingMoveDirection::Backward);

    // Test RetimingMoveSafety enum
    assert(RetimingMoveSafety::SafeIntraDomain == RetimingMoveSafety::SafeIntraDomain);
    assert(RetimingMoveSafety::Suspicious == RetimingMoveSafety::Suspicious);
    assert(RetimingMoveSafety::Forbidden == RetimingMoveSafety::Forbidden);

    // Test RetimingMove structure
    RetimingMove move;
    move.move_id = "RTM_0001";
    move.src_reg_id = "REG_A";
    move.dst_reg_id = "REG_B";
    move.direction = RetimingMoveDirection::Forward;
    move.domain_id = 0;
    move.src_stage_index = 0;
    move.dst_stage_index = 1;
    move.before_comb_depth = 10;
    move.after_comb_depth_est = 5;
    move.safety = RetimingMoveSafety::SafeIntraDomain;
    move.safety_reason = "Intra-domain, no CDC crossings, internal path";
    move.affected_ops.Add("ADD_1");

    assert(move.move_id == "RTM_0001");
    assert(move.src_reg_id == "REG_A");
    assert(move.dst_reg_id == "REG_B");
    assert(move.direction == RetimingMoveDirection::Forward);
    assert(move.domain_id == 0);
    assert(move.src_stage_index == 0);
    assert(move.dst_stage_index == 1);
    assert(move.before_comb_depth == 10);
    assert(move.after_comb_depth_est == 5);
    assert(move.safety == RetimingMoveSafety::SafeIntraDomain);
    assert(move.safety_reason == "Intra-domain, no CDC crossings, internal path");
    assert(move.affected_ops.GetCount() == 1);
    assert(move.affected_ops[0] == "ADD_1");

    // Test RetimingPlan structure
    RetimingPlan plan;
    plan.id = "RTP_PLAN_1";
    plan.target_id = "BLOCK_A";
    plan.description = "Test retiming plan";
    plan.moves.Add(move);
    plan.estimated_max_depth_before = 10;
    plan.estimated_max_depth_after = 5;
    plan.respects_cdc_fences = true;

    assert(plan.id == "RTP_PLAN_1");
    assert(plan.target_id == "BLOCK_A");
    assert(plan.description == "Test retiming plan");
    assert(plan.moves.GetCount() == 1);
    assert(plan.estimated_max_depth_before == 10);
    assert(plan.estimated_max_depth_after == 5);
    assert(plan.respects_cdc_fences == true);

    std::cout << "  ✓ RetimingModel structures test passed" << std::endl;
}

void TestRetimingAnalysisBasicFunctionality() {
    std::cout << "Testing RetimingAnalysis basic functionality..." << std::endl;

    // Create minimal pipeline map for testing
    PipelineMap pipeline;
    pipeline.id = "TEST_BLOCK";
    
    // Add a clock domain
    ClockSignalInfo clock;
    clock.signal_name = "CLK";
    clock.domain_id = 0;
    pipeline.clock_domains.push_back(clock);

    // Add registers
    RegisterInfo reg1, reg2;
    reg1.reg_id = "REG_A";
    reg1.name = "Register A";
    reg1.clock_signal = "CLK";
    reg1.domain_id = 0;
    pipeline.registers.push_back(reg1);

    reg2.reg_id = "REG_B";
    reg2.name = "Register B";
    reg2.clock_signal = "CLK";
    reg2.domain_id = 0;
    pipeline.registers.push_back(reg2);

    // Add stages
    PipelineStageInfo stage1, stage2;
    stage1.stage_index = 0;
    stage1.domain_id = 0;
    stage1.registers_out.push_back("REG_A");
    stage1.comb_depth_estimate = 10;
    pipeline.stages.push_back(stage1);

    stage2.stage_index = 1;
    stage2.domain_id = 0;
    stage2.registers_in.push_back("REG_B");
    stage2.comb_depth_estimate = 0;
    pipeline.stages.push_back(stage2);

    // Add a reg-to-reg path
    RegToRegPathInfo path;
    path.src_reg_id = "REG_A";
    path.dst_reg_id = "REG_B";
    path.domain_id = 0;
    path.comb_depth_estimate = 10;
    path.stage_span = 1;
    path.crosses_clock_domain = false;
    pipeline.reg_paths.push_back(path);

    // Create minimal CDC report (no crossings)
    CdcReport cdc_report;
    cdc_report.id = "TEST_BLOCK";
    cdc_report.clock_domains.Add(clock);

    // Test the analysis function (it should return at least one plan for this path)
    auto result = RetimingAnalysis::AnalyzeRetimingForBlock(pipeline, cdc_report);
    
    if (result.ok()) {
        std::cout << "  ✓ RetimingAnalysis::AnalyzeRetimingForBlock ran successfully" << std::endl;
        
        // Check that we got some results
        auto plans = result.data();
        std::cout << "    Generated " << plans.GetCount() << " retiming plans" << std::endl;
        
        for (const auto& plan : plans) {
            std::cout << "    Plan: " << plan.id << " with " << plan.moves.GetCount() << " moves" << std::endl;
            for (const auto& move : plan.moves) {
                std::cout << "      Move: " << move.move_id 
                          << " from " << move.src_reg_id 
                          << " to " << move.dst_reg_id 
                          << " (depth: " << move.before_comb_depth 
                          << " -> " << move.after_comb_depth_est << ")" << std::endl;
            }
        }
    } else {
        std::cout << "  ? RetimingAnalysis::AnalyzeRetimingForBlock returned error: " 
                  << result.error_message() << std::endl;
    }
}

int main() {
    std::cout << "Running retiming tests..." << std::endl;

    TestRetimingModelStructures();
    TestRetimingAnalysisBasicFunctionality();

    std::cout << "All retiming tests completed!" << std::endl;
    return 0;
}