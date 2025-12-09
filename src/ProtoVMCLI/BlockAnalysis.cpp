#include "BlockAnalysis.h"
#include "CircuitGraphQueries.h"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <queue>

namespace ProtoVMCLI {

bool BlockAnalysis::IsCombinationalComponent(const ComponentData& component) const {
    // Check if component has no state storage (i.e., not a latch, register, or memory)
    // This is a simplified check - in a real implementation, you'd have more comprehensive type checking
    std::string type = component.type.ToStd();
    
    // Common combinational component types
    if (type == "AND" || type == "OR" || type == "NOT" || type == "NAND" || 
        type == "NOR" || type == "XOR" || type == "XNOR" || 
        type == "BUFFER" || type == "TRISTATE_BUFFER") {
        return true;
    }
    
    // Components that have state are not combinational
    if (type == "DFF" || type == "DFFR" || type == "DFFS" || type == "LATCH" || 
        type == "REGISTER" || type == "MEMORY" || type == "RAM" || type == "ROM") {
        return false;
    }
    
    // By default, assume it's combinational unless we know otherwise
    return true;
}

bool BlockAnalysis::IsComponentType(const ComponentData& component, const std::string& type) const {
    return component.type.ToStd() == type;
}

std::vector<std::vector<std::string>> BlockAnalysis::FindConnectedComponents(
    const CircuitGraph& graph,
    const std::vector<std::string>& component_ids
) {
    // Create a set of component IDs for quick lookup
    std::unordered_set<std::string> component_set(component_ids.begin(), component_ids.end());
    
    // Create a mapping from component ID to graph node index
    std::unordered_map<std::string, size_t> comp_to_idx;
    for (size_t i = 0; i < graph.nodes.size(); ++i) {
        if (graph.nodes[i].kind == GraphNodeKind::Component && 
            component_set.count(graph.nodes[i].id) > 0) {
            comp_to_idx[graph.nodes[i].id] = i;
        }
    }
    
    // Use BFS to find connected components
    std::vector<bool> visited(graph.nodes.size(), false);
    std::vector<std::vector<std::string>> result;
    
    for (const auto& comp_id : component_ids) {
        if (comp_set.count(comp_id) == 0) continue;  // Not in our set to check
        
        size_t comp_idx = comp_to_idx[comp_id];
        if (visited[comp_idx]) continue;  // Already processed
        
        // BFS to find all connected components
        std::queue<size_t> queue;
        std::vector<std::string> current_cluster;
        queue.push(comp_idx);
        visited[comp_idx] = true;
        
        while (!queue.empty()) {
            size_t current_idx = queue.front();
            queue.pop();
            
            const GraphNodeId& current_node = graph.nodes[current_idx];
            if (current_node.kind == GraphNodeKind::Component) {
                current_cluster.push_back(current_node.id);
            }
            
            // Check all adjacent edges
            for (size_t edge_idx : graph.adjacency_list[current_idx]) {
                const GraphEdge& edge = graph.edges[edge_idx];
                for (size_t i = 0; i < graph.nodes.size(); ++i) {
                    if (graph.nodes[i] == edge.to && !visited[i]) {
                        if (graph.nodes[i].kind == GraphNodeKind::Component && 
                            component_set.count(graph.nodes[i].id) > 0) {
                            visited[i] = true;
                            queue.push(i);
                        }
                    }
                }
            }
            
            // Also check reverse adjacency (for bidirectional connectivity through nets)
            for (size_t edge_idx : graph.reverse_adjacency_list[current_idx]) {
                const GraphEdge& edge = graph.edges[edge_idx];
                for (size_t i = 0; i < graph.nodes.size(); ++i) {
                    if (graph.nodes[i] == edge.from && !visited[i]) {
                        if (graph.nodes[i].kind == GraphNodeKind::Component && 
                            component_set.count(graph.nodes[i].id) > 0) {
                            visited[i] = true;
                            queue.push(i);
                        }
                    }
                }
            }
        }
        
        if (!current_cluster.empty()) {
            result.push_back(current_cluster);
        }
    }
    
    return result;
}

BlockKind BlockAnalysis::ClassifyBlock(const std::vector<std::string>& component_ids,
                                       const CircuitData& circuit) const {
    // Create a map of component ID to component for quick lookup
    std::unordered_map<std::string, const ComponentData*> comp_map;
    for (const auto& comp : circuit.components) {
        comp_map[comp.id.id] = &comp;
    }
    
    int and_count = 0, or_count = 0, xor_count = 0, not_count = 0;
    int mux_related = 0;  // Count of components that could be part of a mux (AND, OR, NOT)
    int adder_related = 0; // Count of components that could be part of an adder (XOR, AND)
    
    for (const auto& comp_id : component_ids) {
        auto it = comp_map.find(comp_id);
        if (it != comp_map.end()) {
            std::string type = it->second->type.ToStd();
            if (type == "AND") { and_count++; mux_related++; adder_related++; }
            else if (type == "OR") { or_count++; mux_related++; }
            else if (type == "XOR") { xor_count++; adder_related++; }
            else if (type == "NOT") { not_count++; mux_related++; }
        }
    }
    
    // Classify based on component composition
    if (xor_count > 0 && and_count > 0) {
        // Likely an adder (XOR + AND for sum and carry logic)
        return BlockKind::Adder;
    }
    
    if (and_count > 1 && or_count > 0 && not_count > 0) {
        // Likely a multiplexer (AND gates for selection, OR gate to combine, NOT for inversions)
        return BlockKind::Mux;
    }
    
    if (xor_count >= 2 && and_count == 0 && or_count == 0) {
        // Likely a comparator (series of XORs for equality checking)
        return BlockKind::Comparator;
    }
    
    // If we have multiple AND gates with a common pattern, could be decoder
    if (and_count >= 2 && or_count == 0) {
        return BlockKind::Decoder;
    }
    
    // Default to generic combinational if no specific pattern is found
    return BlockKind::GenericComb;
}

std::vector<BlockPort> BlockAnalysis::DetermineBlockPorts(
    const std::vector<std::string>& component_ids,
    const CircuitGraph& graph
) const {
    std::vector<BlockPort> ports;
    
    // Create a set of component IDs for quick lookup
    std::unordered_set<std::string> component_set(component_ids.begin(), component_ids.end());
    
    // Find all pins that belong to the components in this block
    std::vector<GraphNodeId> internal_pins;
    for (const auto& node : graph.nodes) {
        if (node.kind == GraphNodeKind::Pin) {
            // Extract component ID from pin ID (format: "COMP_ID:PIN_NAME")
            size_t colon_pos = node.id.find(':');
            if (colon_pos != std::string::npos) {
                std::string comp_id = node.id.substr(0, colon_pos);
                if (component_set.count(comp_id) > 0) {
                    internal_pins.push_back(node);
                }
            }
        }
    }
    
    // Determine which pins are connected to nets outside this block
    // These are the external interfaces of the block
    
    std::unordered_set<std::string> external_nets;
    std::unordered_map<std::string, std::vector<std::string>> net_to_pins;
    
    for (const auto& pin : internal_pins) {
        // Look for connectivity edges between this pin and nets
        for (size_t edge_idx = 0; edge_idx < graph.edges.size(); ++edge_idx) {
            const GraphEdge& edge = graph.edges[edge_idx];
            if (edge.kind == GraphEdgeKind::Connectivity) {
                if (edge.from == pin && edge.to.kind == GraphNodeKind::Net) {
                    // pin -> net
                    net_to_pins[edge.to.id].push_back(pin.id);
                    
                    // Check if this net connects to any components outside our block
                    bool connected_outside = false;
                    for (size_t check_edge_idx = 0; check_edge_idx < graph.edges.size(); ++check_edge_idx) {
                        const GraphEdge& check_edge = graph.edges[check_edge_idx];
                        if (check_edge.kind == GraphEdgeKind::Connectivity) {
                            if (check_edge.to.id == edge.to.id && check_edge.from.kind == GraphNodeKind::Pin) {
                                // Found another pin connected to the same net
                                size_t colon_pos = check_edge.from.id.find(':');
                                if (colon_pos != std::string::npos) {
                                    std::string other_comp_id = check_edge.from.id.substr(0, colon_pos);
                                    if (component_set.count(other_comp_id) == 0) {
                                        // This net connects to a component outside our block
                                        connected_outside = true;
                                        external_nets.insert(edge.to.id);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (connected_outside) {
                        external_nets.insert(edge.to.id);
                    }
                }
                else if (edge.to == pin && edge.from.kind == GraphNodeKind::Net) {
                    // net -> pin
                    net_to_pins[edge.from.id].push_back(pin.id);
                    
                    // Check if this net connects to any components outside our block
                    bool connected_outside = false;
                    for (size_t check_edge_idx = 0; check_edge_idx < graph.edges.size(); ++check_edge_idx) {
                        const GraphEdge& check_edge = graph.edges[check_edge_idx];
                        if (check_edge.kind == GraphEdgeKind::Connectivity) {
                            if (check_edge.from.id == edge.from.id && check_edge.to.kind == GraphNodeKind::Pin) {
                                // Found another pin connected to the same net
                                size_t colon_pos = check_edge.to.id.find(':');
                                if (colon_pos != std::string::npos) {
                                    std::string other_comp_id = check_edge.to.id.substr(0, colon_pos);
                                    if (component_set.count(other_comp_id) == 0) {
                                        // This net connects to a component outside our block
                                        connected_outside = true;
                                        external_nets.insert(edge.from.id);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    if (connected_outside) {
                        external_nets.insert(edge.from.id);
                    }
                }
            }
        }
    }
    
    // For each external net, determine if it's an input or output based on signal flow
    for (const auto& net_id : external_nets) {
        auto net_pins_it = net_to_pins.find(net_id);
        if (net_pins_it != net_to_pins.end()) {
            // Determine direction based on signal flow edges
            std::vector<std::string> input_pins, output_pins;
            
            for (const auto& pin_id : net_pins_it->second) {
                bool is_input_pin = false;
                bool is_output_pin = false;
                
                // Check signal flow edges
                for (const auto& edge : graph.edges) {
                    if (edge.kind == GraphEdgeKind::SignalFlow) {
                        // If this pin is the source of a signal flow, it's an output
                        if (edge.from.id == pin_id) {
                            is_output_pin = true;
                        }
                        // If this pin is the target of a signal flow, it's an input
                        if (edge.to.id == pin_id) {
                            is_input_pin = true;
                        }
                    }
                }
                
                if (is_output_pin) {
                    output_pins.push_back(pin_id);
                }
                if (is_input_pin) {
                    input_pins.push_back(pin_id);
                }
            }
            
            // Create port definitions
            if (!input_pins.empty()) {
                ports.push_back(BlockPort("IN", "in", input_pins));
            }
            if (!output_pins.empty()) {
                ports.push_back(BlockPort("OUT", "out", output_pins));
            }
        }
    }
    
    return ports;
}

Result<std::vector<BlockInstance>> BlockAnalysis::DetectGenericCombinationalBlocks(
    const CircuitGraph& graph,
    const CircuitData& circuit
) {
    std::vector<BlockInstance> blocks;
    
    // Collect all combinational components
    std::vector<std::string> combinational_components;
    for (const auto& component : circuit.components) {
        if (IsCombinationalComponent(component)) {
            combinational_components.push_back(component.id.id);
        }
    }
    
    // Group connected combinational components into blocks
    auto connected_clusters = FindConnectedComponents(graph, combinational_components);
    
    // Process each cluster to create blocks
    int block_id_counter = 1;
    for (const auto& cluster : connected_clusters) {
        if (cluster.size() >= 2) {  // Only create blocks with 2 or more components
            // Classify the block based on its composition
            BlockKind kind = ClassifyBlock(cluster, circuit);
            
            // Determine block ports
            auto ports = DetermineBlockPorts(cluster, graph);
            
            std::string block_id = "B" + std::to_string(block_id_counter++);
            
            blocks.push_back(BlockInstance(block_id, kind, cluster, std::vector<std::string>(), ports));
        }
    }
    
    return Result<std::vector<BlockInstance>>::MakeOk(blocks);
}

Result<std::vector<BlockInstance>> BlockAnalysis::DetectAdders(
    const CircuitGraph& graph,
    const CircuitData& circuit
) {
    std::vector<BlockInstance> blocks;
    
    // Find chains of XOR and AND gates that match adder patterns
    std::unordered_map<std::string, const ComponentData*> comp_map;
    for (const auto& comp : circuit.components) {
        comp_map[comp.id.id] = &comp;
    }
    
    // Look for XOR-AND patterns that might indicate adder logic
    std::unordered_set<std::string> processed;
    for (const auto& comp : circuit.components) {
        if (processed.count(comp.id.id) > 0) continue;
        
        std::string type = comp.type.ToStd();
        if (type == "XOR") {
            // Look for potential carry-lookahead logic or simple ripple carry
            std::vector<std::string> candidate_components = {comp.id.id};
            
            // Look for connected AND gates (potential carry logic)
            CircuitGraphQueries queries;
            auto fanout_result = queries.FindFanOut(graph, GraphNodeId(GraphNodeKind::Pin, comp.id.id + ":" + comp.outputs[0].name), 3);
            
            if (fanout_result.ok) {
                for (const auto& path : fanout_result.data.paths) {
                    for (const auto& node : path.nodes) {
                        if (node.kind == GraphNodeKind::Component) {
                            size_t colon_pos = node.id.find(':');
                            if (colon_pos != std::string::npos) {
                                std::string comp_id = node.id.substr(0, colon_pos);
                                auto it = comp_map.find(comp_id);
                                if (it != comp_map.end() && 
                                    (it->second->type.ToStd() == "AND" || it->second->type.ToStd() == "OR")) {
                                    candidate_components.push_back(comp_id);
                                }
                            }
                        }
                    }
                }
            }
            
            // If we found a meaningful combination, create an adder block
            if (candidate_components.size() >= 2) {
                auto ports = DetermineBlockPorts(candidate_components, graph);
                
                std::string block_id = "B" + std::to_string(blocks.size() + 1);
                blocks.push_back(BlockInstance(
                    block_id, 
                    BlockKind::Adder, 
                    candidate_components, 
                    std::vector<std::string>(), 
                    ports
                ));
                
                // Mark all components in this block as processed
                for (const auto& comp_id : candidate_components) {
                    processed.insert(comp_id);
                }
            }
        }
    }
    
    return Result<std::vector<BlockInstance>>::MakeOk(blocks);
}

Result<std::vector<BlockInstance>> BlockAnalysis::DetectMuxes(
    const CircuitGraph& graph,
    const CircuitData& circuit
) {
    std::vector<BlockInstance> blocks;
    
    // Find patterns typical of multiplexers: AND gates feeding into an OR gate with selection logic
    std::unordered_map<std::string, const ComponentData*> comp_map;
    for (const auto& comp : circuit.components) {
        comp_map[comp.id.id] = &comp;
    }
    
    std::unordered_set<std::string> processed;
    for (const auto& comp : circuit.components) {
        if (processed.count(comp.id.id) > 0) continue;
        
        std::string type = comp.type.ToStd();
        if (type == "OR") {
            // Check if this OR gate has AND gates as inputs (potential mux output)
            std::vector<std::string> candidate_components = {comp.id.id};
            
            // Look for AND gates that feed into this OR gate
            CircuitGraphQueries queries;
            auto fanin_result = queries.FindFanIn(graph, GraphNodeId(GraphNodeKind::Pin, comp.id.id + ":" + comp.inputs[0].name), 3);
            
            if (fanin_result.ok) {
                for (const auto& path : fanin_result.data.paths) {
                    for (const auto& node : path.nodes) {
                        if (node.kind == GraphNodeKind::Component) {
                            size_t colon_pos = node.id.find(':');
                            if (colon_pos != std::string::npos) {
                                std::string comp_id = node.id.substr(0, colon_pos);
                                auto it = comp_map.find(comp_id);
                                if (it != comp_map.end() && it->second->type.ToStd() == "AND") {
                                    if (std::find(candidate_components.begin(), 
                                                 candidate_components.end(), 
                                                 comp_id) == candidate_components.end()) {
                                        candidate_components.push_back(comp_id);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Check for selection logic (NOT gates for inverted selection)
            auto full_fanin = queries.FindFanIn(graph, GraphNodeId(GraphNodeKind::Pin, comp.id.id + ":" + comp.inputs[0].name), 5);
            if (full_fanin.ok) {
                for (const auto& path : full_fanin.data.paths) {
                    for (const auto& node : path.nodes) {
                        if (node.kind == GraphNodeKind::Component) {
                            size_t colon_pos = node.id.find(':');
                            if (colon_pos != std::string::npos) {
                                std::string comp_id = node.id.substr(0, colon_pos);
                                auto it = comp_map.find(comp_id);
                                if (it != comp_map.end() && it->second->type.ToStd() == "NOT") {
                                    if (std::find(candidate_components.begin(), 
                                                 candidate_components.end(), 
                                                 comp_id) == candidate_components.end()) {
                                        candidate_components.push_back(comp_id);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // If we found a meaningful combination, create a mux block
            if (candidate_components.size() >= 3) {  // Need at least AND gates + OR gate
                auto ports = DetermineBlockPorts(candidate_components, graph);
                
                std::string block_id = "M" + std::to_string(blocks.size() + 1);
                blocks.push_back(BlockInstance(
                    block_id, 
                    BlockKind::Mux, 
                    candidate_components, 
                    std::vector<std::string>(), 
                    ports
                ));
                
                // Mark all components in this block as processed
                for (const auto& comp_id : candidate_components) {
                    processed.insert(comp_id);
                }
            }
        }
    }
    
    return Result<std::vector<BlockInstance>>::MakeOk(blocks);
}

Result<std::vector<BlockInstance>> BlockAnalysis::DetectComparators(
    const CircuitGraph& graph,
    const CircuitData& circuit
) {
    std::vector<BlockInstance> blocks;
    
    // Find chains of XOR gates that might indicate equality comparison logic
    std::unordered_map<std::string, const ComponentData*> comp_map;
    for (const auto& comp : circuit.components) {
        comp_map[comp.id.id] = &comp;
    }
    
    std::unordered_set<std::string> processed;
    for (const auto& comp : circuit.components) {
        if (processed.count(comp.id.id) > 0) continue;
        
        std::string type = comp.type.ToStd();
        if (type == "XOR") {
            // Look for XOR gates that feed into AND gates (equality check)
            std::vector<std::string> candidate_components = {comp.id.id};
            
            CircuitGraphQueries queries;
            auto fanout_result = queries.FindFanOut(graph, GraphNodeId(GraphNodeKind::Pin, comp.id.id + ":" + comp.outputs[0].name), 2);
            
            if (fanout_result.ok) {
                for (const auto& path : fanout_result.data.paths) {
                    for (const auto& node : path.nodes) {
                        if (node.kind == GraphNodeKind::Component) {
                            size_t colon_pos = node.id.find(':');
                            if (colon_pos != std::string::npos) {
                                std::string comp_id = node.id.substr(0, colon_pos);
                                auto it = comp_map.find(comp_id);
                                if (it != comp_map.end() && 
                                    (it->second->type.ToStd() == "AND" || it->second->type.ToStd() == "NOR")) {
                                    if (std::find(candidate_components.begin(), 
                                                 candidate_components.end(), 
                                                 comp_id) == candidate_components.end()) {
                                        candidate_components.push_back(comp_id);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Look for additional XOR gates that might be part of the same comparison
            auto fanin_result = queries.FindFanIn(graph, GraphNodeId(GraphNodeKind::Pin, comp.id.id + ":" + comp.inputs[0].name), 2);
            if (fanin_result.ok) {
                for (const auto& path : fanin_result.data.paths) {
                    for (const auto& node : path.nodes) {
                        if (node.kind == GraphNodeKind::Component) {
                            size_t colon_pos = node.id.find(':');
                            if (colon_pos != std::string::npos) {
                                std::string comp_id = node.id.substr(0, colon_pos);
                                auto it = comp_map.find(comp_id);
                                if (it != comp_map.end() && it->second->type.ToStd() == "XOR") {
                                    if (std::find(candidate_components.begin(), 
                                                 candidate_components.end(), 
                                                 comp_id) == candidate_components.end()) {
                                        candidate_components.push_back(comp_id);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // If we found a meaningful combination, create a comparator block
            if (candidate_components.size() >= 2) {
                auto ports = DetermineBlockPorts(candidate_components, graph);
                
                std::string block_id = "CMP" + std::to_string(blocks.size() + 1);
                blocks.push_back(BlockInstance(
                    block_id, 
                    BlockKind::Comparator, 
                    candidate_components, 
                    std::vector<std::string>(), 
                    ports
                ));
                
                // Mark all components in this block as processed
                for (const auto& comp_id : candidate_components) {
                    processed.insert(comp_id);
                }
            }
        }
    }
    
    return Result<std::vector<BlockInstance>>::MakeOk(blocks);
}

Result<std::vector<BlockInstance>> BlockAnalysis::DetectDecoders(
    const CircuitGraph& graph,
    const CircuitData& circuit
) {
    std::vector<BlockInstance> blocks;
    
    // Find patterns typical of decoders: AND gates with specific input combinations
    std::unordered_map<std::string, const ComponentData*> comp_map;
    for (const auto& comp : circuit.components) {
        comp_map[comp.id.id] = &comp;
    }
    
    std::unordered_set<std::string> processed;
    for (const auto& comp : circuit.components) {
        if (processed.count(comp.id.id) > 0) continue;
        
        std::string type = comp.type.ToStd();
        if (type == "AND") {
            // Check if this AND gate has NOT gates as inputs (typical decoder pattern)
            std::vector<std::string> candidate_components = {comp.id.id};
            
            CircuitGraphQueries queries;
            auto fanin_result = queries.FindFanIn(graph, GraphNodeId(GraphNodeKind::Pin, comp.id.id + ":" + comp.inputs[0].name), 2);
            
            if (fanin_result.ok) {
                for (const auto& path : fanin_result.data.paths) {
                    for (const auto& node : path.nodes) {
                        if (node.kind == GraphNodeKind::Component) {
                            size_t colon_pos = node.id.find(':');
                            if (colon_pos != std::string::npos) {
                                std::string comp_id = node.id.substr(0, colon_pos);
                                auto it = comp_map.find(comp_id);
                                if (it != comp_map.end() && 
                                    (it->second->type.ToStd() == "NOT" || it->second->type.ToStd() == "AND")) {
                                    if (std::find(candidate_components.begin(), 
                                                 candidate_components.end(), 
                                                 comp_id) == candidate_components.end()) {
                                        candidate_components.push_back(comp_id);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // If we have multiple AND gates with similar fan-in patterns, they might be part of the same decoder
            if (candidate_components.size() >= 2) {
                auto ports = DetermineBlockPorts(candidate_components, graph);
                
                std::string block_id = "DEC" + std::to_string(blocks.size() + 1);
                blocks.push_back(BlockInstance(
                    block_id, 
                    BlockKind::Decoder, 
                    candidate_components, 
                    std::vector<std::string>(), 
                    ports
                ));
                
                // Mark all components in this block as processed
                for (const auto& comp_id : candidate_components) {
                    processed.insert(comp_id);
                }
            }
        }
    }
    
    return Result<std::vector<BlockInstance>>::MakeOk(blocks);
}

Result<BlockGraph> BlockAnalysis::DetectBlocks(
    const CircuitGraph& graph,
    const CircuitData& circuit
) {
    try {
        BlockGraph result;
        
        // Detect different types of blocks
        auto generic_blocks_result = DetectGenericCombinationalBlocks(graph, circuit);
        if (!generic_blocks_result.ok) {
            return Result<BlockGraph>::MakeError(generic_blocks_result.error_code, generic_blocks_result.error_message);
        }
        
        auto adder_blocks_result = DetectAdders(graph, circuit);
        if (!adder_blocks_result.ok) {
            return Result<BlockGraph>::MakeError(adder_blocks_result.error_code, adder_blocks_result.error_message);
        }
        
        auto mux_blocks_result = DetectMuxes(graph, circuit);
        if (!mux_blocks_result.ok) {
            return Result<BlockGraph>::MakeError(mux_blocks_result.error_code, mux_blocks_result.error_message);
        }
        
        auto comparator_blocks_result = DetectComparators(graph, circuit);
        if (!comparator_blocks_result.ok) {
            return Result<BlockGraph>::MakeError(comparator_blocks_result.error_code, comparator_blocks_result.error_message);
        }
        
        auto decoder_blocks_result = DetectDecoders(graph, circuit);
        if (!decoder_blocks_result.ok) {
            return Result<BlockGraph>::MakeError(decoder_blocks_result.error_code, decoder_blocks_result.error_message);
        }
        
        // Combine all detected blocks
        result.blocks.insert(result.blocks.end(), 
                            generic_blocks_result.data.begin(), 
                            generic_blocks_result.data.end());
        result.blocks.insert(result.blocks.end(), 
                            adder_blocks_result.data.begin(), 
                            adder_blocks_result.data.end());
        result.blocks.insert(result.blocks.end(), 
                            mux_blocks_result.data.begin(), 
                            mux_blocks_result.data.end());
        result.blocks.insert(result.blocks.end(), 
                            comparator_blocks_result.data.begin(), 
                            comparator_blocks_result.data.end());
        result.blocks.insert(result.blocks.end(), 
                            decoder_blocks_result.data.begin(), 
                            decoder_blocks_result.data.end());
        
        return Result<BlockGraph>::MakeOk(result);
    }
    catch (const std::exception& e) {
        return Result<BlockGraph>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in BlockAnalysis::DetectBlocks: ") + e.what()
        );
    }
}

} // namespace ProtoVMCLI