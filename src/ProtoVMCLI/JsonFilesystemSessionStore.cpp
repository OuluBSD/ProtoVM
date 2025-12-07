#include "SessionStore.h"
#include "ProtoVM.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;

namespace ProtoVMCLI {

// Helper function to generate ISO 8601 timestamp
std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::gmtime(&time_t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
}

// Helper function to perform atomic write of JSON files
bool AtomicWriteJsonFile(const fs::path& file_path, const Upp::String& content) {
    try {
        fs::path temp_path = file_path;
        temp_path += ".tmp";

        std::ofstream temp_file(temp_path);
        if (!temp_file.is_open()) {
            return false;
        }

        temp_file << content.ToStd();
        temp_file.flush();
        temp_file.close();

        // Atomic rename operation
        fs::rename(temp_path, file_path);
        return true;
    } catch (...) {
        return false;
    }
}

class JsonFilesystemSessionStore : public ISessionStore {
public:
    JsonFilesystemSessionStore(const std::string& workspace_path)
        : workspace_path_(workspace_path) {
        sessions_dir_ = fs::path(workspace_path_) / "sessions";
    }

    Result<int> CreateSession(const SessionCreateInfo& info) override {
        try {
            // Find next available session ID from workspace.json
            int next_id = GetNextSessionId();

            // Create workspace/sessions directory if it doesn't exist
            if (!fs::exists(sessions_dir_)) {
                fs::create_directories(sessions_dir_);
            }

            fs::path session_dir = sessions_dir_ / std::to_string(next_id);
            fs::create_directories(session_dir);

            // Copy circuit file to session directory
            fs::path circuit_path(info.circuit_file);
            fs::path target_circuit_path = session_dir / circuit_path.filename();
            fs::copy_file(circuit_path, target_circuit_path, fs::copy_options::overwrite_existing);

            // Create initial session metadata
            SessionMetadata metadata;
            metadata.session_id = next_id;
            metadata.circuit_file = target_circuit_path.string();
            metadata.workspace = info.workspace;
            metadata.state = SessionState::CREATED;
            metadata.created_at = GetCurrentTimestamp();
            metadata.last_used_at = metadata.created_at;  // Initially same as created_at

            // Save session metadata with proper schema
            fs::path metadata_path = session_dir / "session.json";

            Upp::String json_content = "{\n";
            json_content += "  \"schema_version\": 1,\n";
            json_content += "  \"session_id\": " + Upp::AsString(metadata.session_id) + ",\n";
            json_content += "  \"state\": " + Upp::AsString(static_cast<int>(metadata.state)) + ",\n";
            json_content += "  \"circuit_file\": \"" + Upp::String(metadata.circuit_file.c_str()) + "\",\n";
            json_content += "  \"created_at\": \"" + Upp::String(metadata.created_at.c_str()) + "\",\n";
            json_content += "  \"last_used_at\": \"" + Upp::String(metadata.last_used_at.c_str()) + "\",\n";
            json_content += "  \"total_ticks\": " + Upp::AsString(metadata.total_ticks) + ",\n";
            json_content += "  \"engine_version\": \"unknown\"\n";  // Placeholder
            json_content += "}";

            if (!AtomicWriteJsonFile(metadata_path, json_content)) {
                return Result<int>::MakeError(ErrorCode::StorageIoError,
                                             "Could not create session metadata file");
            }

            // Update workspace.json with the new next_session_id
            if (!IncrementNextSessionId(next_id + 1)) {
                return Result<int>::MakeError(ErrorCode::StorageIoError,
                                             "Could not update workspace next_session_id");
            }

            return Result<int>::MakeOk(next_id);
        } catch (const std::exception& e) {
            return Result<int>::MakeError(ErrorCode::InternalError,
                                         "Failed to create session: " + std::string(e.what()));
        }
    }

    Result<SessionMetadata> LoadSession(int session_id) override {
        try {
            fs::path session_dir = sessions_dir_ / std::to_string(session_id);
            fs::path metadata_path = session_dir / "session.json";

            if (!fs::exists(metadata_path)) {
                return Result<SessionMetadata>::MakeError(ErrorCode::SessionNotFound,
                                                        "Session not found: " + std::to_string(session_id));
            }

            std::ifstream file(metadata_path);
            if (!file.is_open()) {
                return Result<SessionMetadata>::MakeError(ErrorCode::StorageIoError,
                                                        "Could not open session file");
            }

            // Read the entire file into a string
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            file.close();

            // Parse and validate the session metadata
            SessionMetadata metadata;
            metadata.session_id = session_id;  // Set from the parameter since it's reliable

            // Extract values from JSON string - this is a simplified parser for our format
            std::string content_copy = content;

            // Validate schema version
            size_t schema_pos = content_copy.find("\"schema_version\": ");
            if (schema_pos == std::string::npos) {
                return Result<SessionMetadata>::MakeError(ErrorCode::StorageSchemaMismatch,
                                                        "Missing schema_version in session.json");
            }

            size_t schema_val_start = schema_pos + 18; // Length of "\"schema_version\": "
            size_t comma_pos = content_copy.find(",", schema_val_start);
            size_t brace_pos = content_copy.find("}", schema_val_start);
            size_t end_pos = (comma_pos < brace_pos) ? comma_pos : brace_pos;

            if (end_pos == std::string::npos) {
                return Result<SessionMetadata>::MakeError(ErrorCode::StorageSchemaMismatch,
                                                        "Invalid schema_version format in session.json");
            }

            std::string schema_str = content_copy.substr(schema_val_start, end_pos - schema_val_start);
            int schema_version = 0;
            try {
                schema_version = std::stoi(schema_str);
                if (schema_version != 1) {
                    return Result<SessionMetadata>::MakeError(ErrorCode::StorageSchemaMismatch,
                                                            "Unsupported schema_version: " + std::to_string(schema_version));
                }
            } catch (...) {
                return Result<SessionMetadata>::MakeError(ErrorCode::StorageSchemaMismatch,
                                                        "Invalid schema_version in session.json");
            }

            // Extract created_at
            size_t pos = content_copy.find("\"created_at\": \"");
            if (pos != std::string::npos) {
                pos += 15; // Length of "\"created_at\": \""
                size_t end_pos = content_copy.find("\"", pos);
                if (end_pos != std::string::npos) {
                    metadata.created_at = content_copy.substr(pos, end_pos - pos);
                }
            }

            // Extract last_used_at
            pos = content_copy.find("\"last_used_at\": \"");
            if (pos != std::string::npos) {
                pos += 18; // Length of "\"last_used_at\": \""
                size_t end_pos = content_copy.find("\"", pos);
                if (end_pos != std::string::npos) {
                    metadata.last_used_at = content_copy.substr(pos, end_pos - pos);
                }
            }

            // Extract circuit_file
            pos = content_copy.find("\"circuit_file\": \"");
            if (pos != std::string::npos) {
                pos += 18; // Length of "\"circuit_file\": \""
                size_t end_pos = content_copy.find("\"", pos);
                if (end_pos != std::string::npos) {
                    metadata.circuit_file = content_copy.substr(pos, end_pos - pos);
                }
            }

            // Extract state
            pos = content_copy.find("\"state\": ");
            if (pos != std::string::npos) {
                pos += 9; // Length of "\"state\": "
                size_t comma_pos = content_copy.find(",", pos);
                size_t brace_pos = content_copy.find("}", pos);
                end_pos = (comma_pos < brace_pos) ? comma_pos : brace_pos;
                if (end_pos != std::string::npos) {
                    std::string state_str = content_copy.substr(pos, end_pos - pos);
                    try {
                        int state_val = std::stoi(state_str);
                        metadata.state = static_cast<SessionState>(state_val);
                    } catch (...) {
                        metadata.state = SessionState::CREATED; // default
                    }
                }
            }

            // Extract total_ticks
            pos = content_copy.find("\"total_ticks\": ");
            if (pos != std::string::npos) {
                pos += 16; // Length of "\"total_ticks\": "
                size_t comma_pos = content_copy.find(",", pos);
                size_t brace_pos = content_copy.find("}", pos);
                end_pos = (comma_pos < brace_pos) ? comma_pos : brace_pos;
                if (end_pos != std::string::npos) {
                    std::string ticks_str = content_copy.substr(pos, end_pos - pos);
                    try {
                        metadata.total_ticks = std::stoi(ticks_str);
                    } catch (...) {
                        metadata.total_ticks = 0; // default
                    }
                }
            }

            // Extract workspace
            pos = content_copy.find("\"workspace\": \"");
            if (pos != std::string::npos) {
                pos += 14; // Length of "\"workspace\": \""
                size_t end_pos = content_copy.find("\"", pos);
                if (end_pos != std::string::npos) {
                    metadata.workspace = content_copy.substr(pos, end_pos - pos);
                }
            }

            return Result<SessionMetadata>::MakeOk(metadata);
        } catch (const std::exception& e) {
            return Result<SessionMetadata>::MakeError(ErrorCode::SessionCorrupt,
                                                    "Failed to load session: " + std::string(e.what()));
        }
    }

    Result<bool> SaveSession(const SessionMetadata& metadata) override {
        try {
            fs::path session_dir = sessions_dir_ / std::to_string(metadata.session_id);
            fs::path metadata_path = session_dir / "session.json";

            // Ensure directory exists
            if (!fs::exists(session_dir)) {
                fs::create_directories(session_dir);
            }

            // Update last_used_at timestamp
            std::string last_used_at = GetCurrentTimestamp();

            // Create a properly formatted JSON string
            Upp::String json_content = "{\n";
            json_content += "  \"schema_version\": 1,\n";
            json_content += "  \"session_id\": " + Upp::AsString(metadata.session_id) + ",\n";
            json_content += "  \"state\": " + Upp::AsString(static_cast<int>(metadata.state)) + ",\n";
            json_content += "  \"circuit_file\": \"" + Upp::String(metadata.circuit_file.c_str()) + "\",\n";
            json_content += "  \"created_at\": \"" + Upp::String(metadata.created_at.c_str()) + "\",\n";
            json_content += "  \"last_used_at\": \"" + Upp::String(last_used_at.c_str()) + "\",\n";
            json_content += "  \"total_ticks\": " + Upp::AsString(metadata.total_ticks) + ",\n";
            json_content += "  \"engine_version\": \"unknown\"\n";  // Placeholder
            json_content += "}";

            if (!AtomicWriteJsonFile(metadata_path, json_content)) {
                return Result<bool>::MakeError(ErrorCode::StorageIoError,
                                              "Could not save session file");
            }

            return Result<bool>::MakeOk(true);
        } catch (const std::exception& e) {
            return Result<bool>::MakeError(ErrorCode::StorageIoError,
                                          "Failed to save session: " + std::string(e.what()));
        }
    }

    Result<ListSessionsResult> ListSessions() override {
        try {
            std::vector<SessionMetadata> sessions;
            std::vector<int> corrupt_sessions;

            if (!fs::exists(sessions_dir_)) {
                ListSessionsResult result;
                result.sessions = sessions;
                result.corrupt_sessions = corrupt_sessions;
                return Result<ListSessionsResult>::MakeOk(result);
            }

            for (const auto& entry : fs::directory_iterator(sessions_dir_)) {
                if (entry.is_directory()) {
                    // Extract session ID from directory name
                    std::string dir_name = entry.path().filename().string();

                    // Check if it's a valid numeric directory name
                    if (std::all_of(dir_name.begin(), dir_name.end(), ::isdigit)) {
                        int session_id = std::stoi(dir_name);

                        // Load this session's metadata
                        auto load_result = LoadSession(session_id);
                        if (load_result.ok) {
                            sessions.push_back(load_result.data);
                        } else {
                            // Session is corrupt, add to corrupt list
                            corrupt_sessions.push_back(session_id);
                        }
                    }
                }
            }

            // Sort sessions by ID
            std::sort(sessions.begin(), sessions.end(),
                     [](const SessionMetadata& a, const SessionMetadata& b) {
                         return a.session_id < b.session_id;
                     });

            ListSessionsResult result;
            result.sessions = sessions;
            result.corrupt_sessions = corrupt_sessions;

            return Result<ListSessionsResult>::MakeOk(result);
        } catch (const std::exception& e) {
            return Result<ListSessionsResult>::MakeError(ErrorCode::InternalError,
                                                        "Failed to list sessions: " + std::string(e.what()));
        }
    }

    Result<bool> DeleteSession(int session_id) override {
        try {
            fs::path session_dir = sessions_dir_ / std::to_string(session_id);

            if (!fs::exists(session_dir)) {
                return Result<bool>::MakeError(ErrorCode::SessionNotFound,
                                              "Session directory does not exist");
            }

            // Remove the entire session directory and all its contents
            fs::remove_all(session_dir);

            return Result<bool>::MakeOk(true);
        } catch (const std::exception& e) {
            return Result<bool>::MakeError(ErrorCode::StorageIoError,
                                          "Failed to delete session: " + std::string(e.what()));
        }
    }

    Result<bool> UpdateSessionState(int session_id, SessionState state) override {
        auto result = LoadSession(session_id);
        if (!result.ok) {
            return result.error_code == ErrorCode::SessionNotFound
                ? Result<bool>::MakeError(ErrorCode::SessionNotFound, result.error_message)
                : Result<bool>::MakeError(ErrorCode::SessionCorrupt, result.error_message);
        }

        SessionMetadata metadata = result.data;
        metadata.state = state;

        return SaveSession(metadata);
    }

    Result<bool> UpdateSessionTicks(int session_id, int ticks) override {
        auto result = LoadSession(session_id);
        if (!result.ok) {
            return result.error_code == ErrorCode::SessionNotFound
                ? Result<bool>::MakeError(ErrorCode::SessionNotFound, result.error_message)
                : Result<bool>::MakeError(ErrorCode::SessionCorrupt, result.error_message);
        }

        SessionMetadata metadata = result.data;
        metadata.total_ticks = ticks;

        return SaveSession(metadata);
    }

private:
    std::string workspace_path_;
    fs::path sessions_dir_;

    // Get next session ID from workspace.json
    int GetNextSessionId() {
        fs::path workspace_json_path = fs::path(workspace_path_) / "workspace.json";

        if (!fs::exists(workspace_json_path)) {
            // If workspace.json doesn't exist, return 1 as initial ID
            return 1;
        }

        // Read and parse workspace.json to get next_session_id
        std::ifstream file(workspace_json_path);
        if (!file.is_open()) {
            return 1;  // Default to 1 if we can't read the file
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        // Extract next_session_id from JSON
        size_t pos = content.find("\"next_session_id\": ");
        if (pos != std::string::npos) {
            pos += 21; // Length of "\"next_session_id\": "
            size_t end_pos = content.find(",", pos);
            if (end_pos == std::string::npos) {
                end_pos = content.find("}", pos);
            }
            if (end_pos != std::string::npos) {
                std::string id_str = content.substr(pos, end_pos - pos);
                try {
                    return std::stoi(id_str);
                } catch (...) {
                    return 1;  // Default to 1 if parsing fails
                }
            }
        }

        return 1;  // Default to 1 if field not found
    }

    // Increment next session ID in workspace.json
    bool IncrementNextSessionId(int next_id) {
        fs::path workspace_json_path = fs::path(workspace_path_) / "workspace.json";

        if (!fs::exists(workspace_json_path)) {
            // If workspace.json doesn't exist, create it with initial values
            Upp::String initial_content = "{\n";
            initial_content += "  \"schema_version\": 1,\n";
            initial_content += "  \"created_at\": \"" + Upp::String(GetCurrentTimestamp().c_str()) + "\",\n";
            initial_content += "  \"created_with\": \"proto-vm-cli/0.1.0\",\n";
            initial_content += "  \"engine_version\": \"unknown\",\n";
            initial_content += "  \"next_session_id\": " + Upp::AsString(next_id) + "\n";
            initial_content += "}";

            return AtomicWriteJsonFile(workspace_json_path, initial_content);
        }

        // Read existing workspace.json
        std::ifstream file(workspace_json_path);
        if (!file.is_open()) {
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        // Find and replace next_session_id
        size_t pos = content.find("\"next_session_id\": ");
        if (pos == std::string::npos) {
            return false;  // Invalid workspace.json format
        }

        pos += 21; // Length of "\"next_session_id\": "
        size_t end_pos = content.find(",", pos);
        if (end_pos == std::string::npos) {
            end_pos = content.find("}", pos);
        }
        if (end_pos == std::string::npos) {
            return false;  // Invalid workspace.json format
        }

        std::string before = content.substr(0, pos);
        std::string after = content.substr(end_pos);

        Upp::String new_content = before + Upp::AsString(next_id) + after;

        return AtomicWriteJsonFile(workspace_json_path, new_content);
    }
};

// Factory function to create the filesystem session store
std::unique_ptr<ISessionStore> CreateFilesystemSessionStore(const std::string& workspace_path) {
    return std::make_unique<JsonFilesystemSessionStore>(workspace_path);
}

} // namespace ProtoVMCLI