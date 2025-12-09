#include "DiffAnalysis.h"
#include "HlsIr.h"
#include <algorithm>
#include <unordered_map>
#include <sstream>

namespace ProtoVMCLI {

Result<BehaviorDiff> DiffAnalysis::DiffBehavior(
    const BehaviorDescriptor& before,
    const BehaviorDescriptor& after
) {
    try {
        // Verify that subject IDs match (basic validation)
        if (before.subject_id != after.subject_id) {
            return Result<BehaviorDiff>::MakeError(
                ErrorCode::InvalidInput,
                "Cannot diff behaviors with different subject IDs"
            );
        }

        // Initialize the result
        BehaviorDiff diff;
        diff.subject_id = before.subject_id;
        diff.subject_kind = before.subject_kind;  // Assuming these are the same

        // Compare behavior kinds
        bool behavior_kind_changed = (before.behavior_kind != after.behavior_kind);

        // Compare bit widths
        bool bit_width_changed = (before.bit_width != after.bit_width);

        // Compare descriptions
        bool description_changed = (before.description != after.description);

        // Compare ports
        std::vector<PortChange> port_changes;

        // Create maps of port names to port roles for efficient comparison
        std::unordered_map<std::string, BehaviorPortRole> before_ports, after_ports;
        for (const auto& port : before.ports) {
            before_ports[port.port_name] = port;
        }
        for (const auto& port : after.ports) {
            after_ports[port.port_name] = port;
        }

        // Check for port changes
        std::unordered_set<std::string> all_port_names;
        for (const auto& pair : before_ports) all_port_names.insert(pair.first);
        for (const auto& pair : after_ports) all_port_names.insert(pair.first);

        bool ports_changed = false;
        for (const auto& port_name : all_port_names) {
            auto before_it = before_ports.find(port_name);
            auto after_it = after_ports.find(port_name);

            if (before_it == before_ports.end()) {
                // New port added - only role and width are relevant in after
                port_changes.emplace_back(port_name, "", after_it->second.role, -1, -1);
                ports_changed = true;
            } else if (after_it == after_ports.end()) {
                // Port removed - only role and width are relevant in before
                port_changes.emplace_back(port_name, before_it->second.role, "", -1, -1);
                ports_changed = true;
            } else {
                // Port exists in both - check for changes
                bool role_changed = (before_it->second.role != after_it->second.role);
                // Note: We don't have port width in BehaviorPortRole, so can't compare that directly
                if (role_changed) {
                    port_changes.emplace_back(port_name, before_it->second.role, after_it->second.role, -1, -1);
                    ports_changed = true;
                }
            }
        }

        // Determine the overall change kind
        std::vector<BehaviorChangeKind> change_kinds;
        if (behavior_kind_changed) change_kinds.push_back(BehaviorChangeKind::BehaviorKindChanged);
        if (bit_width_changed) change_kinds.push_back(BehaviorChangeKind::BitWidthChanged);
        if (ports_changed) change_kinds.push_back(BehaviorChangeKind::PortsChanged);
        if (description_changed) change_kinds.push_back(BehaviorChangeKind::DescriptionChanged);

        if (change_kinds.empty()) {
            diff.change_kind = BehaviorChangeKind::None;
        } else if (change_kinds.size() == 1) {
            diff.change_kind = change_kinds[0];
        } else {
            diff.change_kind = BehaviorChangeKind::MultipleChanges;
        }

        // Set the before/after behavior descriptors
        diff.before_behavior = before;
        diff.after_behavior = after;

        // Set the port changes
        diff.port_changes = port_changes;

        return Result<BehaviorDiff>::MakeOk(diff);
    }
    catch (const std::exception& e) {
        return Result<BehaviorDiff>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in DiffBehavior: ") + e.what()
        );
    }
}

Result<IrDiff> DiffAnalysis::DiffIrModule(
    const IrModule& before,
    const IrModule& after
) {
    try {
        // Initialize the result
        IrDiff diff;
        diff.module_id = before.id;  // Assuming these are the same or using before.id

        // Compare interfaces (inputs and outputs)
        IrInterfaceChange iface_changes;

        // Find added and removed inputs
        for (const auto& input : after.inputs) {
            if (!ContainsIrValue(before.inputs, input)) {
                iface_changes.added_inputs.push_back(input);
            }
        }
        for (const auto& input : before.inputs) {
            if (!ContainsIrValue(after.inputs, input)) {
                iface_changes.removed_inputs.push_back(input);
            }
        }

        // Find added and removed outputs
        for (const auto& output : after.outputs) {
            if (!ContainsIrValue(before.outputs, output)) {
                iface_changes.added_outputs.push_back(output);
            }
        }
        for (const auto& output : before.outputs) {
            if (!ContainsIrValue(after.outputs, output)) {
                iface_changes.removed_outputs.push_back(output);
            }
        }

        // Compare combinational expressions
        std::vector<IrExprChange> comb_changes;

        // Create maps of target names to expressions for efficient comparison
        std::unordered_map<std::string, IrExpr> before_comb, after_comb;
        for (const auto& expr : before.comb_assigns) {
            before_comb[expr.target.name] = expr;
        }
        for (const auto& expr : after.comb_assigns) {
            after_comb[expr.target.name] = expr;
        }

        // Check for comb expression changes
        std::unordered_set<std::string> all_comb_targets;
        for (const auto& pair : before_comb) all_comb_targets.insert(pair.first);
        for (const auto& pair : after_comb) all_comb_targets.insert(pair.first);

        for (const auto& target_name : all_comb_targets) {
            auto before_it = before_comb.find(target_name);
            auto after_it = after_comb.find(target_name);

            if (before_it == before_comb.end()) {
                // New expression added
                comb_changes.emplace_back(target_name, "", IrExprToString(after_it->second));
            } else if (after_it == after_comb.end()) {
                // Expression removed
                comb_changes.emplace_back(target_name, IrExprToString(before_it->second), "");
            } else {
                // Expression exists in both - check for changes
                std::string before_repr = IrExprToString(before_it->second);
                std::string after_repr = IrExprToString(after_it->second);
                if (before_repr != after_repr) {
                    comb_changes.emplace_back(target_name, before_repr, after_repr);
                }
            }
        }

        // Compare register assignments
        std::vector<IrRegChange> reg_changes;

        // Create maps of target names to reg assignments for efficient comparison
        std::unordered_map<std::string, IrRegAssign> before_reg, after_reg;
        for (const auto& reg_assign : before.reg_assigns) {
            before_reg[reg_assign.target.name] = reg_assign;
        }
        for (const auto& reg_assign : after.reg_assigns) {
            after_reg[reg_assign.target.name] = reg_assign;
        }

        // Check for reg assignment changes
        std::unordered_set<std::string> all_reg_targets;
        for (const auto& pair : before_reg) all_reg_targets.insert(pair.first);
        for (const auto& pair : after_reg) all_reg_targets.insert(pair.first);

        for (const auto& target_name : all_reg_targets) {
            auto before_it = before_reg.find(target_name);
            auto after_it = after_reg.find(target_name);

            if (before_it == before_reg.end()) {
                // New reg assignment added
                reg_changes.emplace_back(target_name, "", IrRegAssignToString(after_it->second));
            } else if (after_it == after_reg.end()) {
                // Reg assignment removed
                reg_changes.emplace_back(target_name, IrRegAssignToString(before_it->second), "");
            } else {
                // Reg assignment exists in both - check for changes
                std::string before_repr = IrRegAssignToString(before_it->second);
                std::string after_repr = IrRegAssignToString(after_it->second);
                if (before_repr != after_repr) {
                    reg_changes.emplace_back(target_name, before_repr, after_repr);
                }
            }
        }

        // Determine the overall change kind
        bool interface_changed = (!iface_changes.added_inputs.empty() || 
                                  !iface_changes.removed_inputs.empty() || 
                                  !iface_changes.added_outputs.empty() || 
                                  !iface_changes.removed_outputs.empty());
        bool comb_logic_changed = !comb_changes.empty();
        bool reg_logic_changed = !reg_changes.empty();

        std::vector<IrChangeKind> change_kinds;
        if (interface_changed) change_kinds.push_back(IrChangeKind::InterfaceChanged);
        if (comb_logic_changed) change_kinds.push_back(IrChangeKind::CombLogicChanged);
        if (reg_logic_changed) change_kinds.push_back(IrChangeKind::RegLogicChanged);

        if (change_kinds.empty()) {
            diff.change_kind = IrChangeKind::None;
        } else if (change_kinds.size() == 1) {
            diff.change_kind = change_kinds[0];
        } else {
            diff.change_kind = IrChangeKind::MultipleChanges;
        }

        // Set the changes
        diff.iface_changes = iface_changes;
        diff.comb_changes = comb_changes;
        diff.reg_changes = reg_changes;

        return Result<IrDiff>::MakeOk(diff);
    }
    catch (const std::exception& e) {
        return Result<IrDiff>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in DiffIrModule: ") + e.what()
        );
    }
}

std::string DiffAnalysis::IrExprToString(const IrExpr& expr) {
    std::ostringstream oss;
    
    // Convert expression kind to string
    std::string op_str;
    switch (expr.kind) {
        case IrExprKind::Value: op_str = "="; break;
        case IrExprKind::Not: op_str = "~"; break;
        case IrExprKind::And: op_str = "&"; break;
        case IrExprKind::Or: op_str = "|"; break;
        case IrExprKind::Xor: op_str = "^"; break;
        case IrExprKind::Add: op_str = "+"; break;
        case IrExprKind::Sub: op_str = "-"; break;
        case IrExprKind::Mux: op_str = "?"; break;
        case IrExprKind::Eq: op_str = "=="; break;
        case IrExprKind::Neq: op_str = "!="; break;
    }
    
    oss << expr.target.name << " = ";
    
    if (expr.args.size() == 1) {
        oss << op_str << expr.args[0].name;
    } else if (expr.args.size() == 2) {
        oss << expr.args[0].name << " " << op_str << " " << expr.args[1].name;
    } else if (expr.args.size() == 3 && expr.kind == IrExprKind::Mux) {
        // Mux: sel ? a : b
        oss << expr.args[0].name << " ? " << expr.args[1].name << " : " << expr.args[2].name;
    } else if (expr.args.empty()) {
        oss << expr.target.name;  // Just the value itself
    } else {
        // For cases with more than 2 args, show in a generic format
        oss << op_str << "(";
        for (size_t i = 0; i < expr.args.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << expr.args[i].name;
        }
        oss << ")";
    }
    
    return oss.str();
}

std::string DiffAnalysis::IrRegAssignToString(const IrRegAssign& reg_assign) {
    std::ostringstream oss;
    oss << reg_assign.target.name << " = ";
    oss << IrExprToString(reg_assign.expr);
    
    if (!reg_assign.clock.empty()) {
        oss << " [clock: " << reg_assign.clock;
        if (!reg_assign.reset.empty()) {
            oss << ", reset: " << reg_assign.reset;
        }
        oss << "]";
    } else if (!reg_assign.reset.empty()) {
        oss << " [reset: " << reg_assign.reset << "]";
    }
    
    return oss.str();
}

const IrValue* DiffAnalysis::FindIrValueByName(const std::vector<IrValue>& values, const std::string& name) {
    for (const auto& value : values) {
        if (value.name == name) {
            return &value;
        }
    }
    return nullptr;
}

bool DiffAnalysis::ContainsIrValue(const std::vector<IrValue>& values, const IrValue& value) {
    for (const auto& v : values) {
        if (v.name == value.name && v.bit_width == value.bit_width) {
            // For simplicity, we compare name and bit_width
            // In a more detailed comparison, we might also compare is_literal and literal values
            return true;
        }
    }
    return false;
}

} // namespace ProtoVMCLI