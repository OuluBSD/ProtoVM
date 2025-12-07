#ifndef _ProtoVM_SessionStore_h_
#define _ProtoVM_SessionStore_h_

#include "SessionTypes.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

// Abstract interface for session storage
class ISessionStore {
public:
    virtual ~ISessionStore() = default;
    
    virtual Result<int> CreateSession(const SessionCreateInfo& info) = 0;
    virtual Result<SessionMetadata> LoadSession(int session_id) = 0;
    virtual Result<bool> SaveSession(const SessionMetadata& metadata) = 0;
    virtual Result<std::vector<SessionMetadata>> ListSessions() = 0;
    virtual Result<bool> DeleteSession(int session_id) = 0;
    virtual Result<bool> UpdateSessionState(int session_id, SessionState state) = 0;
    virtual Result<bool> UpdateSessionTicks(int session_id, int ticks) = 0;
};

} // namespace ProtoVMCLI

#endif