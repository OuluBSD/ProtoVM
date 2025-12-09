#include "BehavioralAnalysis.h"
#include "CircuitGraph.h"
#include "BlockAnalysis.h"
#include "FunctionalAnalysis.h"
#include "JsonIO.h"

#include <algorithm>
#include <unordered_map>

namespace ProtoVMCLI {

Result<BehaviorDescriptor> BehavioralAnalysis::InferBehaviorForBlock(
    const BlockInstance& block,
    const CircuitGraph& graph
) {
    try {
        // Determine behavior kind from block kind
        BehaviorKind behavior_kind = InferBehaviorKindFromBlockKind(block.kind);

        // Determine port roles
        std::vector<BehaviorPortRole> port_roles = DeterminePortRoles(block.ports);

        // Infer bit width
        int bit_width = InferBitWidth(block.ports);

        // Generate description
        std::string description = GenerateDescription(behavior_kind, bit_width, port_roles);

        // Create the descriptor
        BehaviorDescriptor descriptor(
            block.id,
            "Block",
            behavior_kind,
            port_roles,
            bit_width,
            description
        );

        return MakeOk(descriptor);
    } catch (const std::exception& e) {
        return MakeError(ErrorCode::InternalError, std::string("Error inferring behavior for block: ") + e.what());
    }
}

Result<BehaviorDescriptor> BehavioralAnalysis::InferBehaviorForNode(
    const CircuitGraph& graph,
    const FunctionalAnalysis& func,
    const std::string& node_id,
    const std::string& node_kind_hint
) {
    try {
        // Check if the node belongs to a specific block
        // This would typically require more sophisticated logic depending on how blocks are mapped to the graph
        // For now, we'll implement a basic version

        // First, try to resolve the node in the circuit graph
        GraphNodeId graph_node_id(node_id, node_kind_hint);
        
        // Check if this node is part of a known block
        // This is a simplified approach - in a real implementation, you might need to check
        // which block this node belongs to by finding the containing block based on components
        // For now, let's assume it's not part of a specific block and return a generic descriptor

        // For now, return an unknown behavior descriptor
        BehaviorDescriptor descriptor = GetUnknownBehaviorDescriptor(node_id, node_kind_hint);
        return MakeOk(descriptor);
    } catch (const std::exception& e) {
        return MakeError(ErrorCode::InternalError, std::string("Error inferring behavior for node: ") + e.what());
    }
}

BehaviorKind BehavioralAnalysis::InferBehaviorKindFromBlockKind(BlockKind block_kind) {
    switch (block_kind) {
        case BlockKind::Adder:
            return BehaviorKind::Adder;
        case BlockKind::Comparator:
            return BehaviorKind::Comparator;
        case BlockKind::Mux:
            return BehaviorKind::Mux;
        case BlockKind::Decoder:
            return BehaviorKind::Decoder;
        case BlockKind::Encoder:
            return BehaviorKind::Encoder;
        case BlockKind::Register:
            return BehaviorKind::Register;
        case BlockKind::Counter:
            return BehaviorKind::Counter;
        case BlockKind::Latch:
            return BehaviorKind::Register;  // Treating latches as registers for now
        case BlockKind::GenericComb:
        default:
            return BehaviorKind::CombinationalLogic;
    }
}

std::vector<BehaviorPortRole> BehavioralAnalysis::DeterminePortRoles(const std::vector<BlockPort>& block_ports) {
    std::vector<BehaviorPortRole> roles;
    
    for (const auto& port : block_ports) {
        std::string role = "unknown";
        
        // Determine role based on port name patterns
        std::string lower_name = port.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        
        if (lower_name == "clk" || lower_name == "clock") {
            role = "clock";
        } else if (lower_name == "rst" || lower_name == "reset" || lower_name == "clr") {
            role = "reset";
        } else if (lower_name == "sel" || lower_name == "sel0" || lower_name == "sel1" || lower_name == "sel2" || lower_name == "sel3") {
            role = "select";
        } else if (lower_name == "en" || lower_name == "enable" || lower_name == "oe") {
            role = "enable";
        } else if (lower_name == "cin" || lower_name == "carryin" || lower_name == "carry_in") {
            role = "carry_in";
        } else if (lower_name == "cout" || lower_name == "carryout" || lower_name == "carry_out") {
            role = "carry_out";
        } else if (lower_name == "sum" || lower_name == "out" || lower_name == "q" || lower_name == "y") {
            role = "data_out";
        } else if (lower_name == "a" || lower_name == "b" || lower_name == "in" || lower_name == "d") {
            role = "data_in";
        } else if (port.direction == "in") {
            role = "data_in";
        } else if (port.direction == "out") {
            role = "data_out";
        } else {
            role = "unknown";
        }
        
        roles.emplace_back(port.name, role);
    }
    
    return roles;
}

int BehavioralAnalysis::InferBitWidth(const std::vector<BlockPort>& block_ports) {
    int max_width = 0;
    
    for (const auto& port : block_ports) {
        // Count the number of pins in this port to estimate bit width
        int pin_count = static_cast<int>(port.pins.size());
        if (pin_count > max_width) {
            max_width = pin_count;
        }
    }
    
    return max_width > 0 ? max_width : -1;
}

std::string BehavioralAnalysis::GenerateDescription(BehaviorKind kind, int bit_width, const std::vector<BehaviorPortRole>& ports) {
    std::string description = "";
    
    switch (kind) {
        case BehaviorKind::Adder:
            description = "adder";
            if (bit_width > 0) {
                description = std::to_string(bit_width) + "-bit ripple-carry " + description;
            }
            description += " with";
            bool has_carry = false;
            for (const auto& port : ports) {
                if (port.role == "carry_in" || port.role == "carry_out") {
                    has_carry = true;
                    break;
                }
            }
            if (has_carry) {
                description += " carry in/out";
            } else {
                description += "out carry";
            }
            break;
        case BehaviorKind::Mux:
            description = "multiplexer";
            if (bit_width > 0) {
                description = std::to_string(bit_width) + "-bit " + description;
            }
            break;
        case BehaviorKind::Decoder:
            description = "decoder";
            if (bit_width > 0) {
                description = std::to_string(bit_width) + "-bit " + description;
            }
            break;
        case BehaviorKind::Register:
            description = "register";
            if (bit_width > 0) {
                description = std::to_string(bit_width) + "-bit " + description;
            }
            break;
        case BehaviorKind::Counter:
            description = "counter";
            if (bit_width > 0) {
                description = std::to_string(bit_width) + "-bit " + description;
            }
            break;
        case BehaviorKind::CombinationalLogic:
            description = "generic combinational logic";
            break;
        default:
            description = "unknown behavior";
            break;
    }
    
    return description;
}

BehaviorDescriptor BehavioralAnalysis::GetUnknownBehaviorDescriptor(const std::string& node_id, const std::string& node_kind) {
    return BehaviorDescriptor(
        node_id,
        node_kind,
        BehaviorKind::Unknown,
        std::vector<BehaviorPortRole>(),
        -1,
        "No specific high-level behavior recognized; generic combinational node"
    );
}

} // namespace ProtoVMCLI