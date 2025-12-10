#include "CodeEmitter.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace ProtoVMCLI {

std::string CodeEmitter::GenerateTypeDeclaration(const CodegenValue& value) {
    std::ostringstream oss;
    
    if (value.is_array) {
        if (value.array_length > 0) {
            oss << value.c_type << " " << value.name << "[" << value.array_length << "]";
        } else {
            oss << value.c_type << " " << value.name << "[]";  // Unsized array
        }
    } else {
        oss << value.c_type << " " << value.name;
    }
    
    return oss.str();
}

std::string CodeEmitter::GenerateExpression(const CodegenExpr& expr) {
    std::ostringstream oss;
    
    switch (expr.kind) {
        case CodegenExprKind::Value:
            if (expr.args.empty()) {
                // If no args, it might be a literal
                if (!expr.literal.empty()) {
                    return expr.literal;
                } else {
                    return "";  // No value to return
                }
            } else if (expr.args.size() == 1) {
                return expr.args[0].name;
            }
            break;
            
        case CodegenExprKind::UnaryOp:
            if (expr.args.size() == 1) {
                oss << expr.op << expr.args[0].name;
            }
            break;
            
        case CodegenExprKind::BinaryOp:
            if (expr.args.size() == 2) {
                oss << expr.args[0].name << " " << expr.op << " " << expr.args[1].name;
            }
            break;
            
        case CodegenExprKind::TernaryOp:
            // For MUX: condition ? true_val : false_val
            if (expr.args.size() == 3) {
                oss << expr.args[0].name << " ? " << expr.args[1].name << " : " << expr.args[2].name;
            }
            break;
            
        case CodegenExprKind::Call:
            // For function calls: func_name(arg1, arg2, ...)
            oss << expr.op << "(";
            for (size_t i = 0; i < expr.args.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << expr.args[i].name;
            }
            oss << ")";
            break;
    }
    
    return oss.str();
}

std::string CodeEmitter::GenerateAssignment(const CodegenAssignment& assign) {
    std::string expr_str = GenerateExpression(assign.expr);
    if (assign.target.storage == CodegenStorageKind::Output) {
        // For outputs, we deference the pointer
        return "*out_" + assign.target.name + " = " + expr_str + ";";
    } else if (assign.target.storage == CodegenStorageKind::State) {
        // For state, we access through the state struct
        return "s->" + assign.target.name + " = " + expr_str + ";";
    } else {
        // For locals and inputs, direct assignment
        return assign.target.name + " = " + expr_str + ";";
    }
}

Result<std::string> CodeEmitter::EmitCodeForModule(
    const CodegenModule& module,
    CodegenTargetLanguage lang,
    bool emit_state_struct,
    const std::string& state_struct_name,
    const std::string& function_name
) {
    std::ostringstream oss;
    
    // Add standard headers
    oss << "// Auto-generated code from ProtoVM codegen\n";
    oss << "#include <stdint.h>\n";
    oss << "#include <stdbool.h>\n\n";
    
    // Generate state struct if requested
    if (emit_state_struct && !module.state.empty()) {
        oss << "typedef struct {\n";
        for (const auto& state_val : module.state) {
            oss << "    " << GenerateTypeDeclaration(state_val) << ";\n";
        }
        oss << "} " << state_struct_name << ";\n\n";
    }
    
    // Generate function signature
    oss << "void " << function_name << "(";
    
    // Add state parameter if state exists
    if (emit_state_struct && !module.state.empty()) {
        oss << state_struct_name << "* s";
        
        // Add comma if we have inputs
        if (!module.inputs.empty()) {
            oss << ", ";
        }
    }
    
    // Add input parameters
    for (size_t i = 0; i < module.inputs.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << module.inputs[i].c_type << " in_" << module.inputs[i].name;
    }
    
    // Add output parameters (as pointers)
    for (size_t i = 0; i < module.outputs.size(); ++i) {
        if (i > 0 || !module.inputs.empty() || (emit_state_struct && !module.state.empty())) {
            oss << ", ";
        }
        oss << module.outputs[i].c_type << "* out_" << module.outputs[i].name;
    }
    
    oss << ") {\n";
    
    // Declare local variables
    for (const auto& local : module.locals) {
        oss << "    " << GenerateTypeDeclaration(local) << ";\n";
    }
    
    // Process combinational assignments
    for (const auto& assign : module.comb_assigns) {
        oss << "    " << GenerateAssignment(assign) << "\n";
    }
    
    // Process state updates
    for (const auto& assign : module.state_updates) {
        oss << "    " << GenerateAssignment(assign) << "\n";
    }
    
    oss << "}\n";
    
    return Result<std::string>::Success(oss.str());
}

Result<std::string> CodeEmitter::EmitOscillatorDemo(
    const CodegenModule& module,
    CodegenTargetLanguage lang,
    const std::string& state_struct_name,
    const std::string& step_function_name,
    const std::string& render_function_name
) {
    if (!module.is_oscillator_like) {
        return Result<std::string>::Error(
            ErrorCode::InvalidArgument,
            "Module is not oscillator-like, cannot generate oscillator demo"
        );
    }
    
    std::ostringstream oss;
    
    // Add standard headers
    oss << "// Oscillator demo code generated from ProtoVM\n";
    oss << "#include <stdint.h>\n";
    oss << "#include <stdbool.h>\n";
    oss << "#include <math.h>  // For sin function\n\n";
    
    // First, emit the main oscillator step function
    auto step_result = EmitCodeForModule(
        module, lang, true, state_struct_name, step_function_name
    );
    if (!step_result.ok) {
        return Result<std::string>::Error(
            step_result.error_code,
            step_result.error_message
        );
    }
    
    oss << step_result.data;
    oss << "\n";
    
    // Generate render function that calls step function multiple times
    oss << "// Render n samples of oscillator output\n";
    oss << "void " << render_function_name << "(" 
        << state_struct_name << "* s, float* outL, float* outR, int n) {\n";
    oss << "    for (int i = 0; i < n; ++i) {\n";
    oss << "        float sample = 0.0f;  // Placeholder - actual oscillator output\n";
    oss << "        // Call the oscillator step function\n";
    oss << "        " << step_function_name << "(s, &sample);\n";
    oss << "        \n";
    oss << "        // Simple stereo panning using sin LFO\n";
    oss << "        float pan_lfo = sinf(s->phase * 0.1f);  // Simple LFO based on phase\n";
    oss << "        float pan = (pan_lfo + 1.0f) * 0.5f;  // Normalize to 0-1 range\n";
    oss << "        float left_gain = 0.5f * (1.0f - pan);\n";
    oss << "        float right_gain = 0.5f * (1.0f + pan);\n";
    oss << "        \n";
    oss << "        outL[i] = sample * left_gain;\n";
    oss << "        outR[i] = sample * right_gain;\n";
    oss << "    }\n";
    oss << "}\n";
    
    return Result<std::string>::Success(oss.str());
}

Result<std::string> CodeEmitter::EmitCppClassForModule(
    const CodegenModule& module,
    const CppClassOptions& options
) {
    std::ostringstream oss;

    // Add standard headers
    oss << "// Auto-generated C++ class from ProtoVM codegen\n";
    oss << "#include <stdint.h>\n";
    oss << "#include <stdbool.h>\n";
    oss << "#include <cmath>\n\n";

    // Emit namespace if specified
    if (!options.namespace_name.empty()) {
        oss << "namespace " << options.namespace_name << " {\n\n";
    }

    // Generate state struct
    oss << "struct " << options.state_class_name << " {\n";
    for (const auto& state_val : module.state) {
        oss << "    " << GenerateTypeDeclaration(state_val) << ";\n";
    }
    oss << "};\n\n";

    // Generate class declaration
    oss << "class " << options.class_name << " {\n";
    oss << "public:\n";
    oss << "    " << options.class_name << "() { /* optional ctor/init */ }\n\n";

    // Generate Step method
    oss << "    void " << options.step_method_name
        << "(" << options.state_class_name << "& s, float* outL, float* outR, double sample_rate);\n";

    // Generate Render method if requested
    if (options.generate_render_method) {
        oss << "    void " << options.render_method_name
            << "(" << options.state_class_name << "& s,\n";
        oss << "                  float* outL, float* outR,\n";
        oss << "                  int num_samples,\n";
        oss << "                  double sample_rate);\n";
    }

    oss << "\nprivate:\n";
    oss << "    // Internal computations can be kept here if needed\n";
    oss << "};\n\n";

    // Generate Step method implementation
    oss << "void " << options.class_name << "::"
        << options.step_method_name
        << "(" << options.state_class_name << "& s, float* outL, float* outR, double sample_rate) {\n";

    // Declare local variables
    for (const auto& local : module.locals) {
        oss << "    " << GenerateTypeDeclaration(local) << ";\n";
    }

    // Process combinational assignments
    for (const auto& assign : module.comb_assigns) {
        oss << "    " << GenerateAssignment(assign) << "\n";
    }

    // Process state updates
    for (const auto& assign : module.state_updates) {
        oss << "    " << GenerateAssignment(assign) << "\n";
    }

    // Set default output values if no explicit output assignments were made in comb_assigns
    bool has_output_assignment = false;
    for (const auto& assign : module.comb_assigns) {
        if (assign.target.storage == CodegenStorageKind::Output) {
            has_output_assignment = true;
            break;
        }
    }

    if (!has_output_assignment && !module.outputs.empty()) {
        oss << "    // Default output assignment - you may need to customize this\n";
        for (size_t i = 0; i < module.outputs.size(); ++i) {
            if (i == 0) {
                oss << "    *outL = 0.0f;  // Default left output\n";
            } else if (i == 1) {
                oss << "    *outR = 0.0f;  // Default right output\n";
            } else {
                oss << "    // Additional output " << module.outputs[i].name << " not handled\n";
            }
        }
    }

    oss << "}\n\n";

    // Generate Render method implementation if requested
    if (options.generate_render_method) {
        oss << "void " << options.class_name << "::"
            << options.render_method_name
            << "(" << options.state_class_name << "& s,\n";
        oss << "                      float* outL, float* outR,\n";
        oss << "                      int num_samples,\n";
        oss << "                      double sample_rate)\n";
        oss << "{\n";
        oss << "    for (int i = 0; i < num_samples; ++i) {\n";
        oss << "        float L = 0.0f, R = 0.0f;\n";
        oss << "        " << options.step_method_name << "(s, &L, &R, sample_rate);\n";
        oss << "        outL[i] = L;\n";
        oss << "        outR[i] = R;\n";
        oss << "    }\n";
        oss << "}\n\n";
    }

    // Close namespace if it was opened
    if (!options.namespace_name.empty()) {
        oss << "} // namespace " << options.namespace_name << "\n";
    }

    return Result<std::string>::Success(oss.str());
}

Result<std::string> CodeEmitter::EmitAudioDemoForOscillator(
    const CodegenModule& module,
    const CppClassOptions& class_opts,
    const AudioDslGraph& graph
) {
    if (!module.is_oscillator_like) {
        return Result<std::string>::Error(
            ErrorCode::InvalidArgument,
            "Module is not oscillator-like, cannot generate audio demo"
        );
    }

    std::ostringstream oss;

    // Add standard headers
    oss << "// Audio demo for oscillator generated from ProtoVM\n";
    oss << "#include <cmath>\n";
    oss << "#include <vector>\n";
    oss << "#include <cstdio>\n";
    oss << "#include <fstream>\n\n";

    // Include the C++ class we're going to define
    oss << "// Generated C++ class for the oscillator\n";
    auto class_result = EmitCppClassForModule(module, class_opts);
    if (!class_result.ok) {
        return Result<std::string>::Error(
            class_result.error_code,
            class_result.error_message
        );
    }
    oss << class_result.data << "\n";

    // Define main/demo function
    oss << "void demo_audio() {\n";
    oss << "    const double sample_rate = " << graph.output.sample_rate_hz << ";\n";
    oss << "    const double duration = " << graph.output.duration_sec << ";\n";
    oss << "    const int num_samples = static_cast<int>("
        << graph.output.sample_rate_hz << " * " << graph.output.duration_sec << ");\n\n";

    oss << "    // Initialize state\n";
    oss << "    " << class_opts.state_class_name << " state{};\n";
    oss << "    // TODO: Initialize any oscillator-specific state here if needed\n\n";

    oss << "    // Create output buffers\n";
    oss << "    std::vector<float> left_buffer(num_samples);\n";
    oss << "    std::vector<float> right_buffer(num_samples);\n\n";

    oss << "    // Create oscillator instance\n";
    oss << "    " << class_opts.class_name << " osc;\n\n";

    oss << "    // Generate audio samples\n";
    oss << "    for (int i = 0; i < num_samples; ++i) {\n";
    oss << "        // Calculate time in seconds\n";
    oss << "        double t = static_cast<double>(i) / sample_rate;\n\n";

    oss << "        // Calculate oscillator phase for 440Hz\n";
    oss << "        double osc_phase = 2.0 * M_PI * " << graph.osc.frequency_hz << " * t;\n";
    oss << "        float oscillator_output = sinf(static_cast<float>(osc_phase));\n\n";

    oss << "        // Calculate pan LFO phase (0.25 Hz)\n";
    oss << "        double pan_phase = 2.0 * M_PI * " << graph.pan_lfo.rate_hz << " * t;\n";
    oss << "        double pan = 0.5 * (1.0 + sin(pan_phase)); // 0..1 range\n";
    oss << "        float gainL = static_cast<float>(1.0 - pan);\n";
    oss << "        float gainR = static_cast<float>(pan);\n\n";

    oss << "        // Calculate left and right sample values\n";
    oss << "        float sampleL = oscillator_output * gainL;\n";
    oss << "        float sampleR = oscillator_output * gainR;\n\n";

    oss << "        // Call the oscillator step function (if needed, you may need to adapt this)\n";
    oss << "        // This is a simplified version - in a real scenario, you would call osc.Step(state, &sampleL, &sampleR, sample_rate);\n";
    oss << "        // For now, we'll use the direct calculation above\n\n";

    oss << "        // Store samples\n";
    oss << "        left_buffer[i] = sampleL;\n";
    oss << "        right_buffer[i] = sampleR;\n";
    oss << "    }\n\n";

    oss << "    // Optionally write samples to a simple text file for inspection\n";
    oss << "    std::ofstream output_file(\"stereo_samples.txt\");\n";
    oss << "    if (output_file.is_open()) {\n";
    oss << "        for (int i = 0; i < num_samples; ++i) {\n";
    oss << "            output_file << left_buffer[i] << \",\" << right_buffer[i] << \"\\n\";\n";
    oss << "        }\n";
    oss << "        output_file.close();\n";
    oss << "    }\n\n";

    oss << "    printf(\"Generated %d samples of %g Hz oscillator with %g Hz pan LFO\\n\", \n";
    oss << "           num_samples, " << graph.osc.frequency_hz << ", " << graph.pan_lfo.rate_hz << ");\n";
    oss << "}\n\n";

    oss << "int main() {\n";
    oss << "    demo_audio();\n";
    oss << "    return 0;\n";
    oss << "}\n";

    return Result<std::string>::Success(oss.str());
}

} // namespace ProtoVMCLI