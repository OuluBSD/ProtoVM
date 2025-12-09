#ifndef _ProtoVM_BranchTypes_h_
#define _ProtoVM_BranchTypes_h_

#include <string>
#include <vector>

namespace ProtoVMCLI {

// Metadata for a single branch
struct BranchMetadata {
    std::string name;             // e.g. "main", "experiment-alu"
    int64_t head_revision;        // latest circuit_revision on this branch
    int64_t sim_revision;         // latest sim_revision for this branch (if any)
    int64_t base_revision;        // revision from which this branch originally forked
    bool is_default;              // true for the default branch (e.g. "main")
    
    BranchMetadata() : head_revision(0), sim_revision(0), base_revision(0), is_default(false) {}
    BranchMetadata(const std::string& n, int64_t head_rev, int64_t sim_rev, int64_t base_rev, bool def)
        : name(n), head_revision(head_rev), sim_revision(sim_rev), base_revision(base_rev), is_default(def) {}
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_BranchTypes_h_