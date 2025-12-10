#ifndef _ProtoVM_CodeEmitter_h_
#define _ProtoVM_CodeEmitter_h_

#include "CodegenIr.h"
#include "CodegenCpp.h"  // For CppClassOptions
#include "AudioDsl.h"    // For AudioDsl structures
#include "SessionTypes.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

class CodeEmitter {
public:
    static Result<std::string> EmitCodeForModule(
        const CodegenModule& module,
        CodegenTargetLanguage lang,
        bool emit_state_struct = true,
        const std::string& state_struct_name = "BlockState",
        const std::string& function_name = "BlockStep"
    );

    // Optional: emit a small demo wrapper for oscillator-like modules.
    static Result<std::string> EmitOscillatorDemo(
        const CodegenModule& module,
        CodegenTargetLanguage lang,
        const std::string& state_struct_name = "OscState",
        const std::string& step_function_name = "OscStep",
        const std::string& render_function_name = "OscRender"
    );

    // New: emit C++ class wrapper.
    static Result<std::string> EmitCppClassForModule(
        const CodegenModule& module,
        const CppClassOptions& options
    );

    // New: emit audio demo for oscillator.
    static Result<std::string> EmitAudioDemoForOscillator(
        const CodegenModule& module,
        const CppClassOptions& class_opts,
        const AudioDslGraph& graph
    );
    
private:
    // Helper to generate C/C++ type declaration for a CodegenValue
    static std::string GenerateTypeDeclaration(const CodegenValue& value);
    
    // Helper to generate expression string from CodegenExpr
    static std::string GenerateExpression(const CodegenExpr& expr);
    
    // Helper to generate assignment statement
    static std::string GenerateAssignment(const CodegenAssignment& assign);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CodeEmitter_h_