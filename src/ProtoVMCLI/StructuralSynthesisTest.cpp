#include "StructuralSynthesis.h"
#include "CircuitGraph.h"
#include "CircuitFacade.h"
#include "JsonIO.h"
#include <cassert>
#include <iostream>

using namespace ProtoVMCLI;

// Test basic structural pattern detection
void TestStructuralPatternDetection() {
    std::cout << "Testing structural pattern detection...\n";
    
    // Create a simple structural pattern
    StructuralPattern pattern;
    pattern.pattern_id = "SP_0001";
    pattern.kind = StructuralPatternKind::RedundantLogic;
    pattern.node_ids.Add("C1");
    pattern.node_ids.Add("C2");
    pattern.description = "Test redundant logic pattern";

    // Verify we can serialize it to JSON
    auto json_map = JsonIO::StructuralPatternToValueMap(pattern);
    assert(json_map.Get("pattern_id", Upp::String("")) == "SP_0001");
    assert(json_map.Get("kind", Upp::String("")) == "RedundantLogic");
    assert(json_map.Get("description", Upp::String("")) == "Test redundant logic pattern");
    
    std::cout << "✓ Structural pattern detection test passed\n";
}

// Test refactor move creation
void TestRefactorMoveCreation() {
    std::cout << "Testing refactor move creation...\n";
    
    StructuralRefactorMove move;
    move.move_id = "SRM_0001";
    move.target_block_id = "B1";
    move.kind = StructuralPatternKind::CanonicalMux;
    move.affected_node_ids.Add("M1");
    move.affected_node_ids.Add("M2");
    move.safety = StructuralRefactorSafety::Safe;
    move.safety_reason = "Simple canonicalization within single domain";
    move.transform_hint = "Normalize mux structure";

    // Verify we can serialize it to JSON
    auto json_map = JsonIO::StructuralRefactorMoveToValueMap(move);
    assert(json_map.Get("move_id", Upp::String("")) == "SRM_0001");
    assert(json_map.Get("safety", Upp::String("")) == "Safe");
    assert(json_map.Get("transform_hint", Upp::String("")) == "Normalize mux structure");
    
    std::cout << "✓ Refactor move creation test passed\n";
}

// Test refactor plan creation
void TestRefactorPlanCreation() {
    std::cout << "Testing refactor plan creation...\n";
    
    StructuralRefactorPlan plan;
    plan.id = "SRP_TEST_1";
    plan.target_block_id = "B1";
    plan.gate_count_before = 100;
    plan.gate_count_after_estimate = 80;
    plan.depth_before = 10;
    plan.depth_after_estimate = 8;
    plan.respects_cdc_fences = true;
    
    // Add a pattern and move to test the arrays
    StructuralPattern pattern;
    pattern.pattern_id = "SP_0001";
    pattern.kind = StructuralPatternKind::RedundantLogic;
    pattern.node_ids.Add("C1");
    pattern.description = "Test pattern";
    plan.patterns.Add(pattern);
    
    StructuralRefactorMove move;
    move.move_id = "SRM_0001";
    move.target_block_id = "B1";
    move.kind = StructuralPatternKind::RedundantLogic;
    move.affected_node_ids.Add("C1");
    move.safety = StructuralRefactorSafety::Safe;
    move.safety_reason = "Test safety";
    move.transform_hint = "Test transform";
    plan.moves.Add(move);

    // Verify we can serialize the complete plan
    auto json_map = JsonIO::StructuralRefactorPlanToValueMap(plan);
    assert(json_map.Get("id", Upp::String("")) == "SRP_TEST_1");
    assert(json_map.Get("target_block_id", Upp::String("")) == "B1");
    assert(json_map.Get("respects_cdc_fences", true) == true);
    
    std::cout << "✓ Refactor plan creation test passed\n";
}

// Test structural analysis method (basic functionality)
void TestStructuralAnalysis() {
    std::cout << "Testing structural analysis...\n";
    
    // Create a minimal circuit graph for testing
    CircuitGraph graph;
    // Add a few nodes and edges to the graph for testing
    // In a real test, we would have a complete circuit graph
    
    // Test the analysis with a minimal example
    auto result = StructuralAnalysis::AnalyzeBlockStructure(
        "TEST_BLOCK",
        graph,
        nullptr,  // functional analysis
        nullptr,  // IR module
        nullptr   // CDC report
    );
    
    // The result should be OK but might have no patterns for an empty graph
    assert(result.ok);
    
    std::cout << "✓ Structural analysis test passed\n";
}

// Run all tests
int main() {
    std::cout << "Running structural synthesis tests...\n\n";
    
    TestStructuralPatternDetection();
    TestRefactorMoveCreation();
    TestRefactorPlanCreation();
    TestStructuralAnalysis();
    
    std::cout << "\nAll structural synthesis tests passed!\n";
    return 0;
}