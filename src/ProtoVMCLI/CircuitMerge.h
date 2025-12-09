#ifndef _ProtoVM_CircuitMerge_h_
#define _ProtoVM_CircuitMerge_h_

#include "CircuitData.h"
#include "CircuitOps.h"
#include "CollaborationTypes.h"
#include "EventLogger.h"
#include <vector>
#include <string>

namespace ProtoVMCLI {

class CircuitMerge {
public:
    static MergeResult ResolveConcurrentEdits(
        const CircuitData& base_circuit,
        const std::vector<EditOperation>& new_ops,
        int64_t client_rev,
        int64_t server_rev,
        const std::vector<EventLogEntry>& intervening_events
    );

    // Branch-aware merge function
    static MergeResult MergeBranches(
        const CircuitData& source_circuit,
        const CircuitData& target_circuit,
        const CircuitData& base_circuit,  // Common ancestor
        const std::vector<EditOperation>& source_ops,    // Changes from base to source
        const std::vector<EditOperation>& target_ops     // Changes from base to target
    );

private:
    // Helper methods for conflict detection and resolution
    static MergeResult ResolveAddComponent(const EditOperation& op, const CircuitData& current_circuit);
    static MergeResult ResolveRemoveComponent(const EditOperation& op, const CircuitData& current_circuit);
    static MergeResult ResolveMoveComponent(const EditOperation& op, const CircuitData& current_circuit);
    static MergeResult ResolveSetComponentProperty(const EditOperation& op, const CircuitData& current_circuit);
    static MergeResult ResolveConnect(const EditOperation& op, const CircuitData& current_circuit);
    static MergeResult ResolveDisconnect(const EditOperation& op, const CircuitData& current_circuit);

    // Helper to check if an entity still exists in the circuit
    static bool EntityExists(const CircuitData& circuit, const CircuitEntityId& id);
    static bool WireExists(const CircuitData& circuit, const CircuitEntityId& id);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CircuitMerge_h_