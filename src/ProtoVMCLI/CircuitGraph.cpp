#include "CircuitGraph.h"
#include <unordered_map>
#include <unordered_set>

namespace ProtoVMCLI {

void CircuitGraphBuilder::AddNode(CircuitGraph& graph, const GraphNodeId& node_id) {
    // Check if the node already exists
    for (const auto& existing_node : graph.nodes) {
        if (existing_node == node_id) {
            return; // Node already exists
        }
    }
    
    // Add the new node and initialize adjacency lists
    size_t node_idx = graph.nodes.size();
    graph.nodes.push_back(node_id);
    graph.adjacency_list.push_back(std::vector<size_t>());
    graph.reverse_adjacency_list.push_back(std::vector<size_t>());
}

void CircuitGraphBuilder::AddEdge(CircuitGraph& graph, const GraphNodeId& from, const GraphNodeId& to, GraphEdgeKind kind) {
    // Find the indices of the from and to nodes
    size_t from_idx = static_cast<size_t>(-1);
    size_t to_idx = static_cast<size_t>(-1);
    
    for (size_t i = 0; i < graph.nodes.size(); ++i) {
        if (graph.nodes[i] == from) {
            from_idx = i;
        }
        if (graph.nodes[i] == to) {
            to_idx = i;
        }
    }
    
    // If either node doesn't exist, add it
    if (from_idx == static_cast<size_t>(-1)) {
        AddNode(graph, from);
        from_idx = graph.nodes.size() - 1;
    }
    
    if (to_idx == static_cast<size_t>(-1)) {
        AddNode(graph, to);
        to_idx = graph.nodes.size() - 1;
    }
    
    // Add the edge
    GraphEdge edge(from, to, kind);
    size_t edge_idx = graph.edges.size();
    graph.edges.push_back(edge);
    
    // Update adjacency lists
    graph.adjacency_list[from_idx].push_back(edge_idx);
    graph.reverse_adjacency_list[to_idx].push_back(edge_idx);
}

void CircuitGraphBuilder::BuildAdjacencyLists(CircuitGraph& graph) {
    // Resize adjacency lists to match node count
    graph.adjacency_list.resize(graph.nodes.size());
    graph.reverse_adjacency_list.resize(graph.nodes.size());
    
    // Populate adjacency lists with edge indices
    for (size_t edge_idx = 0; edge_idx < graph.edges.size(); ++edge_idx) {
        const GraphEdge& edge = graph.edges[edge_idx];
        
        // Find node indices
        size_t from_idx = static_cast<size_t>(-1);
        size_t to_idx = static_cast<size_t>(-1);
        
        for (size_t node_idx = 0; node_idx < graph.nodes.size(); ++node_idx) {
            if (graph.nodes[node_idx] == edge.from) {
                from_idx = node_idx;
            }
            if (graph.nodes[node_idx] == edge.to) {
                to_idx = node_idx;
            }
        }
        
        if (from_idx != static_cast<size_t>(-1) && to_idx != static_cast<size_t>(-1)) {
            graph.adjacency_list[from_idx].push_back(edge_idx);
            graph.reverse_adjacency_list[to_idx].push_back(edge_idx);
        }
    }
}

Result<CircuitGraph> CircuitGraphBuilder::BuildGraph(const CircuitData& circuit) {
    try {
        CircuitGraph graph;
        
        // First, create all component nodes
        for (const auto& component : circuit.components) {
            GraphNodeId comp_node(GraphNodeKind::Component, component.id.id);
            AddNode(graph, comp_node);
            
            // Create pin nodes for this component
            for (const auto& input_pin : component.inputs) {
                std::string pin_id = component.id.id + ":" + input_pin.name;
                GraphNodeId pin_node(GraphNodeKind::Pin, pin_id);
                AddNode(graph, pin_node);
                
                // Add connectivity from component to pin (or vice versa)
                AddEdge(graph, comp_node, pin_node, GraphEdgeKind::Connectivity);
                AddEdge(graph, pin_node, comp_node, GraphEdgeKind::Connectivity);
            }
            
            for (const auto& output_pin : component.outputs) {
                std::string pin_id = component.id.id + ":" + output_pin.name;
                GraphNodeId pin_node(GraphNodeKind::Pin, pin_id);
                AddNode(graph, pin_node);
                
                // Add connectivity from component to pin (or vice versa)
                AddEdge(graph, comp_node, pin_node, GraphEdgeKind::Connectivity);
                AddEdge(graph, pin_node, comp_node, GraphEdgeKind::Connectivity);
            }
        }
        
        // Next, create net nodes and connect pins to nets
        for (const auto& wire : circuit.wires) {
            std::string net_id = wire.id.id;
            GraphNodeId net_node(GraphNodeKind::Net, net_id);
            AddNode(graph, net_node);
            
            // Connect start pin to net
            std::string start_pin_id = wire.start_component_id.id + ":" + wire.start_pin_name;
            GraphNodeId start_pin_node(GraphNodeKind::Pin, start_pin_id);
            AddEdge(graph, start_pin_node, net_node, GraphEdgeKind::Connectivity);
            AddEdge(graph, net_node, start_pin_node, GraphEdgeKind::Connectivity);
            
            // Connect end pin to net
            std::string end_pin_id = wire.end_component_id.id + ":" + wire.end_pin_name;
            GraphNodeId end_pin_node(GraphNodeKind::Pin, end_pin_id);
            AddEdge(graph, end_pin_node, net_node, GraphEdgeKind::Connectivity);
            AddEdge(graph, net_node, end_pin_node, GraphEdgeKind::Connectivity);
        }
        
        // Finally, create signal flow edges (output pins -> input pins via nets)
        // This requires additional analysis of the circuit to determine direction
        for (const auto& wire : circuit.wires) {
            // Find the component for the start pin (should be an output) and end pin (should be an input)
            std::string start_comp_id = wire.start_component_id.id;
            std::string end_comp_id = wire.end_component_id.id;
            
            // Look up the actual component to determine pin direction
            ComponentData* start_component = nullptr;
            ComponentData* end_component = nullptr;

            for (auto& comp : const_cast<std::vector<ComponentData>&>(circuit.components)) {
                if (comp.id == wire.start_component_id) {
                    start_component = &comp;
                }
                if (comp.id == wire.end_component_id) {
                    end_component = &comp;
                }
            }

            if (start_component && end_component) {
                // Check if start pin is an output and end pin is an input
                bool start_is_output = false;
                bool end_is_input = false;
                bool end_is_output = false;
                bool start_is_input = false;

                for (const auto& output : start_component->outputs) {
                    if (output.name == wire.start_pin_name) {
                        start_is_output = true;
                        break;
                    }
                }

                for (const auto& input : start_component->inputs) {
                    if (input.name == wire.start_pin_name) {
                        start_is_input = true;
                        break;
                    }
                }

                for (const auto& input : end_component->inputs) {
                    if (input.name == wire.end_pin_name) {
                        end_is_input = true;
                        break;
                    }
                }

                for (const auto& output : end_component->outputs) {
                    if (output.name == wire.end_pin_name) {
                        end_is_output = true;
                        break;
                    }
                }

                if (start_is_output && end_is_input) {
                    // Create signal flow from output pin to input pin
                    std::string start_pin_id = start_comp_id + ":" + wire.start_pin_name;
                    std::string end_pin_id = end_comp_id + ":" + wire.end_pin_name;
                    GraphNodeId start_pin_node(GraphNodeKind::Pin, start_pin_id);
                    GraphNodeId end_pin_node(GraphNodeKind::Pin, end_pin_id);
                    AddEdge(graph, start_pin_node, end_pin_node, GraphEdgeKind::SignalFlow);
                } else if (end_is_output && start_is_input) {
                    // The other direction
                    std::string start_pin_id = start_comp_id + ":" + wire.start_pin_name;
                    std::string end_pin_id = end_comp_id + ":" + wire.end_pin_name;
                    GraphNodeId start_pin_node(GraphNodeKind::Pin, start_pin_id);
                    GraphNodeId end_pin_node(GraphNodeKind::Pin, end_pin_id);
                    AddEdge(graph, end_pin_node, start_pin_node, GraphEdgeKind::SignalFlow);
                }
            }
        }
        
        // Build adjacency lists for efficient traversal
        BuildAdjacencyLists(graph);
        
        return Result<CircuitGraph>::MakeOk(graph);
    }
    catch (const std::exception& e) {
        return Result<CircuitGraph>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in CircuitGraphBuilder::BuildGraph: ") + e.what()
        );
    }
}

} // namespace ProtoVMCLI