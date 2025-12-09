#include "HlsIrInference.h"
#include "CircuitGraphQueries.h"
#include <unordered_set>
#include <algorithm>

namespace ProtoVMCLI {

IrExpr HlsIrInference::CreateBinaryOp(IrExprKind kind, const IrValue& target, const IrValue& a, const IrValue& b) {
    std::vector<IrValue> args = {a, b};
    return IrExpr(kind, target, args);
}

IrExpr HlsIrInference::CreateUnaryOp(IrExprKind kind, const IrValue& target, const IrValue& a) {
    std::vector<IrValue> args = {a};
    return IrExpr(kind, target, args);
}

IrExpr HlsIrInference::CreateTernaryOp(IrExprKind kind, const IrValue& target, const IrValue& sel, const IrValue& a, const IrValue& b) {
    std::vector<IrValue> args = {sel, a, b};
    return IrExpr(kind, target, args);
}

IrValue HlsIrInference::FindValueByName(const std::vector<IrValue>& values, const std::string& name) {
    for (const auto& val : values) {
        if (val.name == name) {
            return val;
        }
    }
    // Return a default value if not found
    return IrValue(name, -1);
}

std::vector<IrValue> HlsIrInference::MapBlockPortsToIrValues(const std::vector<BlockPort>& block_ports) {
    std::vector<IrValue> ir_values;
    
    for (const auto& port : block_ports) {
        // For multi-pin ports, create a value for each pin
        if (!port.pins.empty()) {
            // TODO: Handle multi-bit values properly. For now, we'll use the first pin name as the value name
            ir_values.push_back(IrValue(port.name, static_cast<int>(port.pins.size())));
        } else {
            ir_values.push_back(IrValue(port.name, 1));
        }
    }
    
    return ir_values;
}

std::vector<IrExpr> HlsIrInference::InferExpressionsFromBlockBehavior(const BlockInstance& block, const BehaviorDescriptor& behavior) {
    std::vector<IrExpr> exprs;
    
    switch (behavior.behavior_kind) {
        case BehaviorKind::Adder: {
            // Find the data input ports and output port
            IrValue a_val, b_val, cin_val, sum_val, cout_val;
            bool has_cin = false, has_cout = false;
            
            for (const auto& port : behavior.ports) {
                if (port.role == "data_in") {
                    if (a_val.name.empty()) {
                        a_val = IrValue(port.port_name, behavior.bit_width);
                    } else if (b_val.name.empty()) {
                        b_val = IrValue(port.port_name, behavior.bit_width);
                    }
                } else if (port.role == "data_out") {
                    sum_val = IrValue(port.port_name, behavior.bit_width);
                } else if (port.role == "carry_in") {
                    cin_val = IrValue(port.port_name, 1);
                    has_cin = true;
                } else if (port.role == "carry_out") {
                    cout_val = IrValue(port.port_name, 1);
                    has_cout = true;
                }
            }
            
            // Create expression for sum
            if (!a_val.name.empty() && !b_val.name.empty() && !sum_val.name.empty()) {
                if (has_cin) {
                    // Full adder: SUM = A + B + CIN
                    std::vector<IrValue> args = {a_val, b_val, cin_val};
                    exprs.push_back(IrExpr(IrExprKind::Add, sum_val, args));
                } else {
                    // Half adder: SUM = A + B
                    std::vector<IrValue> args = {a_val, b_val};
                    exprs.push_back(IrExpr(IrExprKind::Add, sum_val, args));
                }
            }
            
            // Create expression for carry out if applicable
            if (has_cout && !cout_val.name.empty()) {
                // Carry out expression would be based on a, b, and cin
                std::vector<IrValue> args;
                if (has_cin) {
                    args = {a_val, b_val, cin_val};
                } else {
                    args = {a_val, b_val};
                }
                exprs.push_back(IrExpr(IrExprKind::Add, cout_val, args)); // Simplified - in reality would need proper carry logic
            }
            break;
        }
        
        case BehaviorKind::Mux: {
            IrValue sel_val, in0_val, in1_val, out_val;
            
            for (const auto& port : behavior.ports) {
                if (port.role == "select") {
                    sel_val = IrValue(port.port_name, 1); // Assuming 2:1 mux
                } else if (port.role == "data_in") {
                    if (in0_val.name.empty()) {
                        in0_val = IrValue(port.port_name, behavior.bit_width);
                    } else if (in1_val.name.empty()) {
                        in1_val = IrValue(port.port_name, behavior.bit_width);
                    }
                } else if (port.role == "data_out") {
                    out_val = IrValue(port.port_name, behavior.bit_width);
                }
            }
            
            // Create mux expression: OUT = SEL ? IN1 : IN0
            if (!sel_val.name.empty() && !in0_val.name.empty() && 
                !in1_val.name.empty() && !out_val.name.empty()) {
                std::vector<IrValue> args = {sel_val, in1_val, in0_val};
                exprs.push_back(IrExpr(IrExprKind::Mux, out_val, args));
            }
            break;
        }
        
        case BehaviorKind::Comparator:
        case BehaviorKind::EqualityComparator: {
            IrValue a_val, b_val, eq_val;
            
            for (const auto& port : behavior.ports) {
                if (port.role == "data_in") {
                    if (a_val.name.empty()) {
                        a_val = IrValue(port.port_name, behavior.bit_width);
                    } else if (b_val.name.empty()) {
                        b_val = IrValue(port.port_name, behavior.bit_width);
                    }
                } else if (port.role == "data_out") {
                    eq_val = IrValue(port.port_name, 1);
                }
            }
            
            // Create equality comparison expression: EQ = (A == B)
            if (!a_val.name.empty() && !b_val.name.empty() && !eq_val.name.empty()) {
                std::vector<IrValue> args = {a_val, b_val};
                exprs.push_back(IrExpr(IrExprKind::Eq, eq_val, args));
            }
            break;
        }
        
        case BehaviorKind::Register: {
            // For registers, we don't create combinational expressions here
            // The sequential assignments are handled separately
            break;
        }
        
        case BehaviorKind::GenericComb: {
            // For generic combinational blocks, we'll try to identify simple connections
            // This is a simplified approach - in a real implementation, you'd need to
            // analyze the internal structure more deeply
            break;
        }
        
        default:
            // For unknown behaviors, we don't generate expressions
            break;
    }
    
    return exprs;
}

Result<IrModule> HlsIrInference::InferIrForBlock(
    const BlockInstance& block,
    const CircuitGraph& graph,
    const BehaviorDescriptor& behavior
) {
    try {
        // Map block ports to IR values
        std::vector<IrValue> ir_inputs, ir_outputs;
        
        for (const auto& port : block.ports) {
            if (port.direction == "in") {
                ir_inputs.push_back(IrValue(port.name, static_cast<int>(port.pins.size())));
            } else if (port.direction == "out") {
                ir_outputs.push_back(IrValue(port.name, static_cast<int>(port.pins.size())));
            } else {
                // If direction is inout or not specified, add to both
                ir_inputs.push_back(IrValue(port.name, static_cast<int>(port.pins.size())));
                ir_outputs.push_back(IrValue(port.name, static_cast<int>(port.pins.size())));
            }
        }
        
        // Generate expressions based on behavior
        std::vector<IrExpr> comb_exprs = InferExpressionsFromBlockBehavior(block, behavior);
        
        // For registers and counters, create sequential assignments
        std::vector<IrRegAssign> reg_assigns;
        if (behavior.behavior_kind == BehaviorKind::Register) {
            // Find clock and reset signals
            std::string clock_name = "";
            std::string reset_name = "";
            
            for (const auto& port : behavior.ports) {
                if (port.role == "clock") {
                    clock_name = port.port_name;
                } else if (port.role == "reset") {
                    reset_name = port.port_name;
                }
            }
            
            // Create register assignments
            // This is a simplified approach - in reality, you'd need to identify the D input and Q output
            for (const auto& port : block.ports) {
                if (port.direction == "out") {
                    // Look for corresponding input port with same bit structure
                    std::string input_port = port.name;  // Simplified mapping
                    IrValue target_val(port.name, static_cast<int>(port.pins.size()));
                    IrValue source_val(input_port, static_cast<int>(port.pins.size()));
                    
                    IrExpr expr(IrExprKind::Value, source_val, {source_val});
                    reg_assigns.push_back(IrRegAssign(target_val, expr, clock_name, reset_name));
                }
            }
        }
        
        IrModule ir_module(block.id, ir_inputs, ir_outputs, comb_exprs, reg_assigns);
        
        return Result<IrModule>::MakeOk(ir_module);
    }
    catch (const std::exception& e) {
        return Result<IrModule>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in InferIrForBlock: ") + e.what()
        );
    }
}

Result<IrModule> HlsIrInference::InferIrForNodeRegion(
    const CircuitGraph& graph,
    const std::string& node_id,
    const std::string& node_kind_hint,
    const FunctionalAnalysis& func,
    const BehavioralAnalysis& beh,
    int max_depth
) {
    try {
        // Create FunctionalNodeId from the provided id and kind hint
        FunctionalNodeId func_node_id(node_id, node_kind_hint);
        
        // Get forward and backward cones to determine the local region
        auto backward_cone_result = func.ComputeBackwardCone(graph, func_node_id, max_depth);
        if (!backward_cone_result.ok) {
            return Result<IrModule>::MakeError(
                backward_cone_result.error_code,
                backward_cone_result.error_message
            );
        }
        
        auto forward_cone_result = func.ComputeForwardCone(graph, func_node_id, max_depth);
        if (!forward_cone_result.ok) {
            return Result<IrModule>::MakeError(
                forward_cone_result.error_code,
                forward_cone_result.error_message
            );
        }
        
        // Combine nodes from both cones to define the region
        std::unordered_set<std::string> region_nodes;
        region_nodes.insert(node_id);
        
        for (const auto& cone_node : backward_cone_result.data.nodes) {
            region_nodes.insert(cone_node.node.id);
        }
        
        for (const auto& cone_node : forward_cone_result.data.nodes) {
            region_nodes.insert(cone_node.node.id);
        }
        
        // Create a minimal IR module for this region
        std::vector<IrValue> inputs, outputs;
        std::vector<IrExpr> comb_assigns;
        std::vector<IrRegAssign> reg_assigns;
        
        // In a real implementation, we'd need to analyze the connections within this region
        // For now, we'll just return a basic module with the central node
        IrModule ir_module(node_id + "_region", inputs, outputs, comb_assigns, reg_assigns);
        
        return Result<IrModule>::MakeOk(ir_module);
    }
    catch (const std::exception& e) {
        return Result<IrModule>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in InferIrForNodeRegion: ") + e.what()
        );
    }
}

} // namespace ProtoVMCLI