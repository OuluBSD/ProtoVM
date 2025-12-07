#include "EventLogger.h"
#include <filesystem>
#include <fstream>
#include <chrono>
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

bool EventLogger::LogEvent(const std::string& session_dir, const EventLogEntry& entry) {
    try {
        // Create the events log file path
        std::string events_file = session_dir + "/events.log";
        
        // Open the events log file in append mode
        std::ofstream file(events_file, std::ios::app);
        if (!file.is_open()) {
            return false;
        }
        
        // Format the event as a JSON line
        std::string json_line = FormatEventAsJson(entry);
        
        // Write the event to the file
        file << json_line << std::endl;
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string EventLogger::FormatEventAsJson(const EventLogEntry& entry) {
    std::string json = "{";
    json += "\"timestamp\":\"" + entry.timestamp + "\",";
    json += "\"user_id\":\"" + entry.user_id + "\",";
    json += "\"session_id\":" + std::to_string(entry.session_id) + ",";
    json += "\"command\":\"" + entry.command + "\",";
    json += "\"params\":" + entry.params + ",";
    json += "\"result\":" + entry.result;
    json += "}";
    
    return json;
}

} // namespace ProtoVMCLI