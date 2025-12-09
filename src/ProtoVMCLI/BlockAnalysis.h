#ifndef _ProtoVM_BlockAnalysis_h_
#define _ProtoVM_BlockAnalysis_h_

#include "CircuitGraph.h"
#include "SessionTypes.h"  // For Result<T>
#include "FunctionalAnalysis.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class BlockKind {
    GenericComb,    // generic combinational cluster
    Adder,
    Comparator,
    Mux,
    Decoder,
    Encoder,
    Register,
    Counter,
    Latch,
    // extendable with more semantic types
};

struct BlockPort {
    std::string name;          // logical name (e.g. "A", "B", "SUM", "IN", "OUT")
    std::string direction;     // "in" | "out" | "inout"
    std::vector<std::string> pins;  // underlying pin IDs, e.g. ["C1:A", "C2:A"] or bit-slices

    BlockPort() : name(""), direction("") {}
    BlockPort(const std::string& n, const std::string& d, const std::vector<std::string>& p) 
        : name(n), direction(d), pins(p) {}
};

struct BlockInstance {
    std::string id;                    // unique ID within the circuit
    BlockKind kind;
    std::vector<std::string> components;    // component_ids included in this block
    std::vector<std::string> nets;         // net_ids (optional, for clarity)
    std::vector<BlockPort> ports;      // logical interface

    BlockInstance() : id(""), kind(BlockKind::GenericComb) {}
    BlockInstance(const std::string& i, BlockKind k, const std::vector<std::string>& comps, 
                  const std::vector<std::string>& n, const std::vector<BlockPort>& p) 
        : id(i), kind(k), components(comps), nets(n), ports(p) {}
};

struct BlockGraph {
    std::vector<BlockInstance> blocks;
    // Optional: high-level connectivity between blocks
    // e.g. edges showing which block outputs feed which block inputs
    std::vector<std::string> edges;  // Placeholder for future block-to-block connectivity
};

class BlockAnalysis {
public:
    // Detect all blocks in the given circuit graph.
    Result<BlockGraph> DetectBlocks(
        const CircuitGraph& graph,
        const CircuitData& circuit
    );

private:
    // Helper methods for pattern detection
    Result<std::vector<BlockInstance>> DetectGenericCombinationalBlocks(
        const CircuitGraph& graph,
        const CircuitData& circuit
    );

    Result<std::vector<BlockInstance>> DetectAdders(
        const CircuitGraph& graph,
        const CircuitData& circuit
    );

    Result<std::vector<BlockInstance>> DetectMuxes(
        const CircuitGraph& graph,
        const CircuitData& circuit
    );

    Result<std::vector<BlockInstance>> DetectComparators(
        const CircuitGraph& graph,
        const CircuitData& circuit
    );

    Result<std::vector<BlockInstance>> DetectDecoders(
        const CircuitGraph& graph,
        const CircuitData& circuit
    );

    // Helper to determine if a component is combinational (no internal state)
    bool IsCombinationalComponent(const ComponentData& component) const;

    // Helper to check if a component is a specific type (e.g. "AND", "OR", "XOR", etc.)
    bool IsComponentType(const ComponentData& component, const std::string& type) const;

    // Helper to group connected components into blocks
    std::vector<std::vector<std::string>> FindConnectedComponents(
        const CircuitGraph& graph,
        const std::vector<std::string>& component_ids
    );

    // Helper to classify a cluster of components based on its characteristics
    BlockKind ClassifyBlock(const std::vector<std::string>& component_ids,
                            const CircuitData& circuit) const;

    // Helper to determine block ports based on external connections
    std::vector<BlockPort> DetermineBlockPorts(
        const std::vector<std::string>& component_ids,
        const CircuitGraph& graph
    ) const;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_BlockAnalysis_h_