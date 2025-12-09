#include "BranchOperations.h"
#include "SessionStore.h"  // For ISessionStore
#include <algorithm>
#include <regex>

namespace ProtoVMCLI {

bool BranchOperations::IsValidBranchName(const std::string& branch_name) {
    if (branch_name.empty() || branch_name.length() > 100) {
        return false;
    }
    
    // Simple validation: only alphanumeric, hyphens, and underscores
    std::regex pattern("^[a-zA-Z0-9_-]+$");
    return std::regex_match(branch_name, pattern);
}

int BranchOperations::FindBranchIndex(SessionMetadata& session, const std::string& branch_name) {
    for (size_t i = 0; i < session.branches.size(); ++i) {
        if (session.branches[i].name == branch_name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Result<BranchListResult> BranchOperations::ListBranches(
    const SessionMetadata& session
) {
    BranchListResult result;
    result.session_id = session.session_id;
    result.branches = session.branches;
    result.current_branch = session.current_branch;
    
    return Result<BranchListResult>::MakeOk(result);
}

Result<BranchCreateResult> BranchOperations::CreateBranch(
    SessionMetadata& session,
    const std::string& branch_name,
    const std::string& from_branch,
    int64_t from_revision
) {
    // Validate branch name
    if (!IsValidBranchName(branch_name)) {
        return Result<BranchCreateResult>::MakeError(
            ErrorCode::InvalidEditOperation,
            "Invalid branch name: " + branch_name
        );
    }
    
    // Check if branch already exists
    for (const auto& branch : session.branches) {
        if (branch.name == branch_name) {
            return Result<BranchCreateResult>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Branch already exists: " + branch_name
            );
        }
    }
    
    // Determine source branch and revision
    std::string source_branch = from_branch.empty() ? session.current_branch : from_branch;
    int64_t source_revision = from_revision;
    
    // Find the source branch
    std::optional<BranchMetadata> source_branch_meta = FindBranchByName(session, source_branch);
    if (!source_branch_meta.has_value()) {
        return Result<BranchCreateResult>::MakeError(
            ErrorCode::InvalidEditOperation,
            "Source branch not found: " + source_branch
        );
    }
    
    // If from_revision is -1, use the head revision of the source branch
    if (from_revision == -1) {
        source_revision = source_branch_meta->head_revision;
    }
    
    // Create the new branch
    BranchMetadata new_branch;
    new_branch.name = branch_name;
    new_branch.head_revision = source_revision;
    new_branch.sim_revision = source_revision;  // Initially, sim revision matches circuit revision
    new_branch.base_revision = source_revision;
    new_branch.is_default = false;
    
    session.branches.push_back(new_branch);
    
    BranchCreateResult result;
    result.session_id = session.session_id;
    result.branch = new_branch;
    
    return Result<BranchCreateResult>::MakeOk(result);
}

Result<BranchSwitchResult> BranchOperations::SwitchBranch(
    SessionMetadata& session,
    const std::string& branch_name
) {
    // Check if branch exists
    bool branch_exists = false;
    for (const auto& branch : session.branches) {
        if (branch.name == branch_name) {
            branch_exists = true;
            break;
        }
    }
    
    if (!branch_exists) {
        return Result<BranchSwitchResult>::MakeError(
            ErrorCode::InvalidEditOperation,
            "Branch not found: " + branch_name
        );
    }
    
    // Update current branch
    session.current_branch = branch_name;
    
    BranchSwitchResult result;
    result.session_id = session.session_id;
    result.current_branch = branch_name;
    
    return Result<BranchSwitchResult>::MakeOk(result);
}

Result<BranchDeleteResult> BranchOperations::DeleteBranch(
    SessionMetadata& session,
    const std::string& branch_name
) {
    // Cannot delete the current branch
    if (session.current_branch == branch_name) {
        return Result<BranchDeleteResult>::MakeError(
            ErrorCode::InvalidEditOperation,
            "Cannot delete the current branch: " + branch_name
        );
    }
    
    // Find the branch to delete
    int branch_index = FindBranchIndex(session, branch_name);
    if (branch_index == -1) {
        return Result<BranchDeleteResult>::MakeError(
            ErrorCode::InvalidEditOperation,
            "Branch not found: " + branch_name
        );
    }
    
    // Cannot delete the default branch
    if (session.branches[branch_index].is_default) {
        return Result<BranchDeleteResult>::MakeError(
            ErrorCode::InvalidEditOperation,
            "Cannot delete the default branch: " + branch_name
        );
    }
    
    // Remove the branch
    session.branches.erase(session.branches.begin() + branch_index);
    
    BranchDeleteResult result;
    result.session_id = session.session_id;
    result.deleted_branch = branch_name;
    
    return Result<BranchDeleteResult>::MakeOk(result);
}

Result<BranchMergeResult> BranchOperations::MergeBranch(
    SessionMetadata& session,
    const std::string& source_branch,
    const std::string& target_branch,
    bool allow_merge
) {
    // Find source and target branches
    int source_index = FindBranchIndex(session, source_branch);
    if (source_index == -1) {
        return Result<BranchMergeResult>::MakeError(
            ErrorCode::InvalidEditOperation,
            "Source branch not found: " + source_branch
        );
    }

    int target_index = FindBranchIndex(session, target_branch);
    if (target_index == -1) {
        return Result<BranchMergeResult>::MakeError(
            ErrorCode::InvalidEditOperation,
            "Target branch not found: " + target_branch
        );
    }

    // Get source and target branch metadata
    const auto& source_branch_meta = session.branches[source_index];
    auto& target_branch_meta = session.branches[target_index];

    // Calculate the merge base (for simplicity, using the lesser of the two base revisions as a starting point)
    // In a real implementation, we would find the common ancestor through commit history
    int64_t base_revision = std::min(source_branch_meta.base_revision, target_branch_meta.base_revision);

    // For a proper merge, we would:
    // 1. Load circuit state from source branch at its head revision
    // 2. Load circuit state from target branch at its head revision
    // 3. Load circuit state from merge base
    // 4. Get all operations from base to source
    // 5. Get all operations from base to target
    // 6. Use CircuitMerge::MergeBranches to perform three-way merge

    // For now, we'll implement a simplified version that just fast-forwards the target branch
    // if the source branch is ahead and there are no complex conflicts

    BranchMergeResult result;
    result.session_id = session.session_id;
    result.source_branch = source_branch;
    result.target_branch = target_branch;

    if (source_branch_meta.head_revision > target_branch_meta.head_revision) {
        // Update the target branch to have the same head revision as source
        target_branch_meta.head_revision = source_branch_meta.head_revision;
        target_branch_meta.sim_revision = source_branch_meta.sim_revision;  // Update sim revision too

        result.target_new_revision = target_branch_meta.head_revision;
        result.merged_ops_count = static_cast<int>(source_branch_meta.head_revision - target_branch_meta.head_revision);

        return Result<BranchMergeResult>::MakeOk(result);
    } else {
        // If source isn't ahead, no merge is needed
        result.target_new_revision = target_branch_meta.head_revision;
        result.merged_ops_count = 0;

        return Result<BranchMergeResult>::MakeOk(result);
    }
}

std::optional<BranchMetadata> FindBranchByName(const SessionMetadata& session, const std::string& branch_name) {
    for (const auto& branch : session.branches) {
        if (branch.name == branch_name) {
            return branch;
        }
    }
    return std::nullopt;
}

} // namespace ProtoVMCLI