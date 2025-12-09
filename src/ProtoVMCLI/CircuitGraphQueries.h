#ifndef _ProtoVM_CircuitGraphQueries_h_
#define _ProtoVM_CircuitGraphQueries_h_

#include "CircuitGraph.h"
#include <vector>

namespace ProtoVMCLI {

struct PathQueryResult {
    std::vector<GraphNodeId> nodes;  // ordered path
};

struct FanQueryResult {
    std::vector<GraphNodeId> endpoints;
};

class CircuitGraphQueries {
public:
    // Find any (or all) paths from source to target
    Result<std::vector<PathQueryResult>> FindSignalPaths(
        const CircuitGraph& graph,
        const GraphNodeId& source,
        const GraphNodeId& target,
        int max_depth = 128
    );

    // Upstream / downstream queries
    Result<FanQueryResult> FindFanIn(
        const CircuitGraph& graph,
        const GraphNodeId& node,
        int max_depth = 128
    );

    Result<FanQueryResult> FindFanOut(
        const CircuitGraph& graph,
        const GraphNodeId& node,
        int max_depth = 128
    );

    // Basic graph stats
    Result<void> ComputeGraphStats(
        const CircuitGraph& graph,
        int& node_count,
        int& edge_count
    );

private:
    // Helper methods
    void DFSPaths(
        const CircuitGraph& graph,
        const GraphNodeId& current,
        const GraphNodeId& target,
        std::vector<GraphNodeId>& current_path,
        std::vector<PathQueryResult>& all_paths,
        std::unordered_set<GraphNodeId>& visited,
        int current_depth,
        int max_depth,
        bool signal_flow_only = true
    );
    
    void DFSCollectUpstream(
        const CircuitGraph& graph,
        const GraphNodeId& current,
        std::unordered_set<GraphNodeId>& visited,
        std::vector<GraphNodeId>& result,
        int current_depth,
        int max_depth
    );
    
    void DFSCollectDownstream(
        const CircuitGraph& graph,
        const GraphNodeId& current,
        std::unordered_set<GraphNodeId>& visited,
        std::vector<GraphNodeId>& result,
        int current_depth,
        int max_depth
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CircuitGraphQueries_h_