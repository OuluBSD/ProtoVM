#ifndef _ProtoVM_CollaborationTypes_h_
#define _ProtoVM_CollaborationTypes_h_

#include "CircuitOps.h"
#include "ProtoVM.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

struct MergeResult {
    bool merged;               // true if auto-merge was applied
    bool conflict;             // true if merge failed
    std::string conflict_reason;    // detailed reason
    std::vector<EditOperation> transformed_ops; // final ops to apply
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CollaborationTypes_h_