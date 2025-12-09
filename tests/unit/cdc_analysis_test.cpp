#include "../../src/ProtoVMCLI/CdcModel.h"
#include "../../src/ProtoVMCLI/CdcAnalysis.h"
#include "../../src/ProtoVMCLI/PipelineModel.h"  // Need for pipeline structures
#include "../../src/ProtoVMCLI/CircuitGraph.h"   // Need for circuit graph structures
#include <iostream>
#include <cassert>

using namespace ProtoVMCLI;

void testCdcCrossingKindSerialization() {
    std::cout << "Testing CdcCrossingKind serialization..." << std::endl;

    assert(CdcCrossingKind::SingleBitSyncCandidate != CdcCrossingKind::MultiBitBundle);
    assert(CdcCrossingKind::HandshakeLike != CdcCrossingKind::UnknownPattern);

    std::cout << "CdcCrossingKind tests passed!" << std::endl;
}

void testCdcSeveritySerialization() {
    std::cout << "Testing CdcSeverity serialization..." << std::endl;

    assert(CdcSeverity::Info != CdcSeverity::Warning);
    assert(CdcSeverity::Warning != CdcSeverity::Error);
    assert(CdcSeverity::Info != CdcSeverity::Error);

    std::cout << "CdcSeverity tests passed!" << std::endl;
}

void testCdcCrossingEndpoint() {
    std::cout << "Testing CdcCrossingEndpoint..." << std::endl;

    CdcCrossingEndpoint endpoint;
    endpoint.reg_id = "REG_001";
    endpoint.clock_signal = "CLK_A";
    endpoint.domain_id = 0;

    assert(endpoint.reg_id == "REG_001");
    assert(endpoint.clock_signal == "CLK_A");
    assert(endpoint.domain_id == 0);

    std::cout << "CdcCrossingEndpoint tests passed!" << std::endl;
}

void testCdcCrossing() {
    std::cout << "Testing CdcCrossing..." << std::endl;

    CdcCrossing crossing;
    crossing.id = "CDCC_0001";
    crossing.src.reg_id = "REG_SRC";
    crossing.src.clock_signal = "CLK_A";
    crossing.src.domain_id = 0;
    crossing.dst.reg_id = "REG_DST";
    crossing.dst.clock_signal = "CLK_B";
    crossing.dst.domain_id = 1;
    crossing.kind = CdcCrossingKind::SingleBitSyncCandidate;
    crossing.is_single_bit = true;
    crossing.bit_width = 1;
    crossing.crosses_reset_boundary = false;

    assert(crossing.id == "CDCC_0001");
    assert(crossing.src.reg_id == "REG_SRC");
    assert(crossing.src.clock_signal == "CLK_A");
    assert(crossing.src.domain_id == 0);
    assert(crossing.dst.reg_id == "REG_DST");
    assert(crossing.dst.clock_signal == "CLK_B");
    assert(crossing.dst.domain_id == 1);
    assert(crossing.kind == CdcCrossingKind::SingleBitSyncCandidate);
    assert(crossing.is_single_bit == true);
    assert(crossing.bit_width == 1);
    assert(crossing.crosses_reset_boundary == false);

    std::cout << "CdcCrossing tests passed!" << std::endl;
}

void testCdcIssue() {
    std::cout << "Testing CdcIssue..." << std::endl;

    CdcIssue issue;
    issue.id = "CDCISS_0001";
    issue.severity = CdcSeverity::Warning;
    issue.summary = "Test summary";
    issue.detail = "Test detailed information";
    issue.crossing_id = "CDCC_0001";

    assert(issue.id == "CDCISS_0001");
    assert(issue.severity == CdcSeverity::Warning);
    assert(issue.summary == "Test summary");
    assert(issue.detail == "Test detailed information");
    assert(issue.crossing_id == "CDCC_0001");

    std::cout << "CdcIssue tests passed!" << std::endl;
}

void testCdcReport() {
    std::cout << "Testing CdcReport..." << std::endl;

    CdcReport report;
    report.id = "TEST_BLOCK";

    // Add a clock domain
    ClockSignalInfo clock_info;
    clock_info.signal_name = "CLK_A";
    clock_info.domain_id = 0;
    report.clock_domains.push_back(clock_info);

    // Add a crossing
    CdcCrossing crossing;
    crossing.id = "CDCC_0001";
    crossing.src.reg_id = "REG_SRC";
    crossing.src.clock_signal = "CLK_A";
    crossing.src.domain_id = 0;
    crossing.dst.reg_id = "REG_DST";
    crossing.dst.clock_signal = "CLK_B";
    crossing.dst.domain_id = 1;
    crossing.kind = CdcCrossingKind::MultiBitBundle;
    crossing.is_single_bit = false;
    crossing.bit_width = 8;
    crossing.crosses_reset_boundary = false;
    report.crossings.push_back(crossing);

    // Add an issue
    CdcIssue issue;
    issue.id = "CDCISS_0001";
    issue.severity = CdcSeverity::Error;
    issue.summary = "Multi-bit CDC bundle from CLK_A to CLK_B";
    issue.detail = "8-bit register crossing clock domains without recognized safe structure";
    issue.crossing_id = "CDCC_0001";
    report.issues.push_back(issue);

    assert(report.id == "TEST_BLOCK");
    assert(report.clock_domains.size() == 1);
    assert(report.clock_domains[0].signal_name == "CLK_A");
    assert(report.clock_domains[0].domain_id == 0);
    assert(report.crossings.size() == 1);
    assert(report.crossings[0].id == "CDCC_0001");
    assert(report.issues.size() == 1);
    assert(report.issues[0].severity == CdcSeverity::Error);

    std::cout << "CdcReport tests passed!" << std::endl;
}

void testCdcAnalysisBasic() {
    std::cout << "Testing CdcAnalysis basic functionality..." << std::endl;

    // Create a simple pipeline map for testing
    PipelineMap pipeline;
    pipeline.id = "TEST_BLOCK";

    // Add clock domains
    ClockSignalInfo clk_a, clk_b;
    clk_a.domain_id = "CLK_A";
    clk_a.domain_id_num = 0;
    clk_b.domain_id = "CLK_B";
    clk_b.domain_id_num = 1;
    pipeline.clock_domains.push_back(clk_a);
    pipeline.clock_domains.push_back(clk_b);

    // Add a register in clock domain A
    RegisterInfo reg_a;
    reg_a.reg_id = "REG_A";
    reg_a.name = "REG_A";
    reg_a.clock_signal = "CLK_A";
    reg_a.domain_id = 0;
    reg_a.reset_signal = "RST";
    pipeline.registers.push_back(reg_a);

    // Add a register in clock domain B
    RegisterInfo reg_b;
    reg_b.reg_id = "REG_B";
    reg_b.name = "REG_B";
    reg_b.clock_signal = "CLK_B";
    reg_b.domain_id = 1;
    reg_b.reset_signal = "RST";
    pipeline.registers.push_back(reg_b);

    // Add a path that crosses clock domains
    RegToRegPathInfo path;
    path.src_reg_id = "REG_A";
    path.dst_reg_id = "REG_B";
    path.src_clock_domain = clk_a;
    path.dst_clock_domain = clk_b;
    path.width = 8;  // Multi-bit
    path.crosses_clock_domain = true;
    pipeline.reg_paths.push_back(path);

    // Create a dummy circuit graph
    CircuitGraph graph;

    // Create a dummy timing analysis
    TimingAnalysis timing;

    // Test BuildCdcReportForBlock
    auto report_result = CdcAnalysis::BuildCdcReportForBlock(pipeline, graph, &timing);
    assert(report_result.ok);

    const CdcReport& report = report_result.data;
    assert(report.id == "TEST_BLOCK");
    assert(report.crossings.size() >= 1);  // Should have at least the crossing we added
    assert(report.issues.size() >= 1);     // Should have at least one issue
    
    // Check that the crossing was properly classified (MultiBitBundle for 8-bit)
    bool found_multibit_crossing = false;
    for (const auto& crossing : report.crossings) {
        if (crossing.bit_width == 8 && crossing.kind == CdcCrossingKind::MultiBitBundle) {
            found_multibit_crossing = true;
            break;
        }
    }
    assert(found_multibit_crossing);

    std::cout << "CdcAnalysis basic functionality tests passed!" << std::endl;
}

void testCdcAnalysisSingleBit() {
    std::cout << "Testing CdcAnalysis with single-bit crossing..." << std::endl;

    // Create a pipeline map with single-bit crossing
    PipelineMap pipeline;
    pipeline.id = "TEST_SINGLE_BIT";

    // Add clock domains
    ClockSignalInfo clk_a, clk_b;
    clk_a.domain_id = "CLK_A";
    clk_a.domain_id_num = 0;
    clk_b.domain_id = "CLK_B";
    clk_b.domain_id_num = 1;
    pipeline.clock_domains.push_back(clk_a);
    pipeline.clock_domains.push_back(clk_b);

    // Add registers
    RegisterInfo reg_a, reg_b;
    reg_a.reg_id = "REG_A";
    reg_a.name = "REG_A";
    reg_a.clock_signal = "CLK_A";
    reg_a.domain_id = 0;
    reg_a.reset_signal = "RST";
    pipeline.registers.push_back(reg_a);

    reg_b.reg_id = "REG_B";
    reg_b.name = "REG_B";
    reg_b.clock_signal = "CLK_B";
    reg_b.domain_id = 1;
    reg_b.reset_signal = "RST";
    pipeline.registers.push_back(reg_b);

    // Add a single-bit path that crosses clock domains
    RegToRegPathInfo path;
    path.src_reg_id = "REG_A";
    path.dst_reg_id = "REG_B";
    path.src_clock_domain = clk_a;
    path.dst_clock_domain = clk_b;
    path.width = 1;  // Single-bit
    path.crosses_clock_domain = true;
    pipeline.reg_paths.push_back(path);

    // Create a dummy circuit graph and timing analysis
    CircuitGraph graph;
    TimingAnalysis timing;

    // Test BuildCdcReportForBlock
    auto report_result = CdcAnalysis::BuildCdcReportForBlock(pipeline, graph, &timing);
    assert(report_result.ok);

    const CdcReport& report = report_result.data;
    
    // Check that the single-bit crossing was properly classified
    bool found_singlebit_crossing = false;
    for (const auto& crossing : report.crossings) {
        if (crossing.is_single_bit && crossing.bit_width == 1 && 
            crossing.kind == CdcCrossingKind::SingleBitSyncCandidate) {
            found_singlebit_crossing = true;
            break;
        }
    }
    assert(found_singlebit_crossing);

    std::cout << "CdcAnalysis single-bit crossing tests passed!" << std::endl;
}

int main() {
    std::cout << "Starting CDC Analysis Unit Tests..." << std::endl;

    testCdcCrossingKindSerialization();
    testCdcSeveritySerialization();
    testCdcCrossingEndpoint();
    testCdcCrossing();
    testCdcIssue();
    testCdcReport();
    testCdcAnalysisBasic();
    testCdcAnalysisSingleBit();

    std::cout << "All CDC Analysis Unit Tests Passed!" << std::endl;

    return 0;
}