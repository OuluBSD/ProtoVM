#ifndef _ProtoVM_FunctionalAnalysis_h_
#define _ProtoVM_FunctionalAnalysis_h_

#include "CircuitGraph.h"
#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>
#include <unordered_set>

namespace ProtoVMCLI {

struct FunctionalNodeId {
    std::string id;         // e.g. "C1:OUT" for pins, "C1" for components, "N10" for nets
    std::string kind;       // "Pin", "Component", "Net" (string for JSON-friendliness)

    FunctionalNodeId() : id(""), kind("") {}
    FunctionalNodeId(const std::string& i, const std::string& k) : id(i), kind(k) {}

    bool operator==(const FunctionalNodeId& other) const {
        return id == other.id && kind == other.kind;
    }

    bool operator!=(const FunctionalNodeId& other) const {
        return !(*this == other);
    }

    bool operator<(const FunctionalNodeId& other) const {
        if (id != other.id) {
            return id < other.id;
        }
        return kind < other.kind;
    }
};

struct ConeNode {
    FunctionalNodeId node;
    int depth;                // combinational distance from the root (0 for the root itself, 1 for direct neighbors, etc.)
};

struct FunctionalCone {
    FunctionalNodeId root;
    std::vector<ConeNode> nodes;   // all nodes in the cone, including root or excluding depending on your design (document it)
};

struct DependencySummary {
    FunctionalNodeId root;
    int upstream_count;
    int downstream_count;
};

class FunctionalAnalysis {
public:
    // Compute backward cone (influences) from root.
    Result<FunctionalCone> ComputeBackwardCone(
        const CircuitGraph& graph,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

    // Compute forward cone (impacts) from root.
    Result<FunctionalCone> ComputeForwardCone(
        const CircuitGraph& graph,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

    // Summarize dependency sizes in both directions.
    Result<DependencySummary> ComputeDependencySummary(
        const CircuitGraph& graph,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

private:
    // Helper methods for internal use
    void DFSBackwardCone(
        const CircuitGraph& graph,
        const GraphNodeId& current,
        std::unordered_set<GraphNodeId>& visited,
        std::vector<ConeNode>& result,
        int current_depth,
        int max_depth
    );

    void DFSForwardCone(
        const CircuitGraph& graph,
        const GraphNodeId& current,
        std::unordered_set<GraphNodeId>& visited,
        std::vector<ConeNode>& result,
        int current_depth,
        int max_depth
    );
};

// Helper function to resolve user-provided identifiers to functional/graph nodes
Result<FunctionalNodeId> ResolveFunctionalNode(
    const CircuitGraph& graph,
    const std::string& raw_id,
    const std::string& kind_hint = "" // "Pin", "Component", "Net", or empty
);

// Convert GraphNodeId to FunctionalNodeId
FunctionalNodeId GraphNodeIdToFunctional(const GraphNodeId& graph_node);

// Convert FunctionalNodeId to GraphNodeId
Result<GraphNodeId> FunctionalNodeIdToGraph(const FunctionalNodeId& func_node);

} // namespace ProtoVMCLI

#endif // _ProtoVM_FunctionalAnalysis_h_