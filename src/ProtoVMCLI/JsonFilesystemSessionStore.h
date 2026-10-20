#ifndef _ProtoVM_JsonFilesystemSessionStore_h_
#define _ProtoVM_JsonFilesystemSessionStore_h_

// NOTE: The implementation of this class lives, fully defined in-class, in
// JsonFilesystemSessionStore.cpp. This header mirrors its public interface
// and data members so client TUs (SessionServer etc.) can construct and use
// the concrete store; private helper methods defined in-class in the .cpp do
// not need to be repeated here.

#include "SessionStore.h"
#include <string>
#include <filesystem>
#include <memory>

namespace ProtoVMCLI {

class JsonFilesystemSessionStore : public ISessionStore {
public:
    explicit JsonFilesystemSessionStore(const std::string& workspace_path);

    Result<int> CreateSession(const SessionCreateInfo& info) override;
    Result<SessionMetadata> LoadSession(int session_id) override;
    Result<bool> SaveSession(const SessionMetadata& metadata) override;
    Result<ListSessionsResult> ListSessions() override;
    Result<bool> DeleteSession(int session_id) override;
    Result<bool> UpdateSessionState(int session_id, SessionState state) override;
    Result<bool> UpdateSessionTicks(int session_id, int ticks) override;

private:
    std::string workspace_path_;
    std::filesystem::path sessions_dir_;
};

// Factory function to create the filesystem session store (defined in
// JsonFilesystemSessionStore.cpp).
std::unique_ptr<ISessionStore> CreateFilesystemSessionStore(const std::string& workspace_path);

} // namespace ProtoVMCLI

#endif // _ProtoVM_JsonFilesystemSessionStore_h_
