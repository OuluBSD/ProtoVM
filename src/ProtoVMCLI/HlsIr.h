#ifndef _ProtoVM_HlsIr_h_
#define _ProtoVM_HlsIr_h_

#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>

namespace ProtoVMCLI {

// Value reference in IR (wire, port, literal).
struct IrValue {
    std::string name;        // symbolic name (e.g. "A", "B", "SUM", "tmp1")
    int bit_width;      // -1 or 0 if unknown
    bool is_literal;    // true if this is a constant
    uint64_t literal;     // only valid if is_literal == true (for small constants)

    IrValue() : name(""), bit_width(-1), is_literal(false), literal(0) {}
    IrValue(const std::string& n, int width, bool is_lit = false, uint64_t lit = 0)
        : name(n), bit_width(width), is_literal(is_lit), literal(lit) {}
};

// Expression kinds.
enum class IrExprKind {
    Value,      // direct reference
    Not,
    And,
    Or,
    Xor,
    Add,
    Sub,
    Mux,        // ternary: sel ? a : b
    Eq,
    Neq,
    // extendable: AndReduce, OrReduce, etc.
};

struct IrExpr {
    IrExprKind kind;
    IrValue target;         // lhs (e.g. SUM)
    std::vector<IrValue> args;   // rhs operands (0,1,2,3 as needed)

    IrExpr() : kind(IrExprKind::Value) {}
    IrExpr(IrExprKind k, const IrValue& t, const std::vector<IrValue>& a)
        : kind(k), target(t), args(a) {}
};

// Simple sequential assignment (per clock edge).
struct IrRegAssign {
    IrValue target;     // register output
    IrExpr  expr;       // next-state expression
    std::string clock;       // clock signal name (if known)
    std::string reset;       // reset signal name (optional)

    IrRegAssign() : clock(""), reset("") {}
    IrRegAssign(const IrValue& t, const IrExpr& e, const std::string& clk = "", const std::string& rst = "")
        : target(t), expr(e), clock(clk), reset(rst) {}
};

// Top-level IR for a block/subcircuit.
struct IrModule {
    std::string id;                   // e.g. block ID or region ID
    std::vector<IrValue> inputs;
    std::vector<IrValue> outputs;
    std::vector<IrExpr>  comb_assigns;  // combinational equations
    std::vector<IrRegAssign> reg_assigns; // sequential updates

    IrModule() : id("") {}
    IrModule(const std::string& i, const std::vector<IrValue>& in, const std::vector<IrValue>& out,
             const std::vector<IrExpr>& comb, const std::vector<IrRegAssign>& reg)
        : id(i), inputs(in), outputs(out), comb_assigns(comb), reg_assigns(reg) {}
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_HlsIr_h_