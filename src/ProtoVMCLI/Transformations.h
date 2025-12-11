#ifndef PROTOVM_TRANSFORMATIONS_H
#define PROTOVM_TRANSFORMATIONS_H

#include "SessionTypes.h"  // For Result<T>, SessionMetadata
#include "CircuitOps.h"    // For EditOperation
#include "CircuitGraph.h"  // For CircuitGraph
#include "PipelineModel.h" // For PipelineMap
#include "TimingAnalysis.h" // For TimingAnalysis
#include <vector>
#include <string>

// Forward declarations that may still be needed
class BlockGraph;
class Circuit;
class BlockInstance;

enum class TransformationKind {
    Unknown,
    SimplifyDoubleInversion,
    SimplifyRedundantGate,
    ReplaceWithKnownBlock,
    RewireFanoutTree,
    MergeEquivalentBlocks,
};

enum class PreservationLevel {
    BehaviorKindPreserved,
    IOContractPreserved,
    DependencyPatternPreserved
};

struct TransformationTarget {
    std::string subject_id;          // e.g. block ID, component ID, or region identifier
    std::string subject_kind;        // "Block", "Component", "Region"
};

struct TransformationStep {
    std::string description;         // human-readable
    // You may include references to components/pins/nets as needed.
};

struct TransformationPlan {
    std::string id;                              // unique transformation ID (per proposal)
    TransformationKind kind;
    TransformationTarget target;
    std::vector<PreservationLevel> guarantees;   // what we assert is preserved
    std::vector<TransformationStep> steps;       // high-level steps
    // Optional: pre- and post- BehaviorDescriptor snapshots (for verification)
};

class TransformationEngine {
public:
    // Discover transformation opportunities for a given branch.
    ProtoVMCLI::Result<std::vector<TransformationPlan>> ProposeTransformationsForBranch(
        const ProtoVMCLI::SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        int max_plans
    );

    // Propose transformations for a specific block.
    ProtoVMCLI::Result<std::vector<TransformationPlan>> ProposeTransformationsForBlock(
        const ProtoVMCLI::SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        int max_plans
    );

    // Convert a TransformationPlan into a sequence of CircuitOps edit operations.
    ProtoVMCLI::Result<std::vector<ProtoVMCLI::EditOperation>> MaterializePlan(
        const TransformationPlan& plan
    );

    // Verify that a transformation plan preserves behavior
    ProtoVMCLI::Result<bool> VerifyBehaviorPreserved(
        const ProtoVMCLI::SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const TransformationPlan& plan
    );

private:
    // Internal helper methods
    std::vector<TransformationPlan> FindDoubleInversionPatterns(const ProtoVMCLI::CircuitGraph& graph, int max_plans);
    std::vector<TransformationPlan> FindRedundantGatePatterns(const ProtoVMCLI::CircuitGraph& graph, int max_plans);
    std::vector<TransformationPlan> FindKnownBlockReplacementPatterns(
        const Circuit& circuit,
        const BlockGraph& block_graph,
        int max_plans
    );
    std::vector<TransformationPlan> FindDoubleInversionPatternsInBlock(
        const ProtoVMCLI::CircuitGraph& graph,
        const std::string& block_id,
        int max_plans
    );
    std::vector<TransformationPlan> FindRedundantGatePatternsInBlock(
        const ProtoVMCLI::CircuitGraph& graph,
        const std::string& block_id,
        int max_plans
    );

    // Materialization methods
    std::vector<ProtoVMCLI::EditOperation> MaterializeDoubleInversionSimplification(const TransformationPlan& plan);
    std::vector<ProtoVMCLI::EditOperation> MaterializeRedundantGateSimplification(const TransformationPlan& plan);
    std::vector<ProtoVMCLI::EditOperation> MaterializeKnownBlockReplacement(const TransformationPlan& plan);
    std::vector<ProtoVMCLI::EditOperation> MaterializeEquivalentBlockMerge(const TransformationPlan& plan);
    std::vector<ProtoVMCLI::EditOperation> MaterializeFanoutRewiring(const TransformationPlan& plan);

    // Helper for applying edit operations
    ProtoVMCLI::Result<void> ApplyEditOperation(Circuit& circuit, const ProtoVMCLI::EditOperation& op);

    // Helper for analyzing block structure
    struct BlockAnalysisResult {
        bool matches_adder = false;
        bool matches_mux = false;
        // Add more patterns as needed
    };
    BlockAnalysisResult AnalyzeBlockStructure(const Circuit& circuit, const BlockInstance& block);

    static int transformation_id_counter;
};

#endif // PROTOVM_TRANSFORMATIONS_H