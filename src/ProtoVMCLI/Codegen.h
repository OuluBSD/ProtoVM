#ifndef _ProtoVM_Codegen_h_
#define _ProtoVM_Codegen_h_

#include <string>
#include <vector>

namespace ProtoVMCLI {

// Code generation module structure
struct CodegenModule {
    std::string id;
    std::string name;
    std::string flavor;  // e.g., "PseudoVerilog", "VHDL", etc.
    std::string code;    // Generated code text
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_Codegen_h_