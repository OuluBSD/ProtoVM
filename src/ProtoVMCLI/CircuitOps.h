#ifndef _ProtoVM_CircuitOps_h_
#define _ProtoVM_CircuitOps_h_

#include "CircuitData.h"  // The enhanced version with IDs
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class EditOpType {
    AddComponent,
    RemoveComponent,
    MoveComponent,
    SetComponentProperty,
    Connect,
    Disconnect
};

// Structure representing a circuit editing operation
struct EditOperation {
    EditOpType type;
    int64_t revision_base;  // optional: expected current revision, for optimistic concurrency
    CircuitEntityId component_id;  // ID of the component involved in the operation
    CircuitEntityId wire_id;       // ID of the wire involved in the operation
    // Position / geometry:
    int x = 0, y = 0;
    // Properties:
    std::string property_name;
    std::string property_value;
    // Connection endpoints (for Connect/Disconnect):
    CircuitEntityId target_component_id;  // For connecting to another component
    std::string pin_name;                 // Pin name on the source component
    std::string target_pin_name;          // Pin name on the target component
    // Component type and name for add operations:
    std::string component_type;
    std::string component_name;
    // Additional properties for the component
    std::vector<std::pair<std::string, std::string>> properties;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CircuitOps_h_