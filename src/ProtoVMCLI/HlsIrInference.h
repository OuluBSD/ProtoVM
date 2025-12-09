#ifndef _ProtoVM_HlsIrInference_h_
#define _ProtoVM_HlsIrInference_h_

#include "HlsIr.h"
#include "CircuitGraph.h"
#include "BlockAnalysis.h"
#include "BehavioralAnalysis.h"
#include "FunctionalAnalysis.h"
#include "SessionTypes.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace ProtoVMCLI {

class HlsIrInference {
public:
    // Infer IR for a single block.
    Result<IrModule> InferIrForBlock(
        const BlockInstance& block,
        const CircuitGraph& graph,
        const BehaviorDescriptor& behavior
    );

    // Infer IR for a small region around a node (optional but recommended).
    Result<IrModule> InferIrForNodeRegion(
        const CircuitGraph& graph,
        const std::string& node_id,
        const std::string& node_kind_hint, // "Pin", "Component", "Net" or empty
        const FunctionalAnalysis& func,
        const BehavioralAnalysis& beh,
        int max_depth = 4
    );

private:
    // Helper methods for expression creation
    IrExpr CreateBinaryOp(IrExprKind kind, const IrValue& target, const IrValue& a, const IrValue& b);
    IrExpr CreateUnaryOp(IrExprKind kind, const IrValue& target, const IrValue& a);
    IrExpr CreateTernaryOp(IrExprKind kind, const IrValue& target, const IrValue& sel, const IrValue& a, const IrValue& b);

    // Helper for value lookup
    IrValue FindValueByName(const std::vector<IrValue>& values, const std::string& name);
    
    // Helper to infer expression from block behavior
    std::vector<IrExpr> InferExpressionsFromBlockBehavior(const BlockInstance& block, const BehaviorDescriptor& behavior);
    
    // Helper to map block ports to IR values
    std::vector<IrValue> MapBlockPortsToIrValues(const std::vector<BlockPort>& block_ports);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_HlsIrInference_h_