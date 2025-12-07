#include "SessionStore.h"
#include "ProtoVM.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace fs = std::filesystem;

namespace ProtoVMCLI {

class JsonFilesystemSessionStore : public ISessionStore {
public:
    JsonFilesystemSessionStore(const std::string& workspace_path) 
        : workspace_path_(workspace_path) {
        sessions_dir_ = fs::path(workspace_path_) / "sessions";
    }
    
    Result<int> CreateSession(const SessionCreateInfo& info) override {
        try {
            // Find next available session ID
            int next_id = getNextAvailableId();
            
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
            
            // Save session metadata
            fs::path metadata_path = session_dir / "session.json";
            std::ofstream file(metadata_path);
            if (!file.is_open()) {
                return Result<int>::Error("Could not create session metadata file", "FILE_IO_ERROR");
            }
            
            // Create a simple JSON string manually since U++ ValueMap doesn't directly serialize
            Upp::String json_content = "{\n";
            json_content += "  \"session_id\": " + Upp::AsString(metadata.session_id) + ",\n";
            json_content += "  \"created_at\": \"" + Upp::String(metadata.created_at.c_str()) + "\",\n";
            json_content += "  \"last_used_at\": \"" + Upp::String(metadata.last_used_at.c_str()) + "\",\n";
            json_content += "  \"circuit_file\": \"" + Upp::String(metadata.circuit_file.c_str()) + "\",\n";
            json_content += "  \"state\": " + Upp::AsString(static_cast<int>(metadata.state)) + ",\n";
            json_content += "  \"total_ticks\": " + Upp::AsString(metadata.total_ticks) + ",\n";
            json_content += "  \"workspace\": \"" + Upp::String(metadata.workspace.c_str()) + "\"\n";
            json_content += "}";
            
            file << json_content.ToStd();
            file.close();
            
            return Result<int>::Success(next_id);
        } catch (const std::exception& e) {
            return Result<int>::Error("Failed to create session: " + std::string(e.what()), "SESSION_CREATION_ERROR");
        }
    }
    
    Result<SessionMetadata> LoadSession(int session_id) override {
        try {
            fs::path session_dir = sessions_dir_ / std::to_string(session_id);
            fs::path metadata_path = session_dir / "session.json";
            
            if (!fs::exists(metadata_path)) {
                return Result<SessionMetadata>::Error("Session not found: " + std::to_string(session_id), "SESSION_NOT_FOUND");
            }
            
            std::ifstream file(metadata_path);
            if (!file.is_open()) {
                return Result<SessionMetadata>::Error("Could not open session file", "FILE_IO_ERROR");
            }
            
            // Read the entire file into a string
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            file.close();
            
            // Parse the simple JSON manually
            SessionMetadata metadata;
            metadata.session_id = session_id;  // Set from the parameter since it's reliable
            
            // Extract values from JSON string - this is a simplified parser for our format
            // In a real implementation, we'd use a proper JSON parser
            std::string content_copy = content;
            
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
                size_t end_pos = content_copy.find(",", pos);
                if (end_pos == std::string::npos) {
                    end_pos = content_copy.find("}", pos);
                }
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
                size_t end_pos = content_copy.find(",", pos);
                if (end_pos == std::string::npos) {
                    end_pos = content_copy.find("}", pos);
                }
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
            
            return Result<SessionMetadata>::Success(metadata);
        } catch (const std::exception& e) {
            return Result<SessionMetadata>::Error("Failed to load session: " + std::string(e.what()), "SESSION_LOAD_ERROR");
        }
    }
    
    Result<bool> SaveSession(const SessionMetadata& metadata) override {
        try {
            fs::path session_dir = sessions_dir_ / std::to_string(metadata.session_id);
            fs::path metadata_path = session_dir / "session.json";
            
            // Update last_used_at timestamp
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::gmtime(&time_t);
            char buffer[32];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
            
            std::string last_used_at(buffer);
            
            // Create a simple JSON string manually
            Upp::String json_content = "{\n";
            json_content += "  \"session_id\": " + Upp::AsString(metadata.session_id) + ",\n";
            json_content += "  \"created_at\": \"" + Upp::String(metadata.created_at.c_str()) + "\",\n";
            json_content += "  \"last_used_at\": \"" + Upp::String(last_used_at.c_str()) + "\",\n";
            json_content += "  \"circuit_file\": \"" + Upp::String(metadata.circuit_file.c_str()) + "\",\n";
            json_content += "  \"state\": " + Upp::AsString(static_cast<int>(metadata.state)) + ",\n";
            json_content += "  \"total_ticks\": " + Upp::AsString(metadata.total_ticks) + ",\n";
            json_content += "  \"workspace\": \"" + Upp::String(metadata.workspace.c_str()) + "\"\n";
            json_content += "}";
            
            std::ofstream file(metadata_path);
            if (!file.is_open()) {
                return Result<bool>::Error("Could not save session file", "FILE_IO_ERROR");
            }
            
            file << json_content.ToStd();
            file.close();
            
            return Result<bool>::Success(true);
        } catch (const std::exception& e) {
            return Result<bool>::Error("Failed to save session: " + std::string(e.what()), "SESSION_SAVE_ERROR");
        }
    }
    
    Result<std::vector<SessionMetadata>> ListSessions() override {
        try {
            std::vector<SessionMetadata> sessions;
            
            if (!fs::exists(sessions_dir_)) {
                return Result<std::vector<SessionMetadata>>::Success(sessions);
            }
            
            for (const auto& entry : fs::directory_iterator(sessions_dir_)) {
                if (entry.is_directory()) {
                    // Extract session ID from directory name
                    std::string dir_name = entry.path().filename().string();
                    
                    // Check if it's a valid numeric directory name
                    if (std::all_of(dir_name.begin(), dir_name.end(), ::isdigit)) {
                        int session_id = std::stoi(dir_name);
                        
                        // Load this session's metadata
                        auto result = LoadSession(session_id);
                        if (result.success) {
                            sessions.push_back(result.value);
                        }
                    }
                }
            }
            
            // Sort sessions by ID
            std::sort(sessions.begin(), sessions.end(), 
                     [](const SessionMetadata& a, const SessionMetadata& b) {
                         return a.session_id < b.session_id;
                     });
            
            return Result<std::vector<SessionMetadata>>::Success(sessions);
        } catch (const std::exception& e) {
            return Result<std::vector<SessionMetadata>>::Error("Failed to list sessions: " + std::string(e.what()), 
                                                              "SESSION_LIST_ERROR");
        }
    }
    
    Result<bool> DeleteSession(int session_id) override {
        try {
            fs::path session_dir = sessions_dir_ / std::to_string(session_id);
            
            if (!fs::exists(session_dir)) {
                return Result<bool>::Error("Session directory does not exist", "SESSION_NOT_FOUND");
            }
            
            // Remove the entire session directory and all its contents
            fs::remove_all(session_dir);
            
            return Result<bool>::Success(true);
        } catch (const std::exception& e) {
            return Result<bool>::Error("Failed to delete session: " + std::string(e.what()), "SESSION_DELETION_ERROR");
        }
    }
    
    Result<bool> UpdateSessionState(int session_id, SessionState state) override {
        auto result = LoadSession(session_id);
        if (!result.success) {
            return Result<bool>::Error(result.error, result.error_code);
        }
        
        SessionMetadata metadata = result.value;
        metadata.state = state;
        
        return SaveSession(metadata);
    }
    
    Result<bool> UpdateSessionTicks(int session_id, int ticks) override {
        auto result = LoadSession(session_id);
        if (!result.success) {
            return Result<bool>::Error(result.error, result.error_code);
        }
        
        SessionMetadata metadata = result.value;
        metadata.total_ticks = ticks;
        
        return SaveSession(metadata);
    }

private:
    std::string workspace_path_;
    fs::path sessions_dir_;
    
    int getNextAvailableId() {
        int id = 1;
        
        // Find the next available ID by checking existing directories
        std::vector<int> existing_ids;
        
        if (fs::exists(sessions_dir_)) {
            for (const auto& entry : fs::directory_iterator(sessions_dir_)) {
                if (entry.is_directory()) {
                    std::string dir_name = entry.path().filename().string();
                    
                    // Check if it's a valid numeric directory name
                    if (std::all_of(dir_name.begin(), dir_name.end(), ::isdigit)) {
                        existing_ids.push_back(std::stoi(dir_name));
                    }
                }
            }
            
            std::sort(existing_ids.begin(), existing_ids.end());
            
            // Find the first gap in the sequence, or assign to next after the highest
            for (int existing_id : existing_ids) {
                if (existing_id == id) {
                    id++;
                } else if (existing_id > id) {
                    break;  // Found a gap or we're past where we need to be
                }
            }
        }
        
        return id;
    }
};

// Factory function to create the filesystem session store
std::unique_ptr<ISessionStore> CreateFilesystemSessionStore(const std::string& workspace_path) {
    return std::make_unique<JsonFilesystemSessionStore>(workspace_path);
}

} // namespace ProtoVMCLI