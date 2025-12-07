#ifndef _ProtoVM_SessionTypes_h_
#define _ProtoVM_SessionTypes_h_

#include "ProtoVM.h"  // Include U++ types
#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace ProtoVMCLI {

// Result template to encapsulate success/error responses
template<typename T>
struct Result {
    bool success;
    T value;
    std::string error;
    std::string error_code;

    Result(T val) : success(true), value(val) {}
    Result(bool is_success, T val, const std::string& err = "", const std::string& code = "") 
        : success(is_success), value(val), error(err), error_code(code) {}
    
    static Result<T> Success(T val) {
        return Result<T>(true, val, "", "");
    }
    
    static Result<T> Error(const std::string& msg, const std::string& code = "") {
        return Result<T>(false, T{}, msg, code);
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
    std::string workspace;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point last_used_time;
    
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
    
    // Add more options as needed
};

} // namespace ProtoVMCLI

#endif