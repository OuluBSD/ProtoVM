#include "DiffAnalysis.h"
#include "BehavioralAnalysis.h"
#include "HlsIr.h"
#include <iostream>
#include <cassert>

using namespace ProtoVMCLI;

void TestBehaviorDiff() {
    std::cout << "Testing behavior diff functionality..." << std::endl;

    // Create two behavior descriptors with some differences
    std::vector<BehaviorPortRole> ports1 = {
        BehaviorPortRole("A", "data_in"),
        BehaviorPortRole("B", "data_in"),
        BehaviorPortRole("SUM", "data_out")
    };
    
    BehaviorDescriptor before("B1", "Block", BehaviorKind::Adder, ports1, 4, "4-bit adder");
    
    std::vector<BehaviorPortRole> ports2 = {
        BehaviorPortRole("A", "data_in"),
        BehaviorPortRole("B", "data_in"),
        BehaviorPortRole("SUM", "data_out"),
        BehaviorPortRole("COUT", "carry_out")  // Added port
    };
    
    BehaviorDescriptor after("B1", "Block", BehaviorKind::Adder, ports2, 4, "4-bit adder with carry");  // Changed description

    // Test the diff function
    auto diff_result = DiffAnalysis::DiffBehavior(before, after);
    assert(diff_result.ok);
    
    const BehaviorDiff& diff = diff_result.data;
    assert(diff.subject_id == "B1");
    assert(diff.change_kind == BehaviorChangeKind::MultipleChanges);  // Multiple changes: added port and changed description
    assert(diff.port_changes.size() == 1);  // One new port added
    assert(diff.port_changes[0].port_name == "COUT");
    assert(diff.port_changes[0].before_role == "");  // New port in 'after'
    assert(diff.port_changes[0].after_role == "carry_out");

    std::cout << "Behavior diff test passed!" << std::endl;
}

void TestIrDiff() {
    std::cout << "Testing IR diff functionality..." << std::endl;

    // Create two IR modules with differences
    std::vector<IrValue> inputs1 = {IrValue("A", 4), IrValue("B", 4)};
    std::vector<IrValue> outputs1 = {IrValue("SUM", 4)};
    std::vector<IrExpr> comb1 = {IrExpr(IrExprKind::Add, IrValue("SUM", 4), {IrValue("A", 4), IrValue("B", 4)})};
    std::vector<IrRegAssign> reg1 = {};
    
    IrModule before("M1", inputs1, outputs1, comb1, reg1);

    // After: added a new input and changed the expression
    std::vector<IrValue> inputs2 = {IrValue("A", 4), IrValue("B", 4), IrValue("CIN", 1)};  // Added CIN
    std::vector<IrValue> outputs2 = {IrValue("SUM", 4)};
    std::vector<IrExpr> comb2 = {IrExpr(IrExprKind::Add, IrValue("SUM", 4), {IrValue("A", 4), IrValue("B", 4), IrValue("CIN", 1)})};  // Changed expression
    std::vector<IrRegAssign> reg2 = {};
    
    IrModule after("M1", inputs2, outputs2, comb2, reg2);

    // Test the diff function
    auto diff_result = DiffAnalysis::DiffIrModule(before, after);
    assert(diff_result.ok);
    
    const IrDiff& diff = diff_result.data;
    assert(diff.module_id == "M1");
    assert(diff.change_kind == IrChangeKind::MultipleChanges);  // Interface and expression changed
    assert(diff.iface_changes.added_inputs.size() == 1);  // New input CIN
    assert(diff.iface_changes.added_inputs[0].name == "CIN");
    assert(diff.comb_changes.size() == 1);  // Expression changed
    assert(diff.comb_changes[0].target_name == "SUM");

    std::cout << "IR diff test passed!" << std::endl;
}

void TestIrDiffSame() {
    std::cout << "Testing IR diff for identical modules..." << std::endl;

    // Create two identical IR modules
    std::vector<IrValue> inputs = {IrValue("A", 4), IrValue("B", 4)};
    std::vector<IrValue> outputs = {IrValue("SUM", 4)};
    std::vector<IrExpr> comb = {IrExpr(IrExprKind::Add, IrValue("SUM", 4), {IrValue("A", 4), IrValue("B", 4)})};
    std::vector<IrRegAssign> reg = {};
    
    IrModule module1("M1", inputs, outputs, comb, reg);
    IrModule module2("M1", inputs, outputs, comb, reg);

    // Test the diff function
    auto diff_result = DiffAnalysis::DiffIrModule(module1, module2);
    assert(diff_result.ok);
    
    const IrDiff& diff = diff_result.data;
    assert(diff.module_id == "M1");
    assert(diff.change_kind == IrChangeKind::None);  // No changes
    assert(diff.iface_changes.added_inputs.empty());
    assert(diff.iface_changes.removed_inputs.empty());
    assert(diff.iface_changes.added_outputs.empty());
    assert(diff.iface_changes.removed_outputs.empty());
    assert(diff.comb_changes.empty());
    assert(diff.reg_changes.empty());

    std::cout << "Identical IR diff test passed!" << std::endl;
}

int main() {
    std::cout << "Starting diff engine tests..." << std::endl;
    
    TestBehaviorDiff();
    TestIrDiff();
    TestIrDiffSame();
    
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}