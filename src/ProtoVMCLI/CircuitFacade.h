#ifndef _ProtoVM_CircuitFacade_h_
#define _ProtoVM_CircuitFacade_h_

#include "SessionTypes.h"
#include "CircuitData.h"  // The enhanced version with IDs
#include "ProtoVM.h"      // For Upp types
#include "CollaborationTypes.h"  // For collaboration features
#include "BehavioralAnalysis.h"  // For behavioral analysis
#include <string>
#include <vector>
#include <optional>

// Forward declarations for functional analysis types
namespace ProtoVMCLI {
    struct FunctionalNodeId;
    struct FunctionalCone;
    struct DependencySummary;
}

namespace ProtoVMCLI {

// Forward declaration
struct EditOperation;

// Information about a circuit revision
struct CircuitRevisionInfo {
    int64_t revision;
};

// Exported circuit state
struct CircuitStateExport {
    int64_t revision;
    Upp::String circuit_json;  // JSON representation of the circuit
};

// Result for circuit operations
template<typename T>
struct Result;

// Forward declaration
class ISessionStore;

// Circuit facade to handle circuit operations
class CircuitFacade {
public:
    explicit CircuitFacade(std::shared_ptr<ISessionStore> session_store = nullptr)
        : session_store_(session_store) {}

    // Alternative constructor without session store - for use when session saving is handled separately
    explicit CircuitFacade() : session_store_(nullptr) {}

    // Load the current circuit state for a session:
    // - initial circuit (from .circuit or snapshot)
    // - plus all edit events up to circuit_revision.
    Result<CircuitRevisionInfo> LoadCurrentCircuit(
        const SessionMetadata& session,
        const std::string& session_dir,
        CircuitData& out_circuit
    );

    // Branch-aware version: Load circuit state for a specific branch
    Result<CircuitRevisionInfo> LoadCurrentCircuitForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        CircuitData& out_circuit
    );

    // Apply one or more editing operations to the circuit,
    // persist them as events, and bump the circuit_revision.
    Result<CircuitRevisionInfo> ApplyEditOperations(
        SessionMetadata& session,
        const std::string& session_dir,
        const std::vector<EditOperation>& ops,
        const std::string& user_id
    );

    // Branch-aware version: Apply editing operations to a specific branch
    Result<CircuitRevisionInfo> ApplyEditOperationsToBranch(
        SessionMetadata& session,
        const std::string& session_dir,
        const std::vector<EditOperation>& ops,
        const std::string& user_id,
        const std::string& branch_name
    );

    // Optional: export entire circuit state as JSON for clients.
    Result<CircuitStateExport> ExportCircuitState(
        const SessionMetadata& session,
        const std::string& session_dir
    );

    // Graph building methods
    Result<CircuitGraph> BuildGraphForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name
    );

    // Timing analysis methods
    Result<std::pair<std::vector<TimingNodeId>, std::vector<TimingEdge>>> BuildTimingGraphForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name
    );

    // Functional analysis methods
    Result<FunctionalCone> BuildBackwardConeForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

    Result<FunctionalCone> BuildForwardConeForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

    Result<DependencySummary> BuildDependencySummaryForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const FunctionalNodeId& root,
        int max_depth = 128
    );

    // Block analysis methods
    Result<BlockGraph> BuildBlockGraphForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name
    );

    // Behavioral analysis methods
    Result<BehaviorDescriptor> InferBehaviorForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<BehaviorDescriptor> InferBehaviorForNodeInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& node_id,
        const std::string& node_kind_hint
    );

    // HLS IR analysis methods
    Result<IrModule> BuildIrForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id
    );

    Result<IrModule> BuildIrForNodeRegionInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& node_id,
        const std::string& node_kind_hint,
        int max_depth = 4
    );

    // Diff analysis methods
    Result<BehaviorDiff> DiffBlockBehaviorBetweenBranches(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_before,
        const std::string& branch_after,
        const std::string& block_id
    );

    Result<IrDiff> DiffBlockIrBetweenBranches(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_before,
        const std::string& branch_after,
        const std::string& block_id
    );

    Result<IrDiff> DiffNodeRegionIrBetweenBranches(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_before,
        const std::string& branch_after,
        const std::string& node_id,
        const std::string& node_kind_hint,
        int max_depth = 4
    );

    // Transformation methods
    Result<std::vector<TransformationPlan>> ProposeTransformationsForBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        int max_plans
    );

    Result<std::vector<TransformationPlan>> ProposeTransformationsForBlockInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        int max_plans
    );

    Result<void> ApplyTransformationPlan(
        SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const TransformationPlan& plan,
        const std::string& user_id
    );

    // IR optimization methods
    Result<IrOptimizationResult> OptimizeBlockIrInBranch(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const std::vector<IrOptPassKind>& passes_to_run
    );

    // Generate transformation plans from IR optimization for a block.
    Result<std::vector<TransformationPlan>> ProposeIrBasedTransformationsForBlock(
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        const std::string& block_id,
        const std::vector<IrOptPassKind>& passes_to_run
    );

private:
    // Internal helper to load circuit from initial file
    Result<bool> LoadInitialCircuit(const std::string& circuit_file_path, CircuitData& out_circuit);

    // Internal helper to apply an edit operation to a circuit
    Result<bool> ApplyEditOperation(CircuitData& circuit, const EditOperation& op);

    // Internal helper to replay circuit events and update the circuit
    Result<bool> ReplayCircuitEvents(CircuitData& circuit, const std::string& session_dir, int64_t from_revision, int64_t to_revision);

    // Internal helper to replay circuit events for a specific branch
    Result<bool> ReplayCircuitEventsForBranch(CircuitData& circuit, const std::string& session_dir, int64_t from_revision, int64_t to_revision, const std::string& branch_name);

    // Internal helper to get the latest circuit snapshot revision
    int64_t GetLatestCircuitSnapshotRevision(const std::string& session_dir);

    // Internal helper to load circuit from a snapshot
    Result<bool> LoadCircuitFromSnapshot(const std::string& session_dir, CircuitData& out_circuit);

    // Internal helper to save circuit snapshot
    Result<bool> SaveCircuitSnapshot(const CircuitData& circuit, const std::string& session_dir, int64_t revision);

private:
    std::shared_ptr<ISessionStore> session_store_;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CircuitFacade_h_