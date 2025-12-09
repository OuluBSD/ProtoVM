#include "CircuitMerge.h"
#include <algorithm>

namespace ProtoVMCLI {

MergeResult CircuitMerge::ResolveConcurrentEdits(
    const CircuitData& base_circuit,
    const std::vector<EditOperation>& new_ops,
    int64_t client_rev,
    int64_t server_rev,
    const std::vector<EventLogEntry>& intervening_events
) {
    // If client and server revisions are the same, no conflict exists
    if (client_rev == server_rev) {
        MergeResult result;
        result.merged = true;
        result.conflict = false;
        result.conflict_reason = "";
        result.transformed_ops = new_ops;
        return result;
    }

    // Start with the original operations
    std::vector<EditOperation> transformed_ops = new_ops;
    CircuitData current_circuit = base_circuit;

    // Apply all intervening events to get to the current server state
    // This would involve replaying all events between client_rev and server_rev
    // For now, we'll implement basic transformation logic

    MergeResult result;
    result.merged = true;
    result.conflict = false;
    result.conflict_reason = "";
    result.transformed_ops = transformed_ops;

    // For each new operation, check if it conflicts with the current state
    for (auto& op : transformed_ops) {
        MergeResult op_result;
        
        switch (op.type) {
            case EditOpType::AddComponent:
                op_result = ResolveAddComponent(op, current_circuit);
                break;
            case EditOpType::RemoveComponent:
                op_result = ResolveRemoveComponent(op, current_circuit);
                break;
            case EditOpType::MoveComponent:
                op_result = ResolveMoveComponent(op, current_circuit);
                break;
            case EditOpType::SetComponentProperty:
                op_result = ResolveSetComponentProperty(op, current_circuit);
                break;
            case EditOpType::Connect:
                op_result = ResolveConnect(op, current_circuit);
                break;
            case EditOpType::Disconnect:
                op_result = ResolveDisconnect(op, current_circuit);
                break;
            default:
                op_result.merged = true;
                op_result.conflict = false;
                op_result.conflict_reason = "";
                op_result.transformed_ops.push_back(op);
                break;
        }

        if (op_result.conflict) {
            result.conflict = true;
            result.conflict_reason = op_result.conflict_reason;
            result.merged = false;
            return result; // Return immediately on first conflict
        }

        // Update the circuit with this operation if it was transformed
        if (op_result.merged && !op_result.transformed_ops.empty()) {
            // Apply the operation to current_circuit for subsequent conflict checking
            // This is a simplified approach - full replay would be more accurate
        }
    }

    return result;
}

MergeResult CircuitMerge::ResolveAddComponent(const EditOperation& op, const CircuitData& current_circuit) {
    // Check if the component ID already exists
    if (EntityExists(current_circuit, op.component_id)) {
        // Generate a new derived ID for the component
        CircuitEntityId new_id = CircuitEntityId(op.component_id.id + "_user_" + op.component_id.id + "_1");
        
        // Create a transformed operation with the new ID
        EditOperation transformed_op = op;
        transformed_op.component_id = new_id;
        
        MergeResult result;
        result.merged = true;
        result.conflict = false;
        result.conflict_reason = "Component ID collision resolved with new ID: " + new_id.id;
        result.transformed_ops = {transformed_op};
        return result;
    }

    // If the ID doesn't exist, operation is safe to apply
    MergeResult result;
    result.merged = true;
    result.conflict = false;
    result.conflict_reason = "";
    result.transformed_ops = {op};
    return result;
}

MergeResult CircuitMerge::ResolveRemoveComponent(const EditOperation& op, const CircuitData& current_circuit) {
    // If the component was already removed, the operation is a no-op but not a conflict
    if (!EntityExists(current_circuit, op.component_id)) {
        // If the component doesn't exist, this operation becomes a no-op
        MergeResult result;
        result.merged = true;  // Considered merged (as no-op)
        result.conflict = false;
        result.conflict_reason = "Component already removed";
        result.transformed_ops = {};  // No operations to apply
        return result;
    }

    // If the component exists, operation is safe to apply
    MergeResult result;
    result.merged = true;
    result.conflict = false;
    result.conflict_reason = "";
    result.transformed_ops = {op};
    return result;
}

MergeResult CircuitMerge::ResolveMoveComponent(const EditOperation& op, const CircuitData& current_circuit) {
    // If the component was already removed, this is a conflict
    if (!EntityExists(current_circuit, op.component_id)) {
        MergeResult result;
        result.merged = false;
        result.conflict = true;
        result.conflict_reason = "Component no longer exists to be moved";
        result.transformed_ops = {};
        return result;
    }

    // If the component exists, operation is safe to apply (last-writer-wins)
    MergeResult result;
    result.merged = true;
    result.conflict = false;
    result.conflict_reason = "";
    result.transformed_ops = {op};
    return result;
}

MergeResult CircuitMerge::ResolveSetComponentProperty(const EditOperation& op, const CircuitData& current_circuit) {
    // If the component was already removed, this is a conflict
    if (!EntityExists(current_circuit, op.component_id)) {
        MergeResult result;
        result.merged = false;
        result.conflict = true;
        result.conflict_reason = "Component no longer exists for property update";
        result.transformed_ops = {};
        return result;
    }

    // If the component exists, operation is safe to apply (last-writer-wins)
    MergeResult result;
    result.merged = true;
    result.conflict = false;
    result.conflict_reason = "";
    result.transformed_ops = {op};
    return result;
}

MergeResult CircuitMerge::ResolveConnect(const EditOperation& op, const CircuitData& current_circuit) {
    // Check if the components involved in the connection still exist
    if (!EntityExists(current_circuit, op.component_id) || 
        !EntityExists(current_circuit, op.target_component_id)) {
        MergeResult result;
        result.merged = false;
        result.conflict = true;
        result.conflict_reason = "One or more components for connection no longer exist";
        result.transformed_ops = {};
        return result;
    }

    // Check if the connection already exists
    // For simplicity, we won't check this here, but in a real implementation
    // we would check if the connection already exists

    MergeResult result;
    result.merged = true;
    result.conflict = false;
    result.conflict_reason = "";
    result.transformed_ops = {op};
    return result;
}

MergeResult CircuitMerge::ResolveDisconnect(const EditOperation& op, const CircuitData& current_circuit) {
    // If the components involved in the disconnection no longer exist, 
    // this is not necessarily a conflict - we can just ignore the operation
    if (!EntityExists(current_circuit, op.component_id) || 
        !EntityExists(current_circuit, op.target_component_id)) {
        MergeResult result;
        result.merged = true;  // No-op but not a conflict
        result.conflict = false;
        result.conflict_reason = "One or more components for disconnection no longer exist";
        result.transformed_ops = {};  // No operations to apply
        return result;
    }

    // Check if the wire to disconnect still exists
    // For now, we'll assume the operation is safe to apply
    MergeResult result;
    result.merged = true;
    result.conflict = false;
    result.conflict_reason = "";
    result.transformed_ops = {op};
    return result;
}

bool CircuitMerge::EntityExists(const CircuitData& circuit, const CircuitEntityId& id) {
    auto it = std::find_if(circuit.components.begin(), circuit.components.end(),
        [&id](const ComponentData& comp) {
            return comp.id == id;
        });
    return it != circuit.components.end();
}

bool CircuitMerge::WireExists(const CircuitData& circuit, const CircuitEntityId& id) {
    auto it = std::find_if(circuit.wires.begin(), circuit.wires.end(),
        [&id](const WireData& wire) {
            return wire.id == id;
        });
    return it != circuit.wires.end();
}

MergeResult CircuitMerge::MergeBranches(
    const CircuitData& source_circuit,
    const CircuitData& target_circuit,
    const CircuitData& base_circuit,
    const std::vector<EditOperation>& source_ops,
    const std::vector<EditOperation>& target_ops
) {
    // Start with the target circuit
    CircuitData result_circuit = target_circuit;

    // Apply source operations to the target circuit with three-way merge logic
    std::vector<EditOperation> merged_ops;

    // For a proper three-way merge, we need to:
    // 1. Determine which operations from the source are not present in the target
    // 2. Apply those operations to the target circuit
    // 3. Handle any conflicts that arise during the application

    // For now, implement a simplified version that applies all source operations
    // In a real implementation, we'd need more sophisticated logic to detect
    // which operations have already been applied to both branches

    MergeResult result;
    result.merged = true;
    result.conflict = false;
    result.conflict_reason = "";

    // Apply each source operation to the target circuit
    for (const auto& source_op : source_ops) {
        // Check if this operation conflicts with the current target state
        MergeResult op_result = ResolveConcurrentEdits(
            result_circuit,  // Current state of target
            {source_op},     // Operation from source
            -1,              // Placeholder for client revision
            -1,              // Placeholder for server revision
            {}               // Placeholder for intervening events
        );

        if (op_result.conflict) {
            result.conflict = true;
            result.merged = false;
            result.conflict_reason = op_result.conflict_reason;
            return result;
        }

        // If the operation was successfully transformed, add it to the merged ops
        merged_ops.insert(merged_ops.end(),
                         op_result.transformed_ops.begin(),
                         op_result.transformed_ops.end());
    }

    // Update the result with the merged operations
    result.transformed_ops = merged_ops;

    return result;
}

} // namespace ProtoVMCLI