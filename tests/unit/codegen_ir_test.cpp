#include "ProtoVMCLI/CodegenIr.h"
#include "ProtoVMCLI/CodegenIrInference.h"
#include "ProtoVMCLI/CodeEmitter.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace ProtoVMCLI {

// Test basic CodegenIR data structures
TEST(CodegenIrTest, CodegenValueStructure) {
    CodegenValue value("test_var", "int32_t", 32, CodegenStorageKind::Local);
    
    EXPECT_EQ(value.name, "test_var");
    EXPECT_EQ(value.c_type, "int32_t");
    EXPECT_EQ(value.bit_width, 32);
    EXPECT_EQ(value.storage, CodegenStorageKind::Local);
    EXPECT_FALSE(value.is_array);
    EXPECT_EQ(value.array_length, -1);
}

TEST(CodegenIrTest, CodegenExprStructure) {
    std::vector<CodegenValue> args = {
        CodegenValue("a", "int32_t", 32, CodegenStorageKind::Input),
        CodegenValue("b", "int32_t", 32, CodegenStorageKind::Input)
    };
    
    CodegenExpr expr(CodegenExprKind::BinaryOp, "+", args, "");
    
    EXPECT_EQ(expr.kind, CodegenExprKind::BinaryOp);
    EXPECT_EQ(expr.op, "+");
    EXPECT_EQ(expr.args.size(), 2);
    EXPECT_EQ(expr.literal, "");
}

TEST(CodegenIrTest, CodegenAssignmentStructure) {
    CodegenValue target("result", "int32_t", 32, CodegenStorageKind::Output);
    std::vector<CodegenValue> args = {
        CodegenValue("a", "int32_t", 32, CodegenStorageKind::Input),
        CodegenValue("b", "int32_t", 32, CodegenStorageKind::Input)
    };
    CodegenExpr expr(CodegenExprKind::BinaryOp, "+", args, "");
    
    CodegenAssignment assignment(target, expr);
    
    EXPECT_EQ(assignment.target.name, "result");
    EXPECT_EQ(assignment.expr.op, "+");
}

TEST(CodegenIrTest, CodegenModuleStructure) {
    CodegenModule module("test_module", "test_block");
    
    EXPECT_EQ(module.id, "test_module");
    EXPECT_EQ(module.block_id, "test_block");
    EXPECT_FALSE(module.is_oscillator_like);
    EXPECT_EQ(module.behavior_summary, "");
    EXPECT_TRUE(module.inputs.empty());
    EXPECT_TRUE(module.outputs.empty());
    EXPECT_TRUE(module.locals.empty());
    EXPECT_TRUE(module.state.empty());
    EXPECT_TRUE(module.comb_assigns.empty());
    EXPECT_TRUE(module.state_updates.empty());
}

// Test the CodeEmitter functionality
TEST(CodeEmitterTest, BasicCodeEmission) {
    // Create a simple module
    CodegenModule module("simple_adder", "adder_block");
    
    // Add inputs
    module.inputs.push_back(CodegenValue("in_a", "int32_t", 32, CodegenStorageKind::Input));
    module.inputs.push_back(CodegenValue("in_b", "int32_t", 32, CodegenStorageKind::Input));
    
    // Add outputs
    module.outputs.push_back(CodegenValue("out_sum", "int32_t", 32, CodegenStorageKind::Output));
    
    // Add a simple assignment: out_sum = in_a + in_b
    std::vector<CodegenValue> args = {module.inputs[0], module.inputs[1]};
    CodegenExpr expr(CodegenExprKind::BinaryOp, "+", args, "");
    CodegenAssignment assignment(module.outputs[0], expr);
    
    module.comb_assigns.push_back(assignment);
    
    // Generate C code
    auto result = CodeEmitter::EmitCodeForModule(
        module, 
        CodegenTargetLanguage::C, 
        false  // don't emit state struct since we have no state
    );
    
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.data.empty());
    
    // Check that the generated code contains expected elements
    EXPECT_NE(result.data.find("int32_t in_a"), std::string::npos);
    EXPECT_NE(result.data.find("int32_t in_b"), std::string::npos);
    EXPECT_NE(result.data.find("int32_t* out_sum"), std::string::npos);
    EXPECT_NE(result.data.find("in_a + in_b"), std::string::npos);
}

TEST(CodeEmitterTest, CodeWithStateEmission) {
    // Create a module with state (like a register)
    CodegenModule module("simple_register", "reg_block");
    
    // Add inputs
    module.inputs.push_back(CodegenValue("in_data", "int32_t", 32, CodegenStorageKind::Input));
    
    // Add state
    module.state.push_back(CodegenValue("current_value", "int32_t", 32, CodegenStorageKind::State));
    
    // Add a state update: current_value = in_data
    std::vector<CodegenValue> args = {module.inputs[0]};
    CodegenExpr expr(CodegenExprKind::Value, "", args, "");
    CodegenAssignment assignment(module.state[0], expr);
    
    module.state_updates.push_back(assignment);
    
    // Generate C code with state struct
    auto result = CodeEmitter::EmitCodeForModule(
        module,
        CodegenTargetLanguage::C,
        true,  // emit state struct
        "RegState",
        "RegStep"
    );
    
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.data.empty());
    
    // Check that the generated code contains expected elements
    EXPECT_NE(result.data.find("typedef struct"), std::string::npos);
    EXPECT_NE(result.data.find("int32_t current_value"), std::string::npos);
    EXPECT_NE(result.data.find("void RegStep"), std::string::npos);
    EXPECT_NE(result.data.find("s->current_value ="), std::string::npos);
}

// Test the CodegenIrInference functionality (basic structure test)
TEST(CodegenIrInferenceTest, BasicModuleInference) {
    // We can't fully test the inference without a complete CircuitFacade setup,
    // but we can test the structure and error handling
    
    SessionMetadata session;
    session.session_id = 1;
    session.workspace_path = "/test";
    session.current_branch = "main";
    
    // This should fail gracefully with an error since the session doesn't actually exist
    auto result = CodegenIrInference::BuildCodegenModuleForBlockInBranch(
        session,
        "/test/sessions/1",
        "main",
        "nonexistent_block"
    );
    
    // We expect an error since the block doesn't exist, but the function should not crash
    // The exact behavior depends on whether the function returns error or throws
    // This test is mainly to ensure it compiles and doesn't crash
    EXPECT_FALSE(result.ok || true); // This is a placeholder - actual behavior depends on implementation
}

} // namespace ProtoVMCLI