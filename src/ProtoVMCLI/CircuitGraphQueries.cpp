#include "CircuitGraphQueries.h"
#include <unordered_set>
#include <unordered_map>

namespace ProtoVMCLI {

void CircuitGraphQueries::DFSPaths(
    const CircuitGraph& graph,
    const GraphNodeId& current,
    const GraphNodeId& target,
    std::vector<GraphNodeId>& current_path,
    std::vector<PathQueryResult>& all_paths,
    std::unordered_set<GraphNodeId>& visited,
    int current_depth,
    int max_depth,
    bool signal_flow_only
) {
    // Check if we've reached the target
    if (current == target) {
        PathQueryResult result;
        result.nodes = current_path;
        all_paths.push_back(result);
        return; // Don't return early if we want all paths, but for now just return first
    }
    
    // Check depth limit
    if (current_depth >= max_depth) {
        return;
    }
    
    // Mark current node as visited
    visited.insert(current);
    
    // Find the node index in the graph
    size_t current_idx = static_cast<size_t>(-1);
    for (size_t i = 0; i < graph.nodes.size(); ++i) {
        if (graph.nodes[i] == current) {
            current_idx = i;
            break;
        }
    }
    
    if (current_idx == static_cast<size_t>(-1)) {
        visited.erase(current);
        return; // Node not found in graph
    }
    
    // Traverse adjacent nodes
    const auto& adjacent_edges = graph.adjacency_list[current_idx];
    for (size_t edge_idx : adjacent_edges) {
        const GraphEdge& edge = graph.edges[edge_idx];
        
        // If signal flow only is required, only follow signal flow edges
        if (signal_flow_only && edge.kind != GraphEdgeKind::SignalFlow) {
            continue;
        }
        
        // If node hasn't been visited, continue DFS
        if (visited.find(edge.to) == visited.end()) {
            current_path.push_back(edge.to);
            DFSPaths(graph, edge.to, target, current_path, all_paths, visited, 
                     current_depth + 1, max_depth, signal_flow_only);
            current_path.pop_back(); // backtrack
        }
    }
    
    // Unmark current node as visited for other search paths
    visited.erase(current);
}

void CircuitGraphQueries::DFSCollectUpstream(
    const CircuitGraph& graph,
    const GraphNodeId& current,
    std::unordered_set<GraphNodeId>& visited,
    std::vector<GraphNodeId>& result,
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
    
    // Add the current node to result (only if it's a potential endpoint, like a pin)
    if (current.kind == GraphNodeKind::Pin) {
        result.push_back(current);
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
        
        // Only follow signal flow edges in reverse
        if (edge.kind == GraphEdgeKind::SignalFlow) {
            DFSCollectUpstream(graph, edge.from, visited, result, 
                              current_depth + 1, max_depth);
        }
    }
}

void CircuitGraphQueries::DFSCollectDownstream(
    const CircuitGraph& graph,
    const GraphNodeId& current,
    std::unordered_set<GraphNodeId>& visited,
    std::vector<GraphNodeId>& result,
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
    
    // Add the current node to result (only if it's a potential endpoint, like a pin)
    if (current.kind == GraphNodeKind::Pin) {
        result.push_back(current);
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
        
        // Only follow signal flow edges forward
        if (edge.kind == GraphEdgeKind::SignalFlow) {
            DFSCollectDownstream(graph, edge.to, visited, result, 
                                current_depth + 1, max_depth);
        }
    }
}

Result<std::vector<PathQueryResult>> CircuitGraphQueries::FindSignalPaths(
    const CircuitGraph& graph,
    const GraphNodeId& source,
    const GraphNodeId& target,
    int max_depth
) {
    try {
        std::vector<PathQueryResult> all_paths;
        
        // Check if source and target nodes exist in the graph
        bool source_exists = false;
        bool target_exists = false;
        
        for (const auto& node : graph.nodes) {
            if (node == source) source_exists = true;
            if (node == target) target_exists = true;
        }
        
        if (!source_exists) {
            return Result<std::vector<PathQueryResult>>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Source node does not exist in graph: " + source.id
            );
        }
        
        if (!target_exists) {
            return Result<std::vector<PathQueryResult>>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Target node does not exist in graph: " + target.id
            );
        }
        
        // Perform DFS to find paths
        std::vector<GraphNodeId> current_path;
        current_path.push_back(source);
        
        std::unordered_set<GraphNodeId> visited;
        
        if (source == target) {
            // Special case: source and target are the same
            PathQueryResult result;
            result.nodes.push_back(source);
            all_paths.push_back(result);
        } else {
            DFSPaths(graph, source, target, current_path, all_paths, 
                    visited, 0, max_depth, true);  // signal_flow_only = true
        }
        
        return Result<std::vector<PathQueryResult>>::MakeOk(all_paths);
    }
    catch (const std::exception& e) {
        return Result<std::vector<PathQueryResult>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in FindSignalPaths: ") + e.what()
        );
    }
}

Result<FanQueryResult> CircuitGraphQueries::FindFanIn(
    const CircuitGraph& graph,
    const GraphNodeId& node,
    int max_depth
) {
    try {
        FanQueryResult result;
        
        // Check if node exists in the graph
        bool node_exists = false;
        for (const auto& graph_node : graph.nodes) {
            if (graph_node == node) {
                node_exists = true;
                break;
            }
        }
        
        if (!node_exists) {
            return Result<FanQueryResult>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Node does not exist in graph: " + node.id
            );
        }
        
        std::unordered_set<GraphNodeId> visited;
        DFSCollectUpstream(graph, node, visited, result.endpoints, 0, max_depth);
        
        return Result<FanQueryResult>::MakeOk(result);
    }
    catch (const std::exception& e) {
        return Result<FanQueryResult>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in FindFanIn: ") + e.what()
        );
    }
}

Result<FanQueryResult> CircuitGraphQueries::FindFanOut(
    const CircuitGraph& graph,
    const GraphNodeId& node,
    int max_depth
) {
    try {
        FanQueryResult result;
        
        // Check if node exists in the graph
        bool node_exists = false;
        for (const auto& graph_node : graph.nodes) {
            if (graph_node == node) {
                node_exists = true;
                break;
            }
        }
        
        if (!node_exists) {
            return Result<FanQueryResult>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Node does not exist in graph: " + node.id
            );
        }
        
        std::unordered_set<GraphNodeId> visited;
        DFSCollectDownstream(graph, node, visited, result.endpoints, 0, max_depth);
        
        return Result<FanQueryResult>::MakeOk(result);
    }
    catch (const std::exception& e) {
        return Result<FanQueryResult>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in FindFanOut: ") + e.what()
        );
    }
}

Result<void> CircuitGraphQueries::ComputeGraphStats(
    const CircuitGraph& graph,
    int& node_count,
    int& edge_count
) {
    try {
        node_count = static_cast<int>(graph.nodes.size());
        edge_count = static_cast<int>(graph.edges.size());
        
        return Result<void>::MakeOk();
    }
    catch (const std::exception& e) {
        return Result<void>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ComputeGraphStats: ") + e.what()
        );
    }
}

} // namespace ProtoVMCLI