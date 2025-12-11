#ifndef _ProtoVM_CircuitGraph_h_
#define _ProtoVM_CircuitGraph_h_

#include "CircuitData.h"
#include "SessionTypes.h"  // For Result<T>
#include <ProtoVM/ProtoVM.h>
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class GraphNodeKind {
    Component,
    Pin,
    Net
    // extendable later
};

struct GraphNodeId {
    GraphNodeKind kind;
    std::string id;         // component_id, net_id, or composite for pins (e.g. "C42:OUT")

    GraphNodeId() : kind(GraphNodeKind::Component), id("") {}
    GraphNodeId(GraphNodeKind k, const std::string& i) : kind(k), id(i) {}
    
    bool operator==(const GraphNodeId& other) const {
        return kind == other.kind && id == other.id;
    }
    
    bool operator!=(const GraphNodeId& other) const {
        return !(*this == other);
    }
    
    bool operator<(const GraphNodeId& other) const {
        if (kind != other.kind) {
            return static_cast<int>(kind) < static_cast<int>(other.kind);
        }
        return id < other.id;
    }
};

enum class GraphEdgeKind {
    Connectivity,      // e.g. pin <-> net
    SignalFlow         // e.g. output pin -> input pin
};

struct GraphEdge {
    GraphNodeId from;
    GraphNodeId to;
    GraphEdgeKind kind;
    
    GraphEdge() : kind(GraphEdgeKind::Connectivity) {}
    GraphEdge(const GraphNodeId& f, const GraphNodeId& t, GraphEdgeKind k) : from(f), to(t), kind(k) {}
};

struct CircuitGraph {
    std::vector<GraphNodeId> nodes;
    std::vector<GraphEdge> edges;

    // Optional: adjacency maps for faster queries
    std::vector<std::vector<size_t>> adjacency_list;  // index-based for efficiency
    std::vector<std::vector<size_t>> reverse_adjacency_list;  // for backward traversal
};

class CircuitGraphBuilder {
public:
    Result<CircuitGraph> BuildGraph(const CircuitData& circuit);
    
private:
    // Helper methods
    void AddNode(CircuitGraph& graph, const GraphNodeId& node_id);
    void AddEdge(CircuitGraph& graph, const GraphNodeId& from, const GraphNodeId& to, GraphEdgeKind kind);
    
    // Create the adjacency lists for efficient traversal
    void BuildAdjacencyLists(CircuitGraph& graph);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CircuitGraph_h_