#ifndef _ProtoVM_ScheduledIr_h_
#define _ProtoVM_ScheduledIr_h_

#include "HlsIr.h"
#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>

namespace ProtoVMCLI {

// A stage index in a pipeline (0, 1, 2, ...)
using StageIndex = int;

// Represents a scheduled operation derived from an IrExpr or IrRegAssign.
struct ScheduledOpId {
    std::string name;   // e.g. "SUM_add", "TMP1_and"
};

// Represents a scheduled expression with stage information.
struct ScheduledExpr {
    IrExpr expr;        // original expression
    StageIndex stage;   // stage where this expr is evaluated
    
    ScheduledExpr() : stage(0) {}
    ScheduledExpr(const IrExpr& e, StageIndex s) : expr(e), stage(s) {}
};

// Represents a scheduled register assignment with stage information.
struct ScheduledRegAssign {
    IrRegAssign reg_assign; // original reg assignment
    StageIndex stage;       // stage in which next-state value is considered "ready"
    
    ScheduledRegAssign() : stage(0) {}
    ScheduledRegAssign(const IrRegAssign& reg, StageIndex s) : reg_assign(reg), stage(s) {}
};

// Top-level scheduled IR for a block/subcircuit.
struct ScheduledModule {
    std::string id;                             // same as IrModule id
    std::vector<IrValue> inputs;
    std::vector<IrValue> outputs;

    // Stages: index from 0..(num_stages-1)
    int num_stages;

    // Expressions and regs annotated with stages.
    std::vector<ScheduledExpr> comb_ops;
    std::vector<ScheduledRegAssign> reg_ops;

    // Optional metadata:
    // - mapping from IrExpr.target.name to stage
    // - per-stage summary (op counts, etc.)
    
    ScheduledModule() : id(""), num_stages(1) {}
    ScheduledModule(const std::string& i, int stages,
                    const std::vector<IrValue>& in, const std::vector<IrValue>& out,
                    const std::vector<ScheduledExpr>& comb, const std::vector<ScheduledRegAssign>& reg)
        : id(i), inputs(in), outputs(out), num_stages(stages), comb_ops(comb), reg_ops(reg) {}
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_ScheduledIr_h_