#include "CoDesigner.h"
#include "JsonIO.h"
#include "SessionStore.h"
#include "EngineFacade.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>

namespace ProtoVMCLI {

std::string CoDesignerManager::GenerateDesignerSessionId() {
    // Generate a unique ID in format "cd-<random_hex>"
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto seed = duration.count();
    
    std::mt19937 gen(seed);
    std::uniform_int_distribution<uint32_t> dis;
    
    uint32_t random_val = dis(gen);
    
    std::stringstream ss;
    ss << "cd-" << std::hex << std::setfill('0') << std::setw(8) << random_val;
    return ss.str();
}

Result<CoDesignerSessionState> CoDesignerManager::CreateSession(int proto_session_id, const std::string& branch) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    // Check if the proto session exists and is valid using the facade
    // For now, we'll just accept the parameters as-is and create the session
    CoDesignerSessionState new_session;
    new_session.designer_session_id = GenerateDesignerSessionId();
    new_session.proto_session_id = proto_session_id;
    new_session.branch = branch.empty() ? "main" : branch;
    new_session.use_optimized_ir = false;
    
    sessions_[new_session.designer_session_id] = new_session;
    
    return Result<CoDesignerSessionState>::MakeOk(new_session);
}

Result<CoDesignerSessionState> CoDesignerManager::GetSession(const std::string& designer_session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(designer_session_id);
    if (it == sessions_.end()) {
        return Result<CoDesignerSessionState>::MakeError(
            ErrorCode::SessionNotFound, 
            "Designer session not found: " + designer_session_id
        );
    }
    
    return Result<CoDesignerSessionState>::MakeOk(it->second);
}

Result<void> CoDesignerManager::UpdateSession(const CoDesignerSessionState& updated) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(updated.designer_session_id);
    if (it == sessions_.end()) {
        return Result<void>::MakeError(
            ErrorCode::SessionNotFound, 
            "Designer session not found: " + updated.designer_session_id
        );
    }
    
    sessions_[updated.designer_session_id] = updated;
    return Result<void>::MakeOk();
}

Result<void> CoDesignerManager::DestroySession(const std::string& designer_session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(designer_session_id);
    if (it == sessions_.end()) {
        return Result<void>::MakeError(
            ErrorCode::SessionNotFound, 
            "Designer session not found: " + designer_session_id
        );
    }
    
    sessions_.erase(it);
    return Result<void>::MakeOk();
}

} // namespace ProtoVMCLI