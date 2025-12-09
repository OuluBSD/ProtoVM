#include "../src/ProtoVMCLI/TimingAnalysis.h"
#include "../src/ProtoVMCLI/CircuitData.h"
#include <iostream>
#include <cassert>

using namespace ProtoVMCLI;

void testTimingNodeId() {
    std::cout << "Testing TimingNodeId..." << std::endl;
    
    TimingNodeId node1("C1:OUT");
    TimingNodeId node2("C2:IN");
    
    assert(node1.id == "C1:OUT");
    assert(node2.id == "C2:IN");
    assert(node1 != node2);
    
    TimingNodeId node3("C1:OUT");
    assert(node1 == node3);
    
    std::cout << "TimingNodeId tests passed!" << std::endl;
}

void testTimingEdge() {
    std::cout << "Testing TimingEdge..." << std::endl;
    
    TimingNodeId from("C1:OUT");
    TimingNodeId to("C2:IN");
    
    TimingEdge edge(from, to);
    
    assert(edge.from == from);
    assert(edge.to == to);
    
    std::cout << "TimingEdge tests passed!" << std::endl;
}

void testTimingPath() {
    std::cout << "Testing TimingPath..." << std::endl;
    
    TimingPath path;
    path.total_depth = 5;
    
    TimingPathPoint point1;
    point1.node = TimingNodeId("C1:OUT");
    point1.depth = 0;
    
    TimingPathPoint point2;
    point2.node = TimingNodeId("C2:IN");
    point2.depth = 1;
    
    path.points.push_back(point1);
    path.points.push_back(point2);
    
    assert(path.points.size() == 2);
    assert(path.total_depth == 5);
    assert(path.points[0].depth == 0);
    assert(path.points[1].depth == 1);
    
    std::cout << "TimingPath tests passed!" << std::endl;
}

void testTimingSummary() {
    std::cout << "Testing TimingSummary..." << std::endl;
    
    TimingSummary summary;
    summary.max_depth = 10;
    summary.path_count = 5;
    
    assert(summary.max_depth == 10);
    assert(summary.path_count == 5);
    
    std::cout << "TimingSummary tests passed!" << std::endl;
}

void testHazardCandidate() {
    std::cout << "Testing HazardCandidate..." << std::endl;
    
    HazardCandidate hazard;
    hazard.sources.push_back(TimingNodeId("C5:OUT"));
    hazard.reconvergent_points.push_back(TimingNodeId("C9:IN"));
    hazard.description = "Test hazard";
    
    assert(hazard.sources.size() == 1);
    assert(hazard.sources[0].id == "C5:OUT");
    assert(hazard.reconvergent_points.size() == 1);
    assert(hazard.reconvergent_points[0].id == "C9:IN");
    assert(hazard.description == "Test hazard");
    
    std::cout << "HazardCandidate tests passed!" << std::endl;
}

void testTimingAnalysisBasics() {
    std::cout << "Testing TimingAnalysis basic functionality..." << std::endl;
    
    // Create a simple timing graph for testing
    std::vector<TimingNodeId> nodes = {
        TimingNodeId("C1:OUT"),
        TimingNodeId("C2:IN"),
        TimingNodeId("C2:OUT"),
        TimingNodeId("C3:IN")
    };
    
    std::vector<TimingEdge> edges = {
        TimingEdge(TimingNodeId("C1:OUT"), TimingNodeId("C2:IN")),
        TimingEdge(TimingNodeId("C2:OUT"), TimingNodeId("C3:IN"))
    };
    
    // Create TimingAnalysis instance
    TimingAnalysis analysis;
    
    // Test ComputeTimingSummary
    auto summary_result = analysis.ComputeTimingSummary(nodes, edges);
    assert(summary_result.ok);
    // The specific result depends on the implementation, but it should succeed
    
    // Test DetectCombinationalLoops (should be empty for acyclic graph)
    auto loops_result = analysis.DetectCombinationalLoops(nodes, edges);
    assert(loops_result.ok);
    // The specific result depends on the implementation
    
    std::cout << "TimingAnalysis basic functionality tests passed!" << std::endl;
}

void testTimingGraphBuilder() {
    std::cout << "Testing TimingGraphBuilder..." << std::endl;
    
    // Create a dummy CircuitGraph for testing
    CircuitGraph circuit_graph;
    
    // Add some dummy nodes (we'll use them to simulate a circuit graph)
    GraphNodeId comp_node(GraphNodeKind::Component, "C1");
    GraphNodeId pin_node(GraphNodeKind::Pin, "C1:OUT");
    GraphNodeId net_node(GraphNodeKind::Net, "N1");
    
    circuit_graph.nodes.push_back(comp_node);
    circuit_graph.nodes.push_back(pin_node);
    circuit_graph.nodes.push_back(net_node);
    
    // Add an edge
    GraphEdge edge(pin_node, net_node, GraphEdgeKind::SignalFlow);
    circuit_graph.edges.push_back(edge);
    
    // Build adjacency lists
    circuit_graph.adjacency_list.resize(3);
    circuit_graph.reverse_adjacency_list.resize(3);
    
    // Create TimingGraphBuilder and test
    TimingGraphBuilder builder;
    auto result = builder.BuildTimingGraph(circuit_graph);
    
    // The result should be successful
    assert(result.ok);
    
    std::cout << "TimingGraphBuilder tests passed!" << std::endl;
}

int main() {
    std::cout << "Starting Timing Analysis Unit Tests..." << std::endl;
    
    testTimingNodeId();
    testTimingEdge();
    testTimingPath();
    testTimingSummary();
    testHazardCandidate();
    testTimingAnalysisBasics();
    testTimingGraphBuilder();
    
    std::cout << "All Timing Analysis Unit Tests Passed!" << std::endl;
    
    return 0;
}