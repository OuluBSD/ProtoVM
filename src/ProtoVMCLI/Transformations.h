#ifndef PROTOVM_TRANSFORMATIONS_H
#define PROTOVM_TRANSFORMATIONS_H

#include "SessionTypes.h"  // For Result<T>, SessionMetadata
#include "CircuitOps.h"    // For EditOperation
#include "CircuitGraph.h"  // For CircuitGraph
#include "BlockAnalysis.h" // For BlockGraph / BlockInstance / BlockKind
#include "CircuitData.h"   // For CircuitData (the ID-based circuit model)
#include "PipelineModel.h" // For PipelineMap
#include "TimingAnalysis.h" // For TimingAnalysis
#include <vector>
#include <string>

// NOTE: BlockGraph / BlockInstance live in namespace ProtoVMCLI; CircuitData
// (global scope) replaces the retired engine-side Circuit abstraction as the
// analysis subject.

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

struct TransformationTarget : public Upp::Moveable<TransformationTarget> {
    std::string subject_id;          // e.g. block ID, component ID, or region identifier
    std::string subject_kind;        // "Block", "Component", "Region"

    TransformationTarget() {}
    TransformationTarget(const TransformationTarget& other)
        : subject_id(other.subject_id), subject_kind(other.subject_kind) {}
    TransformationTarget(TransformationTarget&& other) = default;
    TransformationTarget& operator=(const TransformationTarget& other) {
        subject_id = other.subject_id;
        subject_kind = other.subject_kind;
        return *this;
    }
    TransformationTarget& operator=(TransformationTarget&& other) = default;
};

struct TransformationStep : public Upp::Moveable<TransformationStep> {
    std::string description;         // human-readable
    // You may include references to components/pins/nets as needed.

    TransformationStep() {}
    TransformationStep(const TransformationStep& other) : description(other.description) {}
    TransformationStep(TransformationStep&& other) = default;
    TransformationStep& operator=(const TransformationStep& other) {
        description = other.description;
        return *this;
    }
    TransformationStep& operator=(TransformationStep&& other) = default;
};

struct TransformationPlan : public Upp::Moveable<TransformationPlan> {
    std::string id;                              // unique transformation ID (per proposal)
    TransformationKind kind;
    TransformationTarget target;
    std::vector<PreservationLevel> guarantees;   // what we assert is preserved
    std::vector<TransformationStep> steps;       // high-level steps
    // Optional: pre- and post- BehaviorDescriptor snapshots (for verification)

    TransformationPlan() : kind(TransformationKind::Unknown) {}
    TransformationPlan(const TransformationPlan& other)
        : id(other.id), kind(other.kind), target(other.target),
          guarantees(other.guarantees), steps(other.steps) {}
    TransformationPlan(TransformationPlan&& other) = default;
    TransformationPlan& operator=(const TransformationPlan& other) {
        id = other.id;
        kind = other.kind;
        target = other.target;
        guarantees = other.guarantees;
        steps = other.steps;
        return *this;
    }
    TransformationPlan& operator=(TransformationPlan&& other) = default;
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
    std::vector<TransformationPlan> FindDoubleInversionPatterns(
        const ProtoVMCLI::CircuitGraph& graph,
        const CircuitData& circuit,
        int max_plans
    );
    std::vector<TransformationPlan> FindRedundantGatePatterns(
        const ProtoVMCLI::CircuitGraph& graph,
        const CircuitData& circuit,
        int max_plans
    );
    std::vector<TransformationPlan> FindKnownBlockReplacementPatterns(
        const CircuitData& circuit,
        const ProtoVMCLI::BlockGraph& block_graph,
        int max_plans
    );
    std::vector<TransformationPlan> FindDoubleInversionPatternsInBlock(
        const ProtoVMCLI::CircuitGraph& graph,
        const CircuitData& circuit,
        const ProtoVMCLI::BlockGraph& block_graph,
        const std::string& block_id,
        int max_plans
    );
    std::vector<TransformationPlan> FindRedundantGatePatternsInBlock(
        const ProtoVMCLI::CircuitGraph& graph,
        const CircuitData& circuit,
        const ProtoVMCLI::BlockGraph& block_graph,
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
    ProtoVMCLI::Result<void> ApplyEditOperation(CircuitData& circuit, const ProtoVMCLI::EditOperation& op);

    // Helper for analyzing block structure
    struct BlockAnalysisResult {
        bool matches_adder = false;
        bool matches_mux = false;
        // Add more patterns as needed
    };
    BlockAnalysisResult AnalyzeBlockStructure(const CircuitData& circuit, const ProtoVMCLI::BlockInstance& block);

    static int transformation_id_counter;
};

#endif // PROTOVM_TRANSFORMATIONS_H
