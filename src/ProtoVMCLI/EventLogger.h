#ifndef _ProtoVM_EventLogger_h_
#define _ProtoVM_EventLogger_h_

#include "SessionTypes.h"
#include <string>
#include <fstream>

namespace ProtoVMCLI {

struct EventLogEntry {
    std::string timestamp;
    std::string user_id;
    int session_id;
    std::string command;
    std::string params;  // JSON string
    std::string result;  // JSON string
    std::string branch;  // Branch name for this event

    EventLogEntry() : session_id(0) {}
};

class EventLogger {
public:
    static bool LogEvent(const std::string& session_dir, const EventLogEntry& entry);
    
private:
    static std::string FormatEventAsJson(const EventLogEntry& entry);
};

} // namespace ProtoVMCLI

#endif