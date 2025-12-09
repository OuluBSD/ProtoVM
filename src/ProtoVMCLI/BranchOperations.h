#ifndef _ProtoVM_BranchOperations_h_
#define _ProtoVM_BranchOperations_h_

#include "SessionTypes.h"
#include "BranchTypes.h"
#include "CircuitFacade.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

struct BranchListResult {
    int session_id = -1;
    std::vector<BranchMetadata> branches;
    std::string current_branch;
};

struct BranchCreateResult {
    int session_id = -1;
    BranchMetadata branch;
};

struct BranchSwitchResult {
    int session_id = -1;
    std::string current_branch;
};

struct BranchDeleteResult {
    int session_id = -1;
    std::string deleted_branch;
};

struct BranchMergeResult {
    int session_id = -1;
    std::string source_branch;
    std::string target_branch;
    int64_t target_new_revision = 0;
    int merged_ops_count = 0;
};

class BranchOperations {
public:
    // List all branches in a session
    static Result<BranchListResult> ListBranches(
        const SessionMetadata& session
    );
    
    // Create a new branch
    static Result<BranchCreateResult> CreateBranch(
        SessionMetadata& session,
        const std::string& branch_name,
        const std::string& from_branch = "",
        int64_t from_revision = -1  // -1 means use head of from_branch
    );
    
    // Switch to a different branch
    static Result<BranchSwitchResult> SwitchBranch(
        SessionMetadata& session,
        const std::string& branch_name
    );
    
    // Delete a branch
    static Result<BranchDeleteResult> DeleteBranch(
        SessionMetadata& session,
        const std::string& branch_name
    );
    
    // Merge a source branch into a target branch
    static Result<BranchMergeResult> MergeBranch(
        SessionMetadata& session,
        const std::string& source_branch,
        const std::string& target_branch,
        bool allow_merge = true
    );
    
private:
    // Helper to validate branch name
    static bool IsValidBranchName(const std::string& branch_name);

    // Helper to find branch index by name
    static int FindBranchIndex(SessionMetadata& session, const std::string& branch_name);
};

// Helper function to find a branch by name in session metadata (public for use in CommandDispatcher)
std::optional<BranchMetadata> FindBranchByName(const SessionMetadata& session, const std::string& branch_name);

} // namespace ProtoVMCLI

#endif // _ProtoVM_BranchOperations_h_