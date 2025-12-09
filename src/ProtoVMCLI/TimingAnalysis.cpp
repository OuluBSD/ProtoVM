#include "TimingAnalysis.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <set>

namespace ProtoVMCLI {

void TimingGraphBuilder::AddTimingNode(std::vector<TimingNodeId>& nodes, const TimingNodeId& node_id) {
    // Check if the node already exists
    for (const auto& existing_node : nodes) {
        if (existing_node == node_id) {
            return; // Node already exists
        }
    }

    // Add the new node
    nodes.push_back(node_id);
}

void TimingGraphBuilder::AddTimingEdge(std::vector<TimingEdge>& edges, const TimingNodeId& from, const TimingNodeId& to) {
    // Check if this edge already exists
    for (const auto& existing_edge : edges) {
        if (existing_edge.from == from && existing_edge.to == to) {
            return; // Edge already exists
        }
    }

    // Add the new edge
    edges.push_back(TimingEdge(from, to));
}

Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>> 
TimingGraphBuilder::BuildTimingGraph(const CircuitGraph& circuit_graph) {
    try {
        std::vector<TimingNodeId> nodes;
        std::vector<TimingEdge> edges;

        // Process the CircuitGraph to extract timing-relevant information
        // We focus on SignalFlow edges which represent the actual signal propagation
        
        // Map to convert GraphNodeId to TimingNodeId
        std::unordered_map<GraphNodeId, TimingNodeId> graph_to_timing_node_map;
        
        // First, create timing nodes for all pin nodes in the circuit graph
        for (const auto& graph_node : circuit_graph.nodes) {
            if (graph_node.kind == GraphNodeKind::Pin) {
                // Pin nodes become timing nodes directly
                TimingNodeId timing_node(graph_node.id);
                AddTimingNode(nodes, timing_node);
                graph_to_timing_node_map[graph_node] = timing_node;
            }
        }
        
        // Then, create timing edges based on SignalFlow edges
        for (const auto& graph_edge : circuit_graph.edges) {
            if (graph_edge.kind == GraphEdgeKind::SignalFlow) {
                // Only SignalFlow edges represent actual signal propagation
                auto from_it = graph_to_timing_node_map.find(graph_edge.from);
                auto to_it = graph_to_timing_node_map.find(graph_edge.to);
                
                if (from_it != graph_to_timing_node_map.end() && to_it != graph_to_timing_node_map.end()) {
                    // Create a timing edge between the corresponding timing nodes
                    AddTimingEdge(edges, from_it->second, to_it->second);
                }
            }
        }
        
        return Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>>::MakeOk(
            std::make_pair(nodes, edges));
    }
    catch (const std::exception& e) {
        return Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in TimingGraphBuilder::BuildTimingGraph: ") + e.what()
        );
    }
}

std::vector<TimingNodeId> TimingAnalysis::FindSources(
    const std::vector<TimingNodeId>& nodes,
    const std::vector<TimingEdge>& edges
) {
    std::vector<TimingNodeId> sources;
    
    // Find nodes that have no incoming edges (sources)
    std::unordered_set<TimingNodeId> all_nodes(nodes.begin(), nodes.end());
    std::unordered_set<TimingNodeId> target_nodes;
    
    for (const auto& edge : edges) {
        target_nodes.insert(edge.to);
    }
    
    for (const auto& node : nodes) {
        if (target_nodes.find(node) == target_nodes.end()) {
            sources.push_back(node);
        }
    }
    
    return sources;
}

std::vector<TimingNodeId> TimingAnalysis::FindSinks(
    const std::vector<TimingNodeId>& nodes,
    const std::vector<TimingEdge>& edges
) {
    std::vector<TimingNodeId> sinks;
    
    // Find nodes that have no outgoing edges (sinks)
    std::unordered_set<TimingNodeId> all_nodes(nodes.begin(), nodes.end());
    std::unordered_set<TimingNodeId> source_nodes;
    
    for (const auto& edge : edges) {
        source_nodes.insert(edge.from);
    }
    
    for (const auto& node : nodes) {
        if (source_nodes.find(node) == source_nodes.end()) {
            sinks.push_back(node);
        }
    }
    
    return sinks;
}

std::vector<std::vector<TimingNodeId>> TimingAnalysis::FindCycles(
    const std::vector<TimingNodeId>& nodes,
    const std::vector<TimingEdge>& edges
) {
    std::vector<std::vector<TimingNodeId>> cycles;
    
    // Build adjacency map for traversal
    std::unordered_map<TimingNodeId, std::vector<TimingNodeId>> adjacency_map;
    for (const auto& edge : edges) {
        adjacency_map[edge.from].push_back(edge.to);
    }
    
    // Use DFS to detect cycles
    std::unordered_set<TimingNodeId> visited;
    std::unordered_set<TimingNodeId> rec_stack;  // Recursive stack for DFS
    std::unordered_map<TimingNodeId, TimingNodeId> parent;  // Track parent for path reconstruction
    
    // DFS function to detect cycles
    std::function<void(const TimingNodeId&, std::vector<TimingNodeId>)> dfs = 
        [&](const TimingNodeId& current, std::vector<TimingNodeId> path) {
            if (rec_stack.count(current)) {
                // Cycle detected - find where the cycle starts in our path
                auto it = std::find(path.begin(), path.end(), current);
                if (it != path.end()) {
                    std::vector<TimingNodeId> cycle(it, path.end());
                    cycle.push_back(current);  // Complete the cycle
                    cycles.push_back(cycle);
                }
                return;
            }
            
            if (visited.count(current)) {
                return;
            }
            
            visited.insert(current);
            rec_stack.insert(current);
            path.push_back(current);
            
            for (const auto& neighbor : adjacency_map[current]) {
                dfs(neighbor, path);
            }
            
            rec_stack.erase(current);
        };
    
    for (const auto& node : nodes) {
        if (!visited.count(node)) {
            dfs(node, std::vector<TimingNodeId>());
        }
    }
    
    return cycles;
}

Result<std::vector<TimingPath>> TimingAnalysis::ComputeCriticalPaths(
    const std::vector<TimingNodeId>& nodes,
    const std::vector<TimingEdge>& edges,
    int max_paths,
    int max_depth
) {
    try {
        std::vector<TimingPath> critical_paths;
        
        // Build adjacency map
        std::unordered_map<TimingNodeId, std::vector<TimingNodeId>> adjacency_map;
        for (const auto& edge : edges) {
            adjacency_map[edge.from].push_back(edge.to);
        }
        
        // Find all source nodes
        std::vector<TimingNodeId> sources = FindSources(nodes, edges);
        std::vector<TimingNodeId> sinks = FindSinks(nodes, edges);
        
        // For each source, compute paths to all sinks
        for (const auto& source : sources) {
            // Use BFS or DFS to find all paths from source to all sinks
            std::queue<std::pair<TimingNodeId, std::vector<TimingPathPoint>>> queue;
            std::set<std::string> visited_paths;  // Prevent duplicate paths
            
            // Start with the source node at depth 0
            std::vector<TimingPathPoint> initial_path;
            TimingPathPoint source_point = {source, 0};
            initial_path.push_back(source_point);
            queue.push({source, initial_path});
            
            while (!queue.empty()) {
                auto current = queue.front();
                queue.pop();
                
                const TimingNodeId& current_node = current.first;
                const std::vector<TimingPathPoint>& current_path = current.second;
                
                // If this is a sink, we've found a complete path
                if (std::find(sinks.begin(), sinks.end(), current_node) != sinks.end()) {
                    TimingPath path;
                    path.points = current_path;
                    path.total_depth = current_path.back().depth;
                    critical_paths.push_back(path);
                    
                    // Limit results for performance
                    if (critical_paths.size() >= max_paths) {
                        break;
                    }
                }
                
                // If we've reached max depth, stop this path
                if (current_path.back().depth >= max_depth) {
                    continue;
                }
                
                // Explore neighbors
                for (const auto& neighbor : adjacency_map[current_node]) {
                    // Create new path by extending current path
                    std::vector<TimingPathPoint> new_path = current_path;
                    
                    // Add neighbor with incremented depth
                    TimingPathPoint neighbor_point = {neighbor, current_path.back().depth + 1};
                    new_path.push_back(neighbor_point);
                    
                    // Check if this path already exists (to avoid cycles)
                    std::string path_key;
                    for (const auto& p : new_path) {
                        path_key += p.node.id + ",";
                    }
                    
                    if (visited_paths.find(path_key) == visited_paths.end()) {
                        visited_paths.insert(path_key);
                        queue.push({neighbor, new_path});
                    }
                }
            }
            
            if (critical_paths.size() >= max_paths) {
                break;
            }
        }
        
        // Sort paths by depth in descending order to get critical paths first
        std::sort(critical_paths.begin(), critical_paths.end(),
                  [](const TimingPath& a, const TimingPath& b) {
                      return a.total_depth > b.total_depth;
                  });
        
        // Limit to max_paths
        if (critical_paths.size() > max_paths) {
            critical_paths.resize(max_paths);
        }
        
        return Result<std::vector<TimingPath>>::MakeOk(critical_paths);
    }
    catch (const std::exception& e) {
        return Result<std::vector<TimingPath>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in TimingAnalysis::ComputeCriticalPaths: ") + e.what()
        );
    }
}

Result<TimingSummary> TimingAnalysis::ComputeTimingSummary(
    const std::vector<TimingNodeId>& nodes,
    const std::vector<TimingEdge>& edges,
    int max_depth
) {
    try {
        TimingSummary summary;
        summary.max_depth = 0;
        summary.path_count = 0;
        
        // Find all source nodes
        std::vector<TimingNodeId> sources = FindSources(nodes, edges);
        std::vector<TimingNodeId> sinks = FindSinks(nodes, edges);
        
        if (sources.empty() || sinks.empty()) {
            // If no sources or no sinks, max depth is 0
            return Result<TimingSummary>::MakeOk(summary);
        }
        
        // Build adjacency map
        std::unordered_map<TimingNodeId, std::vector<TimingNodeId>> adjacency_map;
        for (const auto& edge : edges) {
            adjacency_map[edge.from].push_back(edge.to);
        }
        
        // Track max depth found
        int max_found_depth = 0;
        int path_count = 0;
        
        // For each source, compute depths to all other nodes
        std::unordered_map<TimingNodeId, int> node_depths;  // Track depth of each node
        
        for (const auto& source : sources) {
            // BFS to find depths from this source
            std::queue<std::pair<TimingNodeId, int>> queue;  // {node, current_depth}
            std::unordered_set<TimingNodeId> visited;
            
            queue.push({source, 0});
            visited.insert(source);
            node_depths[source] = std::max(node_depths[source], 0);
            
            while (!queue.empty()) {
                auto current = queue.front();
                queue.pop();
                
                const TimingNodeId& current_node = current.first;
                int current_depth = current.second;
                
                if (current_depth >= max_depth) {
                    continue;  // Respect max depth limit
                }
                
                // Explore neighbors
                for (const auto& neighbor : adjacency_map[current_node]) {
                    int new_depth = current_depth + 1;
                    
                    // Update if we found a deeper path to this node
                    if (node_depths[neighbor] < new_depth) {
                        node_depths[neighbor] = new_depth;
                        max_found_depth = std::max(max_found_depth, new_depth);
                    }
                    
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        queue.push({neighbor, new_depth});
                    }
                }
            }
        }
        
        // Count sink nodes (endpoints of paths)
        path_count = sinks.size();
        
        summary.max_depth = max_found_depth;
        summary.path_count = path_count;
        
        return Result<TimingSummary>::MakeOk(summary);
    }
    catch (const std::exception& e) {
        return Result<TimingSummary>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in TimingAnalysis::ComputeTimingSummary: ") + e.what()
        );
    }
}

Result<std::vector<std::vector<TimingNodeId>>> TimingAnalysis::DetectCombinationalLoops(
    const std::vector<TimingNodeId>& nodes,
    const std::vector<TimingEdge>& edges
) {
    try {
        std::vector<std::vector<TimingNodeId>> loops = FindCycles(nodes, edges);
        return Result<std::vector<std::vector<TimingNodeId>>>::MakeOk(loops);
    }
    catch (const std::exception& e) {
        return Result<std::vector<std::vector<TimingNodeId>>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in TimingAnalysis::DetectCombinationalLoops: ") + e.what()
        );
    }
}

Result<std::vector<HazardCandidate>> TimingAnalysis::DetectReconvergentFanoutHazards(
    const std::vector<TimingNodeId>& nodes,
    const std::vector<TimingEdge>& edges,
    int max_results
) {
    try {
        std::vector<HazardCandidate> hazards;
        
        // Build adjacency maps
        std::unordered_map<TimingNodeId, std::vector<TimingNodeId>> forward_adjacency;  // For signal flow
        std::unordered_map<TimingNodeId, std::vector<TimingNodeId>> reverse_adjacency;  // For backward traversal
        
        for (const auto& edge : edges) {
            forward_adjacency[edge.from].push_back(edge.to);
            reverse_adjacency[edge.to].push_back(edge.from);
        }
        
        // For each node, check if it has multiple distinct paths from a common ancestor
        for (const auto& node : nodes) {
            // Find all predecessors of this node using DFS
            std::vector<TimingNodeId> predecessors;
            std::unordered_set<TimingNodeId> visited;
            std::stack<TimingNodeId> stack;
            
            stack.push(node);
            
            while (!stack.empty()) {
                TimingNodeId current = stack.top();
                stack.pop();
                
                if (visited.count(current)) {
                    continue;
                }
                visited.insert(current);
                
                // Add current as a predecessor (if it's not the target node itself)
                if (current != node) {
                    predecessors.push_back(current);
                }
                
                // Add all predecessors to stack
                for (const auto& pred : reverse_adjacency[current]) {
                    if (visited.find(pred) == visited.end()) {
                        stack.push(pred);
                    }
                }
            }
            
            // Check if multiple paths lead to this node from common ancestors
            // This is a simplified heuristic for reconvergent fanout detection
            if (predecessors.size() > 1) {
                // Look for nodes that have paths to multiple nodes that eventually lead to our target
                for (const auto& pred : predecessors) {
                    std::vector<TimingNodeId> forward_paths_from_pred;
                    std::unordered_set<TimingNodeId> visited_forward;
                    std::queue<TimingNodeId> q;
                    
                    q.push(pred);
                    visited_forward.insert(pred);
                    
                    while (!q.empty() && forward_paths_from_pred.size() < 10) { // Limit search
                        TimingNodeId current = q.front();
                        q.pop();
                        
                        if (current == node) {
                            continue;  // Skip the target node itself
                        }
                        
                        // If this path eventually leads to our target node, record it
                        if (current != node) {
                            // Check if there's a path from current to our target node
                            std::unordered_set<TimingNodeId> temp_visited;
                            std::queue<TimingNodeId> temp_q;
                            temp_q.push(current);
                            temp_visited.insert(current);
                            
                            bool reaches_target = false;
                            while (!temp_q.empty()) {
                                TimingNodeId temp_current = temp_q.front();
                                temp_q.pop();
                                
                                if (temp_current == node) {
                                    reaches_target = true;
                                    break;
                                }
                                
                                for (const auto& next : forward_adjacency[temp_current]) {
                                    if (temp_visited.find(next) == temp_visited.end()) {
                                        temp_visited.insert(next);
                                        temp_q.push(next);
                                    }
                                }
                            }
                            
                            if (reaches_target) {
                                forward_paths_from_pred.push_back(current);
                            }
                        }
                        
                        for (const auto& next : forward_adjacency[current]) {
                            if (visited_forward.find(next) == visited_forward.end()) {
                                visited_forward.insert(next);
                                q.push(next);
                            }
                        }
                    }
                    
                    // If we found multiple paths that can reach our target node from a common ancestor,
                    // this indicates potential reconvergent fanout
                    if (forward_paths_from_pred.size() > 1) {
                        HazardCandidate hazard;
                        hazard.sources.push_back(pred);
                        hazard.reconvergent_points.push_back(node);
                        hazard.description = "Potential glitch due to reconvergent fanout from " + 
                                            pred.id + " to " + node.id;
                        
                        hazards.push_back(hazard);
                        
                        if (hazards.size() >= max_results) {
                            return Result<std::vector<HazardCandidate>>::MakeOk(hazards);
                        }
                    }
                }
            }
        }
        
        // Remove duplicates
        std::vector<HazardCandidate> unique_hazards;
        std::set<std::string> seen;
        
        for (const auto& hazard : hazards) {
            std::string key = hazard.description;
            if (seen.find(key) == seen.end()) {
                seen.insert(key);
                unique_hazards.push_back(hazard);
            }
        }
        
        if (unique_hazards.size() > max_results) {
            unique_hazards.resize(max_results);
        }
        
        return Result<std::vector<HazardCandidate>>::MakeOk(unique_hazards);
    }
    catch (const std::exception& e) {
        return Result<std::vector<HazardCandidate>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in TimingAnalysis::DetectReconvergentFanoutHazards: ") + e.what()
        );
    }
}

} // namespace ProtoVMCLI