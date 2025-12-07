#ifndef _ProtoVM_CommandDispatcher_h_
#define _ProtoVM_CommandDispatcher_h_

#include "SessionTypes.h"
#include "SessionStore.h"
#include "ProtoVM.h"
#include <memory>
#include <string>

namespace ProtoVMCLI {

class CommandDispatcher {
public:
    CommandDispatcher(std::unique_ptr<ISessionStore> store);
    
    Upp::String RunInitWorkspace(const CommandOptions& opts);
    Upp::String RunCreateSession(const CommandOptions& opts);
    Upp::String RunListSessions(const CommandOptions& opts);
    Upp::String RunRunTicks(const CommandOptions& opts);
    Upp::String RunGetState(const CommandOptions& opts);
    Upp::String RunExportNetlist(const CommandOptions& opts);
    Upp::String RunDestroySession(const CommandOptions& opts);

    // Streaming debug commands
    Upp::String RunDebugProcessLogs(int process_id);
    Upp::String RunDebugWebSocketStream(const Upp::String& ws_id);
    Upp::String RunDebugPollStream(const Upp::String& poll_id);
    
private:
    std::unique_ptr<ISessionStore> session_store_;
    
    // Helper to validate workspace path
    bool ValidateWorkspace(const std::string& workspace_path);
    
    // Helper to get the next available session ID
    int GetNextSessionId();
};

} // namespace ProtoVMCLI

#endif