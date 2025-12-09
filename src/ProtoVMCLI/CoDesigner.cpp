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

Result<DesignerRetimeApplyResponse> CoDesignerManager::ApplyRetimeDesign(const DesignerRetimeApplyRequest& request) {
    // Get the designer session first
    auto session_result = GetSession(request.designer_session_id);
    if (!session_result.ok()) {
        return Result<DesignerRetimeApplyResponse>::MakeError(
            session_result.error_code(),
            session_result.error_message()
        );
    }

    CoDesignerSessionState session = session_result.data();

    // Load the ProtoVM session metadata using the circuit facade
    auto load_session_result = circuit_facade_->session_store_->LoadSession(session.proto_session_id);
    if (!load_session_result.ok()) {
        return Result<DesignerRetimeApplyResponse>::MakeError(
            load_session_result.error_code(),
            load_session_result.error_message()
        );
    }

    SessionMetadata metadata = load_session_result.data();
    std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

    DesignerRetimeApplyResponse response;
    response.designer_session = session;

    // Create application options from the request
    RetimingApplicationOptions app_options;
    app_options.apply_only_safe_moves = request.apply_only_safe;
    app_options.allow_suspicious_moves = request.allow_suspicious;
    app_options.max_moves = request.max_moves;

    // Get the retiming plans and find the specific one to apply
    if (request.target == "block") {
        // Get all retiming plans for the block to find the specific plan
        auto plans_result = circuit_facade_->AnalyzeRetimingForBlockInBranch(
            metadata, session_dir, session.branch, session.current_block_id);

        if (!plans_result.ok()) {
            return Result<DesignerRetimeApplyResponse>::MakeError(
                plans_result.error_code(),
                plans_result.error_message()
            );
        }

        // Find the specific plan by ID
        RetimingPlan* target_plan = nullptr;
        for (auto& plan : plans_result.data()) {
            if (plan.id == request.plan_id) {
                target_plan = &plan;
                break;
            }
        }

        if (!target_plan) {
            return Result<DesignerRetimeApplyResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "Retiming plan not found: " + request.plan_id
            );
        }

        // Apply the retiming plan
        auto application_result = circuit_facade_->ApplyRetimingPlanForBlockInBranch(
            metadata, session_dir, session.branch, *target_plan, app_options);

        if (!application_result.ok()) {
            return Result<DesignerRetimeApplyResponse>::MakeError(
                application_result.error_code(),
                application_result.error_message()
            );
        }

        response.application_result = application_result.data();

    } else if (request.target == "subsystem") {
        // Get all retiming plans for the subsystem to find the specific plan
        // For this, we'll need the block IDs that were used originally
        // For simplicity, let's assume we use the current block from the session state
        // Or we might need to pass them in the request too
        Vector<String> block_ids;
        if (!session.current_block_id.empty()) {
            block_ids.Add(Upp::String(session.current_block_id.c_str()));
        }

        auto plans_result = circuit_facade_->AnalyzeRetimingForSubsystemInBranch(
            metadata, session_dir, session.branch, session.current_block_id, block_ids);

        if (!plans_result.ok()) {
            return Result<DesignerRetimeApplyResponse>::MakeError(
                plans_result.error_code(),
                plans_result.error_message()
            );
        }

        // Find the specific plan by ID
        RetimingPlan* target_plan = nullptr;
        for (auto& plan : plans_result.data()) {
            if (plan.id == request.plan_id) {
                target_plan = &plan;
                break;
            }
        }

        if (!target_plan) {
            return Result<DesignerRetimeApplyResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "Retiming plan not found: " + request.plan_id
            );
        }

        // Apply the retiming plan
        auto application_result = circuit_facade_->ApplyRetimingPlanForSubsystemInBranch(
            metadata, session_dir, session.branch, *target_plan, app_options);

        if (!application_result.ok()) {
            return Result<DesignerRetimeApplyResponse>::MakeError(
                application_result.error_code(),
                application_result.error_message()
            );
        }

        response.application_result = application_result.data();

    } else {
        return Result<DesignerRetimeApplyResponse>::MakeError(
            ErrorCode::InvalidArgument,
            "target must be either 'block' or 'subsystem'"
        );
    }

    // Update the session state as necessary
    auto update_result = UpdateSession(response.designer_session);
    if (!update_result.ok()) {
        return Result<DesignerRetimeApplyResponse>::MakeError(
            update_result.error_code(),
            update_result.error_message()
        );
    }

    return Result<DesignerRetimeApplyResponse>::MakeOk(response);
}

} // namespace ProtoVMCLI