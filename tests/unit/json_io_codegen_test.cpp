#include "ProtoVMCLI/CodegenIr.h"
#include "ProtoVMCLI/JsonIO.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace ProtoVMCLI {

TEST(JsonIOSerializationTest, CodegenValueSerialization) {
    CodegenValue value("test_var", "int32_t", 32, CodegenStorageKind::Local, true, 10);
    
    auto value_map = JsonIO::CodegenValueToValueMap(value);
    
    EXPECT_EQ(value_map.Get("name", Upp::Value()).ToString().ToStd(), "test_var");
    EXPECT_EQ(value_map.Get("c_type", Upp::Value()).ToString().ToStd(), "int32_t");
    EXPECT_EQ(value_map.Get("bit_width", Upp::Value(-1)).ToInt(), 32);
    EXPECT_EQ(value_map.Get("storage", Upp::Value()).ToString().ToStd(), "Local");
    EXPECT_EQ(value_map.Get("is_array", Upp::Value(false)).ToBool(), true);
    EXPECT_EQ(value_map.Get("array_length", Upp::Value(-1)).ToInt(), 10);
}

TEST(JsonIOSerializationTest, CodegenExprSerialization) {
    std::vector<CodegenValue> args = {
        CodegenValue("a", "int32_t", 32, CodegenStorageKind::Input),
        CodegenValue("b", "int32_t", 32, CodegenStorageKind::Input)
    };
    CodegenExpr expr(CodegenExprKind::BinaryOp, "+", args, "0");
    
    auto expr_map = JsonIO::CodegenExprToValueMap(expr);
    
    EXPECT_EQ(expr_map.Get("kind", Upp::Value()).ToString().ToStd(), "BinaryOp");
    EXPECT_EQ(expr_map.Get("op", Upp::Value()).ToString().ToStd(), "+");
    EXPECT_EQ(expr_map.Get("literal", Upp::Value()).ToString().ToStd(), "0");
    
    auto args_array = expr_map.Get("args", Upp::ValueArray());
    EXPECT_EQ(args_array.GetCount(), 2);
}

TEST(JsonIOSerializationTest, CodegenModuleSerialization) {
    CodegenModule module("test_module", "test_block");
    module.is_oscillator_like = true;
    module.behavior_summary = "Test oscillator";
    
    // Add some inputs
    module.inputs.push_back(CodegenValue("in_freq", "float", 32, CodegenStorageKind::Input));
    
    // Add some outputs
    module.outputs.push_back(CodegenValue("out_sample", "float", 32, CodegenStorageKind::Output));
    
    // Add some state
    module.state.push_back(CodegenValue("phase", "float", 32, CodegenStorageKind::State));
    
    // Add a simple assignment
    std::vector<CodegenValue> args = {module.inputs[0]};
    CodegenExpr expr(CodegenExprKind::Value, "", args, "");
    CodegenAssignment assignment(module.outputs[0], expr);
    module.comb_assigns.push_back(assignment);
    
    auto module_map = JsonIO::CodegenModuleToValueMap(module);
    
    EXPECT_EQ(module_map.Get("id", Upp::Value()).ToString().ToStd(), "test_module");
    EXPECT_EQ(module_map.Get("block_id", Upp::Value()).ToString().ToStd(), "test_block");
    EXPECT_EQ(module_map.Get("is_oscillator_like", Upp::Value(false)).ToBool(), true);
    EXPECT_EQ(module_map.Get("behavior_summary", Upp::Value()).ToString().ToStd(), "Test oscillator");
    
    auto inputs_array = module_map.Get("inputs", Upp::ValueArray());
    EXPECT_EQ(inputs_array.GetCount(), 1);
    
    auto outputs_array = module_map.Get("outputs", Upp::ValueArray());
    EXPECT_EQ(outputs_array.GetCount(), 1);
    
    auto state_array = module_map.Get("state", Upp::ValueArray());
    EXPECT_EQ(state_array.GetCount(), 1);
    
    auto assigns_array = module_map.Get("comb_assigns", Upp::ValueArray());
    EXPECT_EQ(assigns_array.GetCount(), 1);
}

TEST(JsonIOSerializationTest, EnumConversions) {
    // Test CodegenTargetLanguage conversion
    auto c_lang = JsonIO::CodegenTargetLanguageToJson(CodegenTargetLanguage::C);
    EXPECT_EQ(c_lang.ToString().ToStd(), "C");
    
    auto cpp_lang = JsonIO::CodegenTargetLanguageToJson(CodegenTargetLanguage::Cpp);
    EXPECT_EQ(cpp_lang.ToString().ToStd(), "Cpp");
    
    // Test CodegenExprKind conversion
    auto binary_kind = JsonIO::CodegenExprKindToJson(CodegenExprKind::BinaryOp);
    EXPECT_EQ(binary_kind.ToString().ToStd(), "BinaryOp");
    
    auto ternary_kind = JsonIO::CodegenExprKindToJson(CodegenExprKind::TernaryOp);
    EXPECT_EQ(ternary_kind.ToString().ToStd(), "TernaryOp");
    
    // Test CodegenStorageKind conversion
    auto input_kind = JsonIO::CodegenStorageKindToJson(CodegenStorageKind::Input);
    EXPECT_EQ(input_kind.ToString().ToStd(), "Input");
    
    auto state_kind = JsonIO::CodegenStorageKindToJson(CodegenStorageKind::State);
    EXPECT_EQ(state_kind.ToString().ToStd(), "State");
}

} // namespace ProtoVMCLI