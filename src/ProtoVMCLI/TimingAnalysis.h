#ifndef _ProtoVM_TimingAnalysis_h_
#define _ProtoVM_TimingAnalysis_h_

#include "CircuitGraph.h"
#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>

namespace ProtoVMCLI {

struct TimingNodeId {
    std::string id;     // e.g. "C1:OUT", "C2:IN"
    
    TimingNodeId() : id("") {}
    TimingNodeId(const std::string& i) : id(i) {}
    
    bool operator==(const TimingNodeId& other) const {
        return id == other.id;
    }
    
    bool operator!=(const TimingNodeId& other) const {
        return !(*this == other);
    }
    
    bool operator<(const TimingNodeId& other) const {
        return id < other.id;
    }
};

struct TimingEdge {
    TimingNodeId from;
    TimingNodeId to;
    
    TimingEdge() {}
    TimingEdge(const TimingNodeId& f, const TimingNodeId& t) : from(f), to(t) {}
};

struct TimingPathPoint {
    TimingNodeId node;
    int depth;  // cumulative depth at this node
};

struct TimingPath {
    std::vector<TimingPathPoint> points;  // ordered from source to sink
    int total_depth;                      // depth at sink
};

struct TimingSummary {
    int max_depth;
    int path_count;
};

struct HazardCandidate {
    std::vector<TimingNodeId> reconvergent_points;  // where signals reconverge
    std::vector<TimingNodeId> sources;              // upstream sources
    std::string description;                        // human-readable description
};

class TimingGraphBuilder {
public:
    // Build timing graph from CircuitGraph
    Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>> 
    BuildTimingGraph(const CircuitGraph& circuit_graph);

private:
    // Helper methods
    void AddTimingNode(std::vector<TimingNodeId>& nodes, const TimingNodeId& node_id);
    void AddTimingEdge(std::vector<TimingEdge>& edges, const TimingNodeId& from, const TimingNodeId& to);
};

class TimingAnalysis {
public:
    // Compute longest paths from all sources to all sinks,
    // or limited to a specific target set.
    Result<std::vector<TimingPath>> ComputeCriticalPaths(
        const std::vector<TimingNodeId>& nodes,
        const std::vector<TimingEdge>& edges,
        int max_paths = 5,        // limit number of returned critical paths
        int max_depth = 1024      // safety cap
    );

    // Compute summary stats like max_depth and number of endpoints.
    Result<TimingSummary> ComputeTimingSummary(
        const std::vector<TimingNodeId>& nodes,
        const std::vector<TimingEdge>& edges,
        int max_depth = 1024
    );

    // Detect combinational cycles / loops.
    Result<std::vector<std::vector<TimingNodeId>>> DetectCombinationalLoops(
        const std::vector<TimingNodeId>& nodes,
        const std::vector<TimingEdge>& edges
    );

    // Detect reconvergent fanout hazard candidates
    Result<std::vector<HazardCandidate>> DetectReconvergentFanoutHazards(
        const std::vector<TimingNodeId>& nodes,
        const std::vector<TimingEdge>& edges,
        int max_results = 64
    );

private:
    // Helper methods for internal use
    std::vector<std::vector<TimingNodeId>> FindPathsFromNode(
        const std::vector<TimingNodeId>& nodes,
        const std::vector<TimingEdge>& edges,
        const TimingNodeId& start_node,
        int max_depth
    );
    
    std::vector<TimingNodeId> FindSources(
        const std::vector<TimingNodeId>& nodes,
        const std::vector<TimingEdge>& edges
    );
    
    std::vector<TimingNodeId> FindSinks(
        const std::vector<TimingNodeId>& nodes,
        const std::vector<TimingEdge>& edges
    );
    
    std::vector<std::vector<TimingNodeId>> FindCycles(
        const std::vector<TimingNodeId>& nodes,
        const std::vector<TimingEdge>& edges
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_TimingAnalysis_h_