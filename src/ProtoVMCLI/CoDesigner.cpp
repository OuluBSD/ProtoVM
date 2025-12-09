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

Result<DesignerRetimeResponse> CoDesignerManager::RetimeDesign(const DesignerRetimeRequest& request) {
    // Get the designer session first
    auto session_result = GetSession(request.designer_session_id);
    if (!session_result.ok()) {
        return Result<DesignerRetimeResponse>::MakeError(
            session_result.error_code(),
            session_result.error_message()
        );
    }

    CoDesignerSessionState session = session_result.data();

    // Load the ProtoVM session metadata using the circuit facade
    auto load_session_result = circuit_facade_->session_store_->LoadSession(session.proto_session_id);
    if (!load_session_result.ok()) {
        return Result<DesignerRetimeResponse>::MakeError(
            load_session_result.error_code(),
            load_session_result.error_message()
        );
    }

    SessionMetadata metadata = load_session_result.data();
    std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

    DesignerRetimeResponse response;
    response.designer_session = session;

    // Perform retiming analysis based on the target type
    if (request.target == "block") {
        if (request.block_id.empty()) {
            return Result<DesignerRetimeResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "block_id is required when target is 'block'"
            );
        }

        // Call the circuit facade to perform retiming analysis for the block
        auto retiming_result = circuit_facade_->AnalyzeRetimingForBlockInBranch(
            metadata, session_dir, session.branch, request.block_id);

        if (!retiming_result.ok()) {
            return Result<DesignerRetimeResponse>::MakeError(
                retiming_result.error_code(),
                retiming_result.error_message()
            );
        }

        // Filter results based on min_depth if specified
        for (const auto& plan : retiming_result.data()) {
            if (plan.estimated_max_depth_before >= request.min_depth) {
                response.retiming_plans.push_back(plan);
            }
        }

        // Limit number of plans if specified
        if (request.max_plans > 0 && static_cast<int>(response.retiming_plans.size()) > request.max_plans) {
            response.retiming_plans.resize(request.max_plans);
        }
    } else if (request.target == "subsystem") {
        if (request.subsystem_id.empty() || request.block_ids.empty()) {
            return Result<DesignerRetimeResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "subsystem_id and block_ids are required when target is 'subsystem'"
            );
        }

        // Convert vector to Vector<String> for the facade method
        Vector<String> block_ids;
        for (const auto& id : request.block_ids) {
            block_ids.Add(Upp::String(id.c_str()));
        }

        // Call the circuit facade to perform retiming analysis for the subsystem
        auto retiming_result = circuit_facade_->AnalyzeRetimingForSubsystemInBranch(
            metadata, session_dir, session.branch, request.subsystem_id, block_ids);

        if (!retiming_result.ok()) {
            return Result<DesignerRetimeResponse>::MakeError(
                retiming_result.error_code(),
                retiming_result.error_message()
            );
        }

        // Filter results based on min_depth if specified
        for (const auto& plan : retiming_result.data()) {
            if (plan.estimated_max_depth_before >= request.min_depth) {
                response.retiming_plans.push_back(plan);
            }
        }

        // Limit number of plans if specified
        if (request.max_plans > 0 && static_cast<int>(response.retiming_plans.size()) > request.max_plans) {
            response.retiming_plans.resize(request.max_plans);
        }
    } else {
        return Result<DesignerRetimeResponse>::MakeError(
            ErrorCode::InvalidArgument,
            "target must be either 'block' or 'subsystem'"
        );
    }

    return Result<DesignerRetimeResponse>::MakeOk(response);
}

} // namespace ProtoVMCLI