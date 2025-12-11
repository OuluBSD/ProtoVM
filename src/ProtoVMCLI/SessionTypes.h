#ifndef _ProtoVM_SessionTypes_h_
#define _ProtoVM_SessionTypes_h_

#include <ProtoVM/ProtoVM.h>  // Include U++ types
#include "BranchTypes.h"  // Include branch types
#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace ProtoVMCLI {

// Error codes for the CLI system
enum class ErrorCode {
    None = 0,
    WorkspaceNotFound,
    InvalidWorkspace,
    WorkspaceCorrupt,
    SessionNotFound,
    SessionCorrupt,
    SessionDeleted,
    SessionIdConflict,
    CircuitFileNotFound,
    CircuitFileUnreadable,
    StorageIoError,
    StorageSchemaMismatch,
    CommandParseError,
    InternalError,
    Conflict,
    InvalidEditOperation,
    CircuitStateCorrupt
    // add more if needed
};

// Result template to encapsulate success/error responses
template<typename T>
struct Result {
    bool ok;
    ErrorCode error_code;
    std::string error_message;  // human-readable message
    T data;

    Result(T val) : ok(true), error_code(ErrorCode::None), error_message(""), data(val) {}
    Result(bool is_ok, ErrorCode code, const std::string& msg, T val)
        : ok(is_ok), error_code(code), error_message(msg), data(val) {}

    static Result<T> MakeOk(const T& data) {
        return Result<T>(true, ErrorCode::None, "", data);
    }

    static Result<T> MakeError(ErrorCode code, const std::string& message) {
        return Result<T>(false, code, message, T{});
    }
};

// Specialization for void type
template<>
struct Result<void> {
    bool ok;
    ErrorCode error_code;
    std::string error_message;  // human-readable message

    Result() : ok(true), error_code(ErrorCode::None), error_message("") {}
    Result(bool is_ok, ErrorCode code, const std::string& msg)
        : ok(is_ok), error_code(code), error_message(msg) {}

    static Result<void> MakeOk() {
        return Result<void>(true, ErrorCode::None, "");
    }

    static Result<void> MakeError(ErrorCode code, const std::string& message) {
        return Result<void>(false, code, message);
    }
};

// Session state enum
enum class SessionState {
    CREATED,
    READY,
    RUNNING,
    ERROR,
    DELETED
};

// Session metadata structure
struct SessionMetadata {
    int session_id = -1;
    std::string created_at;
    std::string last_used_at;
    std::string circuit_file;
    SessionState state = SessionState::CREATED;
    int total_ticks = 0;
    int circuit_revision = 0;  // Revision of the circuit (editing operations) - DEPRECATED: use branches info now
    int sim_revision = 0;      // Revision on which the latest simulation snapshot is based - DEPRECATED: use branches info now
    std::string workspace;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point last_used_time;

    // Branch information
    std::string current_branch = "main";  // Current active branch
    std::vector<BranchMetadata> branches;  // List of all branches

    SessionMetadata() {
        auto now = std::chrono::system_clock::now();
        created_time = now;
        last_used_time = now;
        // Format timestamp as ISO 8601 string
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::gmtime(&time_t);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
        created_at = buffer;
        last_used_at = buffer;

        // Initialize with a default main branch
        BranchMetadata main_branch("main", 0, 0, 0, true);
        branches.push_back(main_branch);
    }
};

// Information needed to create a session
struct SessionCreateInfo {
    std::string workspace;
    std::string circuit_file;
    std::string circuit_name;
    
    SessionCreateInfo(const std::string& ws, const std::string& cf) 
        : workspace(ws), circuit_file(cf) {
        // Extract circuit name from file path
        size_t pos = circuit_file.find_last_of("/\\");
        if (pos != std::string::npos) {
            circuit_name = circuit_file.substr(pos + 1);
        } else {
            circuit_name = circuit_file;
        }
    }
};

// Command options structure
struct CommandOptions {
    std::string workspace;
    std::optional<int> session_id;
    std::optional<int> ticks;
    std::optional<int> pcb_id;
    std::optional<std::string> circuit_file;
    std::optional<std::string> netlist_file;
    std::optional<bool> soft_delete;
    std::optional<std::string> branch;  // Branch name for branch-aware operations
    std::optional<std::string> branch_from;  // Source branch for operations like merge or create
    std::optional<std::string> branch_to;    // Target branch for operations like merge
    std::optional<std::string> branch_name;  // Name of branch to create or switch to
    // Graph query parameters
    std::optional<std::string> graph_source_kind;  // Component, Pin, Net for graph queries
    std::optional<std::string> graph_source_id;    // ID for source node in graph queries
    std::optional<std::string> graph_target_kind;  // Component, Pin, Net for graph queries
    std::optional<std::string> graph_target_id;    // ID for target node in graph queries
    std::optional<std::string> graph_node_kind;    // Node kind for single-node queries
    std::optional<std::string> graph_node_id;      // Node ID for single-node queries
    std::optional<int> graph_max_depth;            // Max depth for graph queries
    // Dependency analysis parameters
    std::string deps_node_id;                      // Node ID for dependency analysis
    std::string deps_node_kind;                    // Node kind (Pin, Component, Net) for dependency analysis
    int deps_max_depth = 128;                      // Max depth for dependency analysis
    std::string user_id = "anonymous";  // Default user ID

    // Retiming application parameters
    std::optional<bool> apply_only_safe;    // Apply only safe moves (default true)
    std::optional<bool> allow_suspicious;   // Allow suspicious moves (default false)
    std::optional<int> max_moves;           // Max number of moves to apply (default -1, no limit)

    // Code generation parameters
    std::optional<std::string> block_id;        // Block ID for codegen operations
    std::optional<std::string> node_id;         // Node ID for codegen operations
    std::optional<std::string> lang;            // Language for codegen: "C" or "Cpp"
    std::optional<std::string> emit_state_struct;  // Whether to emit state struct (true/false as string)
    std::optional<std::string> state_struct_name;  // State struct name
    std::optional<std::string> function_name;      // Function name for generated code
    std::optional<std::string> step_function_name; // Step function name for oscillator demos
    std::optional<std::string> render_function_name; // Render function name for oscillator demos

    // Add more options as needed
};

} // namespace ProtoVMCLI

#endif