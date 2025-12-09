#include "../src/ProtoVMCLI/CircuitData.h"
#include "../src/ProtoVMCLI/CircuitGraph.h" 
#include "../src/ProtoVMCLI/TimingAnalysis.h"
#include <iostream>
#include <cassert>

using namespace ProtoVMCLI;

void testSimplePathWithCircuitGraph() {
    std::cout << "Testing timing analysis with simple circuit graph..." << std::endl;
    
    // Create a simple circuit with 2 gates in series: A -> B -> C
    CircuitData circuit;
    
    // Create components
    ComponentData gate_a;
    gate_a.id = CircuitEntityId("C1");
    gate_a.type = "NAND";
    gate_a.name = "GateA";
    gate_a.x = 100;
    gate_a.y = 100;
    
    PinData input_a1, input_a2, output_a;
    input_a1.id = CircuitEntityId("P1");
    input_a1.name = "IN1";
    input_a1.x = 0; input_a1.y = 0;
    input_a1.is_input = true;
    
    input_a2.id = CircuitEntityId("P2");
    input_a2.name = "IN2";
    input_a2.x = 0; input_a2.y = 10;
    input_a2.is_input = true;
    
    output_a.id = CircuitEntityId("P3");
    output_a.name = "OUT";
    output_a.x = 100; output_a.y = 0;
    output_a.is_input = false;
    
    gate_a.inputs.push_back(input_a1);
    gate_a.inputs.push_back(input_a2);
    gate_a.outputs.push_back(output_a);
    
    ComponentData gate_b;
    gate_b.id = CircuitEntityId("C2");
    gate_b.type = "NOR";
    gate_b.name = "GateB";
    gate_b.x = 300;
    gate_b.y = 100;
    
    PinData input_b1, output_b;
    input_b1.id = CircuitEntityId("P4");
    input_b1.name = "IN1";
    input_b1.x = 0; input_b1.y = 0;
    input_b1.is_input = true;
    
    output_b.id = CircuitEntityId("P5");
    output_b.name = "OUT";
    output_b.x = 100; output_b.y = 0;
    output_b.is_input = false;
    
    gate_b.inputs.push_back(input_b1);
    gate_b.outputs.push_back(output_b);
    
    circuit.components.push_back(gate_a);
    circuit.components.push_back(gate_b);
    
    // Create a wire connecting gate A's output to gate B's input
    WireData wire_ab;
    wire_ab.id = CircuitEntityId("W1");
    wire_ab.start_component_id = CircuitEntityId("C1");
    wire_ab.start_pin_name = "OUT";
    wire_ab.end_component_id = CircuitEntityId("C2");
    wire_ab.end_pin_name = "IN1";
    
    circuit.wires.push_back(wire_ab);
    
    // Build circuit graph
    CircuitGraphBuilder graph_builder;
    auto graph_result = graph_builder.BuildGraph(circuit);
    assert(graph_result.ok);
    
    // Convert to timing graph
    TimingGraphBuilder timing_builder;
    auto timing_result = timing_builder.BuildTimingGraph(graph_result.data);
    assert(timing_result.ok);
    
    // Perform timing analysis
    TimingAnalysis analysis;
    auto summary_result = analysis.ComputeTimingSummary(
        timing_result.data.first,  // nodes
        timing_result.data.second  // edges
    );
    assert(summary_result.ok);
    
    // Test critical path analysis
    auto path_result = analysis.ComputeCriticalPaths(
        timing_result.data.first,   // nodes
        timing_result.data.second,  // edges
        5,   // max_paths
        1024 // max_depth
    );
    assert(path_result.ok);
    
    // Test loop detection
    auto loop_result = analysis.DetectCombinationalLoops(
        timing_result.data.first,   // nodes
        timing_result.data.second   // edges
    );
    assert(loop_result.ok);
    
    // Test hazard detection
    auto hazard_result = analysis.DetectReconvergentFanoutHazards(
        timing_result.data.first,   // nodes
        timing_result.data.second,  // edges
        64   // max_results
    );
    assert(hazard_result.ok);
    
    std::cout << "Simple path timing analysis test passed!" << std::endl;
}

void testLoopDetection() {
    std::cout << "Testing loop detection..." << std::endl;
    
    // Create a timing graph with a loop: A -> B -> C -> A
    std::vector<TimingNodeId> nodes = {
        TimingNodeId("A"),
        TimingNodeId("B"), 
        TimingNodeId("C")
    };
    
    std::vector<TimingEdge> edges = {
        TimingEdge(TimingNodeId("A"), TimingNodeId("B")),
        TimingEdge(TimingNodeId("B"), TimingNodeId("C")),
        TimingEdge(TimingNodeId("C"), TimingNodeId("A"))  // This creates a loop
    };
    
    TimingAnalysis analysis;
    auto loop_result = analysis.DetectCombinationalLoops(nodes, edges);
    assert(loop_result.ok);
    
    // We should detect at least one loop
    assert(!loop_result.data.empty());
    
    std::cout << "Loop detection test passed!" << std::endl;
}

int main() {
    std::cout << "Starting Advanced Timing Analysis Tests..." << std::endl;
    
    testSimplePathWithCircuitGraph();
    testLoopDetection();
    
    std::cout << "All Advanced Timing Analysis Tests Passed!" << std::endl;
    
    return 0;
}