#ifndef _ProtoVM_DiffAnalysis_h_
#define _ProtoVM_DiffAnalysis_h_

#include "BehavioralAnalysis.h"
#include "HlsIr.h"
#include "SessionTypes.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

// Enum for behavior change types
enum class BehaviorChangeKind {
    None,
    BehaviorKindChanged,
    BitWidthChanged,
    PortsChanged,
    DescriptionChanged,
    MultipleChanges
};

// Enum for IR change types
enum class IrChangeKind {
    None,
    InterfaceChanged,     // inputs/outputs changed
    CombLogicChanged,     // expressions changed
    RegLogicChanged,      // reg_assigns changed
    MultipleChanges
};

// Detailed port-level change information
struct PortChange {
    std::string port_name;
    std::string before_role;    // e.g. "data_in"
    std::string after_role;
    int before_width;   // -1 if unknown
    int after_width;

    PortChange() : port_name(""), before_role(""), after_role(""), before_width(-1), after_width(-1) {}
    PortChange(const std::string& name, const std::string& before_r, const std::string& after_r, int before_w, int after_w)
        : port_name(name), before_role(before_r), after_role(after_r), before_width(before_w), after_width(after_w) {}
};

// Behavior diff structure
struct BehaviorDiff {
    std::string subject_id;             // block or node id
    std::string subject_kind;           // "Block", "Pin", "Component", "Net"
    BehaviorChangeKind change_kind;
    BehaviorDescriptor before_behavior;
    BehaviorDescriptor after_behavior;
    std::vector<PortChange> port_changes; // detailed port-level changes

    BehaviorDiff() : subject_id(""), subject_kind(""), change_kind(BehaviorChangeKind::None) {}
    BehaviorDiff(const std::string& id, const std::string& kind, BehaviorChangeKind change_k,
                 const BehaviorDescriptor& before, const BehaviorDescriptor& after,
                 const std::vector<PortChange>& changes)
        : subject_id(id), subject_kind(kind), change_kind(change_k),
          before_behavior(before), after_behavior(after), port_changes(changes) {}
};

// IR expression change
struct IrExprChange {
    std::string target_name;       // e.g. "SUM"
    std::string before_expr_repr;  // stringified expression
    std::string after_expr_repr;

    IrExprChange() : target_name(""), before_expr_repr(""), after_expr_repr("") {}
    IrExprChange(const std::string& name, const std::string& before, const std::string& after)
        : target_name(name), before_expr_repr(before), after_expr_repr(after) {}
};

// IR register change
struct IrRegChange {
    std::string target_name;       // e.g. "Q"
    std::string before_expr_repr;
    std::string after_expr_repr;

    IrRegChange() : target_name(""), before_expr_repr(""), after_expr_repr("") {}
    IrRegChange(const std::string& name, const std::string& before, const std::string& after)
        : target_name(name), before_expr_repr(before), after_expr_repr(after) {}
};

// IR interface change
struct IrInterfaceChange {
    std::vector<IrValue> added_inputs;
    std::vector<IrValue> removed_inputs;
    std::vector<IrValue> added_outputs;
    std::vector<IrValue> removed_outputs;

    IrInterfaceChange() {}
};

// IR diff structure
struct IrDiff {
    std::string module_id;         // id of IrModule (block id, region id)
    IrChangeKind change_kind;
    IrInterfaceChange iface_changes;
    std::vector<IrExprChange> comb_changes;
    std::vector<IrRegChange> reg_changes;

    IrDiff() : module_id(""), change_kind(IrChangeKind::None) {}
    IrDiff(const std::string& id, IrChangeKind change_k,
           const IrInterfaceChange& iface_ch, const std::vector<IrExprChange>& comb_ch,
           const std::vector<IrRegChange>& reg_ch)
        : module_id(id), change_kind(change_k), iface_changes(iface_ch), comb_changes(comb_ch), reg_changes(reg_ch) {}
};

class DiffAnalysis {
public:
    // Compare two behaviors
    static Result<BehaviorDiff> DiffBehavior(
        const BehaviorDescriptor& before,
        const BehaviorDescriptor& after
    );

    // Compare two IR modules
    static Result<IrDiff> DiffIrModule(
        const IrModule& before,
        const IrModule& after
    );

private:
    // Helper to convert IrExpr to string representation
    static std::string IrExprToString(const IrExpr& expr);

    // Helper to convert IrRegAssign to string representation
    static std::string IrRegAssignToString(const IrRegAssign& reg_assign);

    // Helper to find IrValue in a vector by name
    static const IrValue* FindIrValueByName(const std::vector<IrValue>& values, const std::string& name);

    // Helper to check if IrValue exists in a vector
    static bool ContainsIrValue(const std::vector<IrValue>& values, const IrValue& value);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_DiffAnalysis_h_