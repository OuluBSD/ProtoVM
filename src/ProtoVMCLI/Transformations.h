#ifndef PROTOVM_TRANSFORMATIONS_H
#define PROTOVM_TRANSFORMATIONS_H

#include <Core/Vector.h>
#include <Core/String.h>
#include <Core/Result.h>
#include "SessionTypes.h"
#include "CircuitOps.h"

// Forward declarations
class CircuitGraph;
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
    String subject_id;          // e.g. block ID, component ID, or region identifier
    String subject_kind;        // "Block", "Component", "Region"
};

struct TransformationStep {
    String description;         // human-readable
    // You may include references to components/pins/nets as needed.
};

struct TransformationPlan {
    String id;                              // unique transformation ID (per proposal)
    TransformationKind kind;
    TransformationTarget target;
    Vector<PreservationLevel> guarantees;   // what we assert is preserved
    Vector<TransformationStep> steps;       // high-level steps
    // Optional: pre- and post- BehaviorDescriptor snapshots (for verification)
};

class TransformationEngine {
public:
    // Discover transformation opportunities for a given branch.
    Result<Vector<TransformationPlan>> ProposeTransformationsForBranch(
        const SessionMetadata& session,
        const String& session_dir,
        const String& branch_name,
        int max_plans
    );

    // Propose transformations for a specific block.
    Result<Vector<TransformationPlan>> ProposeTransformationsForBlock(
        const SessionMetadata& session,
        const String& session_dir,
        const String& branch_name,
        const String& block_id,
        int max_plans
    );

    // Convert a TransformationPlan into a sequence of CircuitOps edit operations.
    Result<Vector<EditOperation>> MaterializePlan(
        const TransformationPlan& plan
    );

    // Verify that a transformation plan preserves behavior
    Result<bool> VerifyBehaviorPreserved(
        const SessionMetadata& session,
        const String& session_dir,
        const String& branch_name,
        const TransformationPlan& plan
    );

private:
    // Internal helper methods
    Vector<TransformationPlan> FindDoubleInversionPatterns(const CircuitGraph& graph, int max_plans);
    Vector<TransformationPlan> FindRedundantGatePatterns(const CircuitGraph& graph, int max_plans);
    Vector<TransformationPlan> FindKnownBlockReplacementPatterns(
        const Circuit& circuit,
        const BlockGraph& block_graph,
        int max_plans
    );
    Vector<TransformationPlan> FindDoubleInversionPatternsInBlock(
        const CircuitGraph& graph,
        const String& block_id,
        int max_plans
    );
    Vector<TransformationPlan> FindRedundantGatePatternsInBlock(
        const CircuitGraph& graph,
        const String& block_id,
        int max_plans
    );

    // Materialization methods
    Vector<EditOperation> MaterializeDoubleInversionSimplification(const TransformationPlan& plan);
    Vector<EditOperation> MaterializeRedundantGateSimplification(const TransformationPlan& plan);
    Vector<EditOperation> MaterializeKnownBlockReplacement(const TransformationPlan& plan);
    Vector<EditOperation> MaterializeEquivalentBlockMerge(const TransformationPlan& plan);
    Vector<EditOperation> MaterializeFanoutRewiring(const TransformationPlan& plan);

    // Helper for applying edit operations
    Result<void> ApplyEditOperation(Circuit& circuit, const EditOperation& op);

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