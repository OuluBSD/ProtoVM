#include "CodegenIrInference.h"
#include "CircuitFacade.h"
#include "HlsIrInference.h"
#include "ScheduledIr.h"
#include "BehavioralAnalysis.h"
#include "BlockAnalysis.h"
#include <algorithm>
#include <cctype>

namespace ProtoVMCLI {

// Helper function to convert HLS IR expression kind to Codegen expression kind
static CodegenExprKind ConvertIrExprKindToCodegenKind(IrExprKind ir_kind) {
    switch (ir_kind) {
        case IrExprKind::Value:
            return CodegenExprKind::Value;
        case IrExprKind::Not:
            return CodegenExprKind::UnaryOp;
        case IrExprKind::And:
        case IrExprKind::Or:
        case IrExprKind::Xor:
        case IrExprKind::Add:
        case IrExprKind::Sub:
        case IrExprKind::Eq:
        case IrExprKind::Neq:
            return CodegenExprKind::BinaryOp;
        case IrExprKind::Mux:
            return CodegenExprKind::TernaryOp;
        default:
            return CodegenExprKind::Value;  // default fallback
    }
}

// Helper function to map HLS IR operator to C/C++ operator
static std::string ConvertIrOpToCOp(IrExprKind ir_kind) {
    switch (ir_kind) {
        case IrExprKind::Not: return "!";
        case IrExprKind::And: return "&";
        case IrExprKind::Or:  return "|";
        case IrExprKind::Xor: return "^";
        case IrExprKind::Add: return "+";
        case IrExprKind::Sub: return "-";
        case IrExprKind::Eq:  return "==";
        case IrExprKind::Neq: return "!=";
        case IrExprKind::Mux: return "?:";
        default: return "";  // fallback
    }
}

// Helper function to infer C type from bit width
static std::string InferCType(int bit_width) {
    if (bit_width <= 0) {
        return "int";  // default type
    } else if (bit_width <= 8) {
        return "int8_t";
    } else if (bit_width <= 16) {
        return "int16_t";
    } else if (bit_width <= 32) {
        return "int32_t";
    } else if (bit_width <= 64) {
        return "int64_t";
    } else {
        return "int64_t";  // for very wide values
    }
}

CodegenValue CodegenIrInference::ConvertIrValueToCodegenValue(const IrValue& ir_value, CodegenStorageKind storage) {
    std::string c_type = InferCType(ir_value.bit_width);
    return CodegenValue(ir_value.name, c_type, ir_value.bit_width, storage);
}

CodegenExpr CodegenIrInference::ConvertIrExprToCodegenExpr(const IrExpr& ir_expr) {
    CodegenExprKind kind = ConvertIrExprKindToCodegenKind(ir_expr.kind);
    std::string op = ConvertIrOpToCOp(ir_expr.kind);
    
    // Convert arguments
    std::vector<CodegenValue> args;
    for (const auto& ir_arg : ir_expr.args) {
        // For now, assume all arguments are locals (will be refined based on context)
        args.push_back(ConvertIrValueToCodegenValue(ir_arg, CodegenStorageKind::Local));
    }
    
    return CodegenExpr(kind, op, args);
}

bool CodegenIrInference::IsOscillatorLike(const BehaviorDescriptor& behavior) {
    // Check if the behavior matches oscillator-like patterns
    // This is a simple heuristic for now - can be enhanced later
    if (behavior.behavior_kind == BehaviorKind::Counter) {
        // Check if any port role suggests phase-related functionality
        for (const auto& port : behavior.ports) {
            if (port.role == "phase" || port.port_name.find("phase") != std::string::npos ||
                port.port_name.find("Phase") != std::string::npos) {
                return true;
            }
        }
    }
    
    // Look for behavioral descriptions that suggest oscillator behavior
    std::string desc_lower = behavior.description;
    std::transform(desc_lower.begin(), desc_lower.end(), desc_lower.begin(), ::tolower);
    
    if (desc_lower.find("oscillator") != std::string::npos ||
        desc_lower.find("sine") != std::string::npos ||
        desc_lower.find("wave") != std::string::npos ||
        desc_lower.find("frequency") != std::string::npos) {
        return true;
    }
    
    return false;
}

Result<CodegenModule> CodegenIrInference::BuildCodegenModuleForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id
) {
    // Create circuit facade to access necessary analysis methods
    CircuitFacade circuit_facade;
    
    // Get the IR module for the block
    Result<IrModule> ir_result = circuit_facade.BuildIrForBlockInBranch(
        session, session_dir, branch_name, block_id
    );
    
    if (!ir_result.ok) {
        return Result<CodegenModule>::Error(
            ir_result.error_code,
            "Failed to build IR module for block: " + ir_result.error_message
        );
    }
    
    // Get behavioral analysis for the block
    Result<BehaviorDescriptor> behavior_result = circuit_facade.InferBehaviorForBlockInBranch(
        session, session_dir, branch_name, block_id
    );
    
    if (!behavior_result.ok) {
        return Result<CodegenModule>::Error(
            behavior_result.error_code,
            "Failed to infer behavior for block: " + behavior_result.error_message
        );
    }
    
    // Build the CodegenModule from IR and behavior
    return BuildFromIrModuleAndBehavior(ir_result.data, behavior_result.data);
}

Result<CodegenModule> CodegenIrInference::BuildCodegenModuleForNodeRegionInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id,
    const std::vector<std::string>& node_ids
) {
    // Create circuit facade to access necessary analysis methods
    CircuitFacade circuit_facade;
    
    // Get the IR module for the node region
    // Using first node as a reference point, with default kind hint
    std::string node_kind_hint = "";  // Empty for automatic detection
    int max_depth = 4;  // Default depth for region
    
    Result<IrModule> ir_result = circuit_facade.BuildIrForNodeRegionInBranch(
        session, session_dir, branch_name, block_id, node_kind_hint, max_depth
    );
    
    if (!ir_result.ok) {
        return Result<CodegenModule>::Error(
            ir_result.error_code,
            "Failed to build IR module for node region: " + ir_result.error_message
        );
    }
    
    // Get behavioral analysis for one of the nodes (first one)
    if (node_ids.empty()) {
        return Result<CodegenModule>::Error(
            ErrorCode::InvalidArgument,
            "Node IDs list is empty"
        );
    }
    
    Result<BehaviorDescriptor> behavior_result = circuit_facade.InferBehaviorForNodeInBranch(
        session, session_dir, branch_name, node_ids[0], "Node"
    );
    
    if (!behavior_result.ok) {
        return Result<CodegenModule>::Error(
            behavior_result.error_code,
            "Failed to infer behavior for node: " + behavior_result.error_message
        );
    }
    
    // Build the CodegenModule from IR and behavior
    return BuildFromIrModuleAndBehavior(ir_result.data, behavior_result.data);
}

Result<CodegenModule> CodegenIrInference::BuildFromIrModuleAndBehavior(
    const IrModule& ir_module,
    const BehaviorDescriptor& behavior
) {
    CodegenModule module(ir_module.id, ir_module.id);  // Using same ID as placeholder
    
    // Convert inputs
    for (const auto& ir_input : ir_module.inputs) {
        CodegenValue input = ConvertIrValueToCodegenValue(ir_input, CodegenStorageKind::Input);
        module.inputs.push_back(input);
    }
    
    // Convert outputs 
    for (const auto& ir_output : ir_module.outputs) {
        CodegenValue output = ConvertIrValueToCodegenValue(ir_output, CodegenStorageKind::Output);
        module.outputs.push_back(output);
    }
    
    // Process combinational assignments
    for (const auto& ir_comb : ir_module.comb_assigns) {
        // Convert the expression
        CodegenExpr expr = ConvertIrExprToCodegenExpr(ir_comb);
        
        // Find if the target already exists in inputs, outputs, or create as local
        CodegenStorageKind target_storage = CodegenStorageKind::Local;
        for (const auto& input : module.inputs) {
            if (input.name == ir_comb.target.name) {
                target_storage = CodegenStorageKind::Input;
                break;
            }
        }
        if (target_storage == CodegenStorageKind::Local) {
            for (const auto& output : module.outputs) {
                if (output.name == ir_comb.target.name) {
                    target_storage = CodegenStorageKind::Output;
                    break;
                }
            }
        }
        
        CodegenValue target = ConvertIrValueToCodegenValue(ir_comb.target, target_storage);
        module.comb_assigns.push_back(CodegenAssignment(target, expr));
        
        // If not input/output, add as local if not already present
        if (target_storage == CodegenStorageKind::Local) {
            bool found = false;
            for (const auto& local : module.locals) {
                if (local.name == target.name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                module.locals.push_back(target);
            }
        }
    }
    
    // Process register assignments (add to state)
    for (const auto& ir_reg : ir_module.reg_assigns) {
        // Add target as state
        CodegenValue state_val = ConvertIrValueToCodegenValue(ir_reg.target, CodegenStorageKind::State);
        bool found = false;
        for (const auto& state : module.state) {
            if (state.name == state_val.name) {
                found = true;
                break;
            }
        }
        if (!found) {
            module.state.push_back(state_val);
        }
        
        // Convert next-state expression
        CodegenExpr next_expr = ConvertIrExprToCodegenExpr(ir_reg.expr);
        module.state_updates.push_back(CodegenAssignment(state_val, next_expr));
    }
    
    // Set oscillator flag based on behavior
    module.is_oscillator_like = IsOscillatorLike(behavior);
    
    // Set behavior summary
    module.behavior_summary = behavior.description;
    
    return Result<CodegenModule>::Success(module);
}

} // namespace ProtoVMCLI