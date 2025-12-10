#ifndef _ProtoVM_CodegenIrInference_h_
#define _ProtoVM_CodegenIrInference_h_

#include "CodegenIr.h"
#include "SessionTypes.h"
#include "HlsIr.h"
#include "BehavioralAnalysis.h"
#include "StructuralSynthesis.h"  // For oscillator detection
#include <string>
#include <vector>

namespace ProtoVMCLI {

class CodegenIrInference {
public:
    // Build a CodegenModule from a block in a given branch.
    static Result<CodegenModule> BuildCodegenModuleForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    // Optionally: build for a node-region (subset of a block).
    static Result<CodegenModule> BuildCodegenModuleForNodeRegionInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const std::vector<std::string>& node_ids
    );

private:
    // Helper functions
    static Result<CodegenModule> BuildFromIrModuleAndBehavior(
        const IrModule& ir_module,
        const BehaviorDescriptor& behavior
    );
    
    // Convert HlsIr value to CodegenValue
    static CodegenValue ConvertIrValueToCodegenValue(const IrValue& ir_value, CodegenStorageKind storage);
    
    // Convert HlsIr expression to CodegenExpression
    static CodegenExpr ConvertIrExprToCodegenExpr(const IrExpr& ir_expr);
    
    // Check if behavior indicates oscillator-like behavior
    static bool IsOscillatorLike(const BehaviorDescriptor& behavior);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CodegenIrInference_h_