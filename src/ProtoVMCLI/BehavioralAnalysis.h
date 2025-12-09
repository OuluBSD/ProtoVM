#ifndef _ProtoVM_BehavioralAnalysis_h_
#define _ProtoVM_BehavioralAnalysis_h_

#include "CircuitGraph.h"
#include "SessionTypes.h"  // For Result<T>
#include "BlockAnalysis.h"
#include "FunctionalAnalysis.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

// A coarse semantic classification of behavior
enum class BehaviorKind {
    Unknown,
    CombinationalLogic,
    Adder,
    Subtractor,
    Comparator,
    EqualityComparator,
    InequalityComparator,
    Mux,
    Decoder,
    Encoder,
    Register,
    Counter,
    StateMachine,
    // extendable with more semantic types
};

// Semantic role for a port in the behavioral context
struct BehaviorPortRole {
    std::string port_name;      // e.g. "A", "B", "SEL", "IN", "OUT", "CLK", "RESET"
    std::string role;           // e.g. "data_in", "data_out", "select", "clock", "reset", "enable", "carry_in", "carry_out"

    BehaviorPortRole() : port_name(""), role("") {}
    BehaviorPortRole(const std::string& name, const std::string& r) : port_name(name), role(r) {}
};

// A structured, machine-readable summary of what a block/subcircuit does
struct BehaviorDescriptor {
    std::string subject_id;             // e.g. block ID or node ID
    std::string subject_kind;           // "Block", "Pin", "Component", "Net"
    BehaviorKind behavior_kind;
    std::vector<BehaviorPortRole> ports; // semantic roles for ports
    int bit_width;                      // inferred width, if applicable (-1 or 0 if unknown)
    std::string description;            // optional human-readable text summary

    BehaviorDescriptor() : subject_id(""), subject_kind(""), behavior_kind(BehaviorKind::Unknown), bit_width(-1), description("") {}
    BehaviorDescriptor(const std::string& id, const std::string& kind, BehaviorKind bkind, 
                       const std::vector<BehaviorPortRole>& p, int width, const std::string& desc)
        : subject_id(id), subject_kind(kind), behavior_kind(bkind), ports(p), bit_width(width), description(desc) {}
};

class BehavioralAnalysis {
public:
    // Infer behavior for a single block instance.
    Result<BehaviorDescriptor> InferBehaviorForBlock(
        const BlockInstance& block,
        const CircuitGraph& graph
    );

    // Infer behavior for an arbitrary node (pin/component/net),
    // potentially by mapping it to a block or using cones.
    Result<BehaviorDescriptor> InferBehaviorForNode(
        const CircuitGraph& graph,
        const FunctionalAnalysis& func,
        const std::string& node_id,
        const std::string& node_kind_hint // "Pin", "Component", "Net" or empty
    );

private:
    // Helper to infer behavior based on BlockKind from BlockAnalysis
    BehaviorKind InferBehaviorKindFromBlockKind(BlockKind block_kind);

    // Helper to determine port roles based on naming patterns and connectivity
    std::vector<BehaviorPortRole> DeterminePortRoles(const std::vector<BlockPort>& block_ports);

    // Helper to infer bit width from port definitions
    int InferBitWidth(const std::vector<BlockPort>& block_ports);

    // Helper to generate description based on behavior kind and port roles
    std::string GenerateDescription(BehaviorKind kind, int bit_width, const std::vector<BehaviorPortRole>& ports);

    // Helper to get behavioral descriptor for unknown/complex nodes
    BehaviorDescriptor GetUnknownBehaviorDescriptor(const std::string& node_id, const std::string& node_kind);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_BehavioralAnalysis_h_