#ifndef _ProtoVM_CodegenIr_h_
#define _ProtoVM_CodegenIr_h_

#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class CodegenTargetLanguage {
    C,
    Cpp
};

enum class CodegenExprKind {
    Value,      // direct variable / constant
    UnaryOp,
    BinaryOp,
    TernaryOp,  // e.g. mux / select
    Call        // intrinsic or helper function call (e.g. sinf, cosf)
};

enum class CodegenStorageKind {
    Input,
    Output,
    Local,
    State      // persistent register / memory across ticks
};

struct CodegenValue {
    std::string name;
    std::string c_type;       // e.g. "int", "float", "uint32_t"
    int bit_width;       // -1 if unknown
    CodegenStorageKind storage;
    bool is_array = false;
    int array_length = -1; // optional

    CodegenValue() : name(""), c_type("int"), bit_width(-1), storage(CodegenStorageKind::Local), is_array(false), array_length(-1) {}
    CodegenValue(const std::string& n, const std::string& type, int width, CodegenStorageKind s,
                 bool arr = false, int arr_len = -1)
        : name(n), c_type(type), bit_width(width), storage(s), is_array(arr), array_length(arr_len) {}
};

struct CodegenExpr {
    CodegenExprKind kind;

    std::string op;                   // "+", "-", "*", "&", "|", "^", "==", "?:", "call_name", etc.
    std::vector<CodegenValue> args;   // references to inputs/locals/state
    std::string literal;              // for constants, if needed (e.g. "0.5f")

    CodegenExpr() : kind(CodegenExprKind::Value) {}
    CodegenExpr(CodegenExprKind k, const std::string& o,
                const std::vector<CodegenValue>& a, const std::string& lit = "")
        : kind(k), op(o), args(a), literal(lit) {}
};

struct CodegenAssignment {
    CodegenValue target;
    CodegenExpr  expr;

    CodegenAssignment() {}
    CodegenAssignment(const CodegenValue& t, const CodegenExpr& e)
        : target(t), expr(e) {}
};

struct CodegenModule {
    std::string id;
    std::string block_id;

    // State and interface
    std::vector<CodegenValue> inputs;
    std::vector<CodegenValue> outputs;
    std::vector<CodegenValue> locals;
    std::vector<CodegenValue> state;      // persistent registers

    // Combinational computations for one tick
    std::vector<CodegenAssignment> comb_assigns;

    // Sequential updates (state <- next state)
    std::vector<CodegenAssignment> state_updates;

    // Optional metadata
    bool is_oscillator_like = false;
    std::string behavior_summary;        // from BehavioralAnalysis

    CodegenModule() : id(""), block_id(""), is_oscillator_like(false), behavior_summary("") {}
    CodegenModule(const std::string& i, const std::string& bid)
        : id(i), block_id(bid), is_oscillator_like(false), behavior_summary("") {}
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CodegenIr_h_