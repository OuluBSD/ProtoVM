#ifndef _ProtoVM_SessionTypes_h_
#define _ProtoVM_SessionTypes_h_

#include <ProtoVM/ProtoVM.h>  // Include U++ types
#include "BranchTypes.h"  // Include branch types
#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace ProtoVMCLI {

// String-typed command member that also satisfies the optional-flavored usage
// idioms the handler layer was generated against (.empty(), .c_str(), .find(),
// .substr(), .value_or(), assignment from std::string/const char*).
struct OptStr {
    std::string s;
    bool set = false;

    OptStr() = default;
    OptStr(const char* x) : s(x ? x : ""), set(true) {}
    OptStr(const std::string& x) : s(x), set(true) {}
    OptStr& operator=(const std::string& x) { s = x; set = true; return *this; }
    OptStr& operator=(const char* x) { s = x ? x : ""; set = true; return *this; }

    bool has_value() const { return set; }
    std::string value_or(const std::string& d) const { return set ? s : d; }
    const std::string& value() const { return s; }
    const std::string& operator*() const { return s; }
    explicit operator bool() const { return !s.empty(); }   // "usable" = non-empty
    const char* c_str() const { return s.c_str(); }
    const std::string& str() const { return s; }
    operator const std::string&() const { return s; }

    bool empty() const { return s.empty(); }
    size_t size() const { return s.size(); }
    size_t length() const { return s.length(); }
    size_t find(const std::string& x, size_t pos = 0) const { return s.find(x, pos); }
    size_t find(char x, size_t pos = 0) const { return s.find(x, pos); }
    std::string substr(size_t pos = 0, size_t n = std::string::npos) const { return s.substr(pos, n); }
};

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
    NotFound,
    InvalidArgument,
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

    Result(T val) : ok(true), error_code(ErrorCode::None), error_message(""), data(std::move(val)) {}
    Result() : ok(false), error_code(ErrorCode::InternalError), error_message(""), data() {}
    Result(bool is_ok, ErrorCode code, const std::string& msg, T val)
        : ok(is_ok), error_code(code), error_message(msg), data(std::move(val)) {}

    static Result<T> MakeOk(T data) {
        return Result<T>(true, ErrorCode::None, "", std::move(data));
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
    std::string workspace_path;  // absolute workspace path (handler layer)
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
    OptStr branch;  // Branch name for branch-aware operations
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
    std::optional<int> min_depth;           // Min depth threshold for path queries
    OptStr max_depth;                       // Max depth for traversal (string form)
    std::optional<std::string> max_plans;   // Max number of refactoring plans to consider
    OptStr plan_id;     // Specific plan id to apply

    // Code generation parameters
    OptStr block_id;        // Block ID for codegen operations
    OptStr node_id;         // Node ID for codegen operations
    OptStr lang;            // Language for codegen: "C" or "Cpp"
    OptStr emit_state_struct;  // Whether to emit state struct (true/false as string)
    OptStr state_struct_name;  // State struct name
    OptStr function_name;      // Function name for generated code
    OptStr step_function_name; // Step function name for oscillator demos
    OptStr render_function_name; // Render function name for oscillator demos

    // Hybrid instrument parameters
    std::optional<std::string> instrument_id;            // Instrument ID for instrument operations
    std::optional<std::string> analog_block_id;          // Analog block ID for hybrid instruments
    std::optional<std::string> digital_block_id;         // Digital block ID for hybrid instruments
    std::optional<std::string> voice_count;              // Number of voices for hybrid instruments
    std::optional<std::string> sample_rate;              // Sample rate in Hz for hybrid instruments
    std::optional<std::string> duration_sec;             // Duration in seconds for hybrid instruments
    std::optional<std::string> base_freq_hz;             // Base frequency in Hz for hybrid instruments
    std::optional<std::string> detune_spread_cents;      // Detune spread in cents for hybrid instruments
    std::optional<std::string> pan_lfo_hz;               // Pan LFO rate in Hz for hybrid instruments
    bool use_analog_primary = false;                     // Whether analog source is primary

    // Instrument export parameters
    std::optional<std::string> program_name;             // Program name for exported C++ code
    std::optional<std::string> namespace_name;           // Namespace for exported C++ code
    std::optional<std::string> wav_filename;             // Output WAV filename for exported C++ code
    bool no_wav_writer = false;                          // Whether to exclude WAV writer in export
    std::optional<std::string> instrument_from_json;     // Load instrument from JSON file

    // Plugin skeleton export parameters
    OptStr plugin_target;            // Target plugin format (vst3, lv2, clap, ladspa)
    OptStr plugin_name;              // Plugin name
    OptStr plugin_id;                // Plugin ID
    OptStr vendor;                   // Plugin vendor name
    OptStr version;                  // Plugin version string
    OptStr output_dir;               // Output directory for plugin project
    OptStr node_kind;                // Node kind hint (Pin, Component, Net)
    OptStr node_kind_hint;           // Node kind hint (handler-layer alias)
    OptStr subsystem_id;             // Subsystem ID
    OptStr block_ids;                // Comma/space separated block ids
    std::string branch_before;                           // Branch ref (before side)
    std::string branch_after;                            // Branch ref (after side)
    std::string passes;                                  // Comma separated pass list

    // Free-form command payload (JSON object with command-specific keys)
    Upp::ValueMap payload;

    // Add more options as needed

    // Kebab-case settings accessor used by the handler layer. Reads the
    // free-form JSON payload first, then falls back to the parsed command
    // members; otherwise returns def.
    std::string Get(const std::string& key, const std::string& def = "") const {
        int q = payload.Find(key);
        if (q >= 0)
            return payload.GetValue(q).ToString().ToStd();
        if (key == "workspace" || key == "workspace-path") return workspace;
        if (key == "session-id") return session_id ? std::to_string(*session_id) : def;
        if (key == "block-id") return block_id.value_or(def);
        if (key == "max-moves") return max_moves ? std::to_string(*max_moves) : def;
        if (key == "branch") return branch.value_or(def);
        if (key == "plan-id") return plan_id.value_or(def);
        if (key == "apply-only-safe") return apply_only_safe ? (*apply_only_safe ? "true" : "false") : def;
        if (key == "allow-suspicious") return allow_suspicious ? (*allow_suspicious ? "true" : "false") : def;
        if (key == "safe-only") return apply_only_safe ? (*apply_only_safe ? "true" : "false") : def;
        if (key == "state-struct-name") return state_struct_name.value_or(def);
        if (key == "lang") return lang.value_or(def);
        if (key == "step-function-name") return step_function_name.value_or(def);
        if (key == "render-function-name") return render_function_name.value_or(def);
        if (key == "function-name") return function_name.value_or(def);
        if (key == "emit-state-struct") return emit_state_struct.value_or(def);
        return def;
    }
};

} // namespace ProtoVMCLI

#endif