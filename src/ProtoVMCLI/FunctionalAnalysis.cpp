#include "FunctionalAnalysis.h"
#include <unordered_set>
#include <unordered_map>

namespace ProtoVMCLI {

FunctionalNodeId GraphNodeIdToFunctional(const GraphNodeId& graph_node) {
    std::string kind_str;
    switch (graph_node.kind) {
        case GraphNodeKind::Component:
            kind_str = "Component";
            break;
        case GraphNodeKind::Pin:
            kind_str = "Pin";
            break;
        case GraphNodeKind::Net:
            kind_str = "Net";
            break;
    }
    return FunctionalNodeId(graph_node.id, kind_str);
}

Result<GraphNodeId> FunctionalNodeIdToGraph(const FunctionalNodeId& func_node) {
    GraphNodeKind kind;
    if (func_node.kind == "Component") {
        kind = GraphNodeKind::Component;
    } else if (func_node.kind == "Pin") {
        kind = GraphNodeKind::Pin;
    } else if (func_node.kind == "Net") {
        kind = GraphNodeKind::Net;
    } else {
        return Result<GraphNodeId>::MakeError(
            ErrorCode::InvalidArgument,
            "Invalid FunctionalNodeId kind: " + func_node.kind
        );
    }
    
    return Result<GraphNodeId>::MakeOk(GraphNodeId(kind, func_node.id));
}

Result<FunctionalNodeId> ResolveFunctionalNode(
    const CircuitGraph& graph,
    const std::string& raw_id,
    const std::string& kind_hint
) {
    // If kind_hint is provided, search for a node with matching kind and ID
    if (!kind_hint.empty()) {
        GraphNodeKind kind;
        if (kind_hint == "Component") {
            kind = GraphNodeKind::Component;
        } else if (kind_hint == "Pin") {
            kind = GraphNodeKind::Pin;
        } else if (kind_hint == "Net") {
            kind = GraphNodeKind::Net;
        } else {
            return Result<FunctionalNodeId>::MakeError(
                ErrorCode::InvalidArgument,
                "Invalid kind hint: " + kind_hint
            );
        }

        // Search for the specific node
        for (const auto& node : graph.nodes) {
            if (node.kind == kind && node.id == raw_id) {
                return Result<FunctionalNodeId>::MakeOk(GraphNodeIdToFunctional(node));
            }
        }

        return Result<FunctionalNodeId>::MakeError(
            ErrorCode::InvalidArgument,
            "Node not found with kind '" + kind_hint + "' and id '" + raw_id + "'"
        );
    }

    // If no kind_hint is provided, try to infer based on patterns:
    // Contains ':' → treat as Pin
    if (raw_id.find(':') != std::string::npos) {
        // Look for a pin node with this ID
        for (const auto& node : graph.nodes) {
            if (node.kind == GraphNodeKind::Pin && node.id == raw_id) {
                return Result<FunctionalNodeId>::MakeOk(GraphNodeIdToFunctional(node));
            }
        }
        
        return Result<FunctionalNodeId>::MakeError(
            ErrorCode::InvalidArgument,
            "Pin node not found: " + raw_id
        );
    }

    // Otherwise try Component, then Net, in that order
    // First try Component
    for (const auto& node : graph.nodes) {
        if (node.kind == GraphNodeKind::Component && node.id == raw_id) {
            return Result<FunctionalNodeId>::MakeOk(GraphNodeIdToFunctional(node));
        }
    }

    // Then try Net
    for (const auto& node : graph.nodes) {
        if (node.kind == GraphNodeKind::Net && node.id == raw_id) {
            return Result<FunctionalNodeId>::MakeOk(GraphNodeIdToFunctional(node));
        }
    }

    return Result<FunctionalNodeId>::MakeError(
        ErrorCode::InvalidArgument,
        "Node not found: " + raw_id
    );
}

void FunctionalAnalysis::DFSBackwardCone(
    const CircuitGraph& graph,
    const GraphNodeId& current,
    std::unordered_set<GraphNodeId>& visited,
    std::vector<ConeNode>& result,
    int current_depth,
    int max_depth
) {
    // Check depth limit
    if (current_depth >= max_depth) {
        return;
    }

    // Mark current node as visited
    if (visited.count(current) > 0) {
        return; // Already processed
    }
    visited.insert(current);

    // Add the current node to result (excluding root, or including depending on design)
    // In this implementation, we include all nodes in the traversal including root (if it's not the initial call)
    if (current_depth > 0) {  // Exclude the root from the cone result
        ConeNode cone_node;
        cone_node.node = GraphNodeIdToFunctional(current);
        cone_node.depth = current_depth;
        result.push_back(cone_node);
    }

    // Find the node index in the graph
    size_t current_idx = static_cast<size_t>(-1);
    for (size_t i = 0; i < graph.nodes.size(); ++i) {
        if (graph.nodes[i] == current) {
            current_idx = i;
            break;
        }
    }

    if (current_idx == static_cast<size_t>(-1)) {
        return; // Node not found in graph
    }

    // Traverse reverse adjacent nodes (following signal flow backwards)
    const auto& reverse_adjacent_edges = graph.reverse_adjacency_list[current_idx];
    for (size_t edge_idx : reverse_adjacent_edges) {
        const GraphEdge& edge = graph.edges[edge_idx];

        // Only follow signal flow edges in reverse (backward cone)
        if (edge.kind == GraphEdgeKind::SignalFlow) {
            DFSBackwardCone(graph, edge.from, visited, result,
                           current_depth + 1, max_depth);
        }
    }
}

void FunctionalAnalysis::DFSForwardCone(
    const CircuitGraph& graph,
    const GraphNodeId& current,
    std::unordered_set<GraphNodeId>& visited,
    std::vector<ConeNode>& result,
    int current_depth,
    int max_depth
) {
    // Check depth limit
    if (current_depth >= max_depth) {
        return;
    }

    // Mark current node as visited
    if (visited.count(current) > 0) {
        return; // Already processed
    }
    visited.insert(current);

    // Add the current node to result (excluding root, or including depending on design)
    // In this implementation, we include all nodes in the traversal including root (if it's not the initial call)
    if (current_depth > 0) {  // Exclude the root from the cone result
        ConeNode cone_node;
        cone_node.node = GraphNodeIdToFunctional(current);
        cone_node.depth = current_depth;
        result.push_back(cone_node);
    }

    // Find the node index in the graph
    size_t current_idx = static_cast<size_t>(-1);
    for (size_t i = 0; i < graph.nodes.size(); ++i) {
        if (graph.nodes[i] == current) {
            current_idx = i;
            break;
        }
    }

    if (current_idx == static_cast<size_t>(-1)) {
        return; // Node not found in graph
    }

    // Traverse adjacent nodes (following signal flow forward)
    const auto& adjacent_edges = graph.adjacency_list[current_idx];
    for (size_t edge_idx : adjacent_edges) {
        const GraphEdge& edge = graph.edges[edge_idx];

        // Only follow signal flow edges forward (forward cone)
        if (edge.kind == GraphEdgeKind::SignalFlow) {
            DFSForwardCone(graph, edge.to, visited, result,
                          current_depth + 1, max_depth);
        }
    }
}

Result<FunctionalCone> FunctionalAnalysis::ComputeBackwardCone(
    const CircuitGraph& graph,
    const FunctionalNodeId& root,
    int max_depth
) {
    try {
        FunctionalCone result;
        result.root = root;

        // Convert FunctionalNodeId to GraphNodeId
        auto graph_node_result = FunctionalNodeIdToGraph(root);
        if (!graph_node_result.ok) {
            return Result<FunctionalCone>::MakeError(
                graph_node_result.error_code,
                graph_node_result.error_message
            );
        }
        
        GraphNodeId graph_node = graph_node_result.data;

        // Check if node exists in the graph
        bool node_exists = false;
        for (const auto& graph_node_iter : graph.nodes) {
            if (graph_node_iter == graph_node) {
                node_exists = true;
                break;
            }
        }

        if (!node_exists) {
            return Result<FunctionalCone>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Node does not exist in graph: " + graph_node.id
            );
        }

        std::unordered_set<GraphNodeId> visited;
        std::vector<ConeNode> nodes;
        
        // Start traversal from the root node, but don't include it in the result
        // The root is the starting point, so we call the DFS with depth 0
        DFSBackwardCone(graph, graph_node, visited, nodes, 0, max_depth);

        result.nodes = nodes;

        return Result<FunctionalCone>::MakeOk(result);
    }
    catch (const std::exception& e) {
        return Result<FunctionalCone>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ComputeBackwardCone: ") + e.what()
        );
    }
}

Result<FunctionalCone> FunctionalAnalysis::ComputeForwardCone(
    const CircuitGraph& graph,
    const FunctionalNodeId& root,
    int max_depth
) {
    try {
        FunctionalCone result;
        result.root = root;

        // Convert FunctionalNodeId to GraphNodeId
        auto graph_node_result = FunctionalNodeIdToGraph(root);
        if (!graph_node_result.ok) {
            return Result<FunctionalCone>::MakeError(
                graph_node_result.error_code,
                graph_node_result.error_message
            );
        }
        
        GraphNodeId graph_node = graph_node_result.data;

        // Check if node exists in the graph
        bool node_exists = false;
        for (const auto& graph_node_iter : graph.nodes) {
            if (graph_node_iter == graph_node) {
                node_exists = true;
                break;
            }
        }

        if (!node_exists) {
            return Result<FunctionalCone>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Node does not exist in graph: " + graph_node.id
            );
        }

        std::unordered_set<GraphNodeId> visited;
        std::vector<ConeNode> nodes;
        
        // Start traversal from the root node, but don't include it in the result
        // The root is the starting point, so we call the DFS with depth 0
        DFSForwardCone(graph, graph_node, visited, nodes, 0, max_depth);

        result.nodes = nodes;

        return Result<FunctionalCone>::MakeOk(result);
    }
    catch (const std::exception& e) {
        return Result<FunctionalCone>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ComputeForwardCone: ") + e.what()
        );
    }
}

Result<DependencySummary> FunctionalAnalysis::ComputeDependencySummary(
    const CircuitGraph& graph,
    const FunctionalNodeId& root,
    int max_depth
) {
    try {
        DependencySummary result;
        result.root = root;

        // Compute both backward and forward cones
        auto backward_result = ComputeBackwardCone(graph, root, max_depth);
        if (!backward_result.ok) {
            return Result<DependencySummary>::MakeError(
                backward_result.error_code,
                backward_result.error_message
            );
        }

        auto forward_result = ComputeForwardCone(graph, root, max_depth);
        if (!forward_result.ok) {
            return Result<DependencySummary>::MakeError(
                forward_result.error_code,
                forward_result.error_message
            );
        }

        result.upstream_count = static_cast<int>(backward_result.data.nodes.size());
        result.downstream_count = static_cast<int>(forward_result.data.nodes.size());

        return Result<DependencySummary>::MakeOk(result);
    }
    catch (const std::exception& e) {
        return Result<DependencySummary>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ComputeDependencySummary: ") + e.what()
        );
    }
}

} // namespace ProtoVMCLI