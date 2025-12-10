#include "CoDesigner.h"
#include "JsonIO.h"
#include "SessionStore.h"
#include "EngineFacade.h"
#include "CodegenIr.h"
#include "CodegenIrInference.h"
#include "CodeEmitter.h"
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

Result<DesignerRetimeOptResponse> CoDesignerManager::OptimizeRetimeDesign(const DesignerRetimeOptRequest& request) {
    // Get the designer session first
    auto session_result = GetSession(request.designer_session_id);
    if (!session_result.ok()) {
        return Result<DesignerRetimeOptResponse>::MakeError(
            session_result.error_code(),
            session_result.error_message()
        );
    }

    CoDesignerSessionState session = session_result.data();

    // Load the ProtoVM session metadata using the circuit facade
    auto load_session_result = circuit_facade_->session_store_->LoadSession(session.proto_session_id);
    if (!load_session_result.ok()) {
        return Result<DesignerRetimeOptResponse>::MakeError(
            load_session_result.error_code(),
            load_session_result.error_message()
        );
    }

    SessionMetadata metadata = load_session_result.data();
    std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

    DesignerRetimeOptResponse response;
    response.designer_session = session;

    // Create application options from the request if apply is true
    RetimingApplicationOptions* app_options = nullptr;
    std::unique_ptr<RetimingApplicationOptions> app_opts_holder;
    if (request.apply) {
        app_opts_holder = std::make_unique<RetimingApplicationOptions>();
        app_opts_holder->apply_only_safe_moves = request.apply_only_safe;
        app_opts_holder->allow_suspicious_moves = request.allow_suspicious;
        app_options = app_opts_holder.get();
    }

    // Perform retiming optimization based on the target type
    if (request.target == "block") {
        if (request.block_id.empty()) {
            return Result<DesignerRetimeOptResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "block_id is required when target is 'block'"
            );
        }

        // Call the circuit facade to perform retiming optimization for the block
        auto result = circuit_facade_->OptimizeRetimingForBlockInBranch(
            metadata, session_dir, session.branch, request.block_id,
            request.objective, app_options);

        if (!result.ok()) {
            return Result<DesignerRetimeOptResponse>::MakeError(
                result.error_code(),
                result.error_message()
            );
        }

        response.optimization_result = result.data();

    } else if (request.target == "subsystem") {
        if (request.subsystem_id.empty() || request.block_ids.empty()) {
            return Result<DesignerRetimeOptResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "subsystem_id and block_ids are required when target is 'subsystem'"
            );
        }

        // Convert vector to Vector<String> for the facade method
        Vector<String> block_ids;
        for (const auto& id : request.block_ids) {
            block_ids.Add(Upp::String(id.c_str()));
        }

        // Call the circuit facade to perform retiming optimization for the subsystem
        auto result = circuit_facade_->OptimizeRetimingForSubsystemInBranch(
            metadata, session_dir, session.branch, request.subsystem_id,
            block_ids, request.objective, app_options);

        if (!result.ok()) {
            return Result<DesignerRetimeOptResponse>::MakeError(
                result.error_code(),
                result.error_message()
            );
        }

        response.optimization_result = result.data();

    } else {
        return Result<DesignerRetimeOptResponse>::MakeError(
            ErrorCode::InvalidArgument,
            "target must be either 'block' or 'subsystem'"
        );
    }

    // Update the session state as necessary
    auto update_result = UpdateSession(response.designer_session);
    if (!update_result.ok()) {
        return Result<DesignerRetimeOptResponse>::MakeError(
            update_result.error_code(),
            update_result.error_message()
        );
    }

    return Result<DesignerRetimeOptResponse>::MakeOk(response);
}

Result<DesignerCodegenBlockCResponse> CoDesignerManager::CodegenBlockC(const DesignerCodegenBlockCRequest& request) {
    // Get the designer session first
    auto session_result = GetSession(request.designer_session_id);
    if (!session_result.ok()) {
        return Result<DesignerCodegenBlockCResponse>::MakeError(
            session_result.error_code(),
            session_result.error_message()
        );
    }

    CoDesignerSessionState session = session_result.data();

    // Load the ProtoVM session metadata using the circuit facade
    auto load_session_result = circuit_facade_->session_store_->LoadSession(session.proto_session_id);
    if (!load_session_result.ok()) {
        return Result<DesignerCodegenBlockCResponse>::MakeError(
            load_session_result.error_code(),
            load_session_result.error_message()
        );
    }

    SessionMetadata metadata = load_session_result.data();
    std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

    DesignerCodegenBlockCResponse response;
    response.designer_session = session;

    // Generate C/C++ code for the block
    auto code_result = circuit_facade_->EmitCodeForBlockInBranch(
        metadata,
        session_dir,
        session.branch,
        request.block_id,
        request.lang,
        request.emit_state_struct,
        request.state_struct_name,
        request.function_name
    );

    if (!code_result.ok()) {
        return Result<DesignerCodegenBlockCResponse>::MakeError(
            code_result.error_code(),
            code_result.error_message()
        );
    }

    // Build the response
    response.result.block_id = request.block_id;
    response.result.lang = request.lang;
    response.result.code = code_result.data;
    response.result.state_struct_name = request.state_struct_name;
    response.result.function_name = request.function_name;

    // Update the session state as necessary
    auto update_result = UpdateSession(response.designer_session);
    if (!update_result.ok()) {
        return Result<DesignerCodegenBlockCResponse>::MakeError(
            update_result.error_code(),
            update_result.error_message()
        );
    }

    return Result<DesignerCodegenBlockCResponse>::MakeOk(response);
}

Result<DesignerCodegenOscDemoResponse> CoDesignerManager::CodegenOscDemo(const DesignerCodegenOscDemoRequest& request) {
    // Get the designer session first
    auto session_result = GetSession(request.designer_session_id);
    if (!session_result.ok()) {
        return Result<DesignerCodegenOscDemoResponse>::MakeError(
            session_result.error_code(),
            session_result.error_message()
        );
    }

    CoDesignerSessionState session = session_result.data();

    // Load the ProtoVM session metadata using the circuit facade
    auto load_session_result = circuit_facade_->session_store_->LoadSession(session.proto_session_id);
    if (!load_session_result.ok()) {
        return Result<DesignerCodegenOscDemoResponse>::MakeError(
            load_session_result.error_code(),
            load_session_result.error_message()
        );
    }

    SessionMetadata metadata = load_session_result.data();
    std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

    DesignerCodegenOscDemoResponse response;
    response.designer_session = session;

    // Generate oscillator demo code for the block
    auto code_result = circuit_facade_->EmitOscillatorDemoForBlockInBranch(
        metadata,
        session_dir,
        session.branch,
        request.block_id,
        request.lang
    );

    if (!code_result.ok()) {
        return Result<DesignerCodegenOscDemoResponse>::MakeError(
            code_result.error_code(),
            code_result.error_message()
        );
    }

    // Build the response
    response.result.block_id = request.block_id;
    response.result.lang = request.lang;
    response.result.osc_code = code_result.data;

    // Update the session state as necessary
    auto update_result = UpdateSession(response.designer_session);
    if (!update_result.ok()) {
        return Result<DesignerCodegenOscDemoResponse>::MakeError(
            update_result.error_code(),
            update_result.error_message()
        );
    }

    return Result<DesignerCodegenOscDemoResponse>::MakeOk(response);
}

Result<DesignerGlobalPipelineResponse> CoDesignerManager::AnalyzeGlobalPipeline(const DesignerGlobalPipelineRequest& request) {
    // Get the designer session first
    auto session_result = GetSession(request.designer_session_id);
    if (!session_result.ok()) {
        return Result<DesignerGlobalPipelineResponse>::MakeError(
            session_result.error_code(),
            session_result.error_message()
        );
    }

    CoDesignerSessionState session = session_result.data();

    // Load the ProtoVM session metadata using the circuit facade
    auto load_session_result = circuit_facade_->session_store_->LoadSession(session.proto_session_id);
    if (!load_session_result.ok()) {
        return Result<DesignerGlobalPipelineResponse>::MakeError(
            load_session_result.error_code(),
            load_session_result.error_message()
        );
    }

    SessionMetadata metadata = load_session_result.data();
    std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

    DesignerGlobalPipelineResponse response;
    response.designer_session = session;

    if (request.target == "subsystem") {
        if (request.subsystem_id.empty() || request.block_ids.empty()) {
            return Result<DesignerGlobalPipelineResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "subsystem_id and block_ids are required when target is 'subsystem'"
            );
        }

        // Convert vector to Vector<String> for the facade method
        Vector<String> block_ids;
        for (const auto& id : request.block_ids) {
            block_ids.Add(Upp::String(id.c_str()));
        }

        if (request.analyze_only) {
            // Call the circuit facade to perform global pipeline analysis for the subsystem
            auto result = circuit_facade_->BuildGlobalPipelineMapForSubsystemInBranch(
                metadata, session_dir, session.branch,
                String(request.subsystem_id.c_str()), block_ids);

            if (!result.ok()) {
                return Result<DesignerGlobalPipelineResponse>::MakeError(
                    result.error_code(),
                    result.error_message()
                );
            }

            response.global_pipeline = result.value();
        } else {
            // For now, analyze only mode is the default; we can extend this to include optimization proposals
            return Result<DesignerGlobalPipelineResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "analyze_only mode does not support optimization proposals in this call - use OptimizeGlobalPipeline"
            );
        }
    } else {
        return Result<DesignerGlobalPipelineResponse>::MakeError(
            ErrorCode::InvalidArgument,
            "target must be 'subsystem' for global pipelining"
        );
    }

    // Update the session state as necessary
    auto update_result = UpdateSession(response.designer_session);
    if (!update_result.ok()) {
        return Result<DesignerGlobalPipelineResponse>::MakeError(
            update_result.error_code(),
            update_result.error_message()
        );
    }

    return Result<DesignerGlobalPipelineResponse>::MakeOk(response);
}

Result<DesignerGlobalPipelineResponse> CoDesignerManager::OptimizeGlobalPipeline(const DesignerGlobalPipelineOptRequest& request) {
    // Get the designer session first
    auto session_result = GetSession(request.designer_session_id);
    if (!session_result.ok()) {
        return Result<DesignerGlobalPipelineResponse>::MakeError(
            session_result.error_code(),
            session_result.error_message()
        );
    }

    CoDesignerSessionState session = session_result.data();

    // Load the ProtoVM session metadata using the circuit facade
    auto load_session_result = circuit_facade_->session_store_->LoadSession(session.proto_session_id);
    if (!load_session_result.ok()) {
        return Result<DesignerGlobalPipelineResponse>::MakeError(
            load_session_result.error_code(),
            load_session_result.error_message()
        );
    }

    SessionMetadata metadata = load_session_result.data();
    std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

    DesignerGlobalPipelineResponse response;
    response.designer_session = session;

    if (request.target == "subsystem") {
        if (request.subsystem_id.empty() || request.block_ids.empty()) {
            return Result<DesignerGlobalPipelineResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "subsystem_id and block_ids are required when target is 'subsystem'"
            );
        }

        // Convert vector to Vector<String> for the facade method
        Vector<String> block_ids;
        for (const auto& id : request.block_ids) {
            block_ids.Add(Upp::String(id.c_str()));
        }

        // Call the circuit facade to propose global pipelining plans for the subsystem
        auto result = circuit_facade_->ProposeGlobalPipeliningPlansInBranch(
            metadata, session_dir, session.branch,
            String(request.subsystem_id.c_str()), block_ids, request.objective);

        if (!result.ok()) {
            return Result<DesignerGlobalPipelineResponse>::MakeError(
                result.error_code(),
                result.error_message()
            );
        }

        response.global_plans = result.value();

        // Optionally apply the best plan if requested
        if (request.apply && !result.value().IsEmpty()) {
            // Create application options from the request
            RetimingApplicationOptions app_options;
            app_options.apply_only_safe_moves = request.apply_only_safe;
            app_options.allow_suspicious_moves = request.allow_suspicious;

            // Apply the first (best) plan
            const GlobalPipeliningPlan& plan_to_apply = result.value()[0];
            auto apply_result = circuit_facade_->ApplyGlobalPipeliningPlanInBranch(
                metadata, session_dir, session.branch, plan_to_apply, app_options);

            if (!apply_result.ok()) {
                return Result<DesignerGlobalPipelineResponse>::MakeError(
                    apply_result.error_code(),
                    apply_result.error_message()
                );
            }
        }
    } else {
        return Result<DesignerGlobalPipelineResponse>::MakeError(
            ErrorCode::InvalidArgument,
            "target must be 'subsystem' for global pipelining"
        );
    }

    // Update the session state as necessary
    auto update_result = UpdateSession(response.designer_session);
    if (!update_result.ok()) {
        return Result<DesignerGlobalPipelineResponse>::MakeError(
            update_result.error_code(),
            update_result.error_message()
        );
    }

    return Result<DesignerGlobalPipelineResponse>::MakeOk(response);
}

Result<DesignerGlobalPipelineApplyResponse> CoDesignerManager::ApplyGlobalPipeline(const DesignerGlobalPipelineApplyRequest& request) {
    // Get the designer session first
    auto session_result = GetSession(request.designer_session_id);
    if (!session_result.ok()) {
        return Result<DesignerGlobalPipelineApplyResponse>::MakeError(
            session_result.error_code(),
            session_result.error_message()
        );
    }

    CoDesignerSessionState session = session_result.data();

    // Load the ProtoVM session metadata using the circuit facade
    auto load_session_result = circuit_facade_->session_store_->LoadSession(session.proto_session_id);
    if (!load_session_result.ok()) {
        return Result<DesignerGlobalPipelineApplyResponse>::MakeError(
            load_session_result.error_code(),
            load_session_result.error_message()
        );
    }

    SessionMetadata metadata = load_session_result.data();
    std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

    DesignerGlobalPipelineApplyResponse response;
    response.designer_session = session;

    // In a full implementation, we would retrieve the plan by ID from a persistent store
    // For now, we'll create a dummy plan with the given ID to pass to the apply function
    GlobalPipeliningPlan plan;
    plan.id = Upp::String(request.plan_id.c_str());

    // Create application options from the request
    RetimingApplicationOptions app_options;
    app_options.apply_only_safe_moves = request.apply_only_safe;
    app_options.allow_suspicious_moves = request.allow_suspicious;
    app_options.max_moves = request.max_moves;

    // Apply the global pipelining plan
    auto apply_result = circuit_facade_->ApplyGlobalPipeliningPlanInBranch(
        metadata, session_dir, session.branch, plan, app_options);

    if (!apply_result.ok()) {
        return Result<DesignerGlobalPipelineApplyResponse>::MakeError(
            apply_result.error_code(),
            apply_result.error_message()
        );
    }

    response.application_result = apply_result.value();

    // Update the session state as necessary
    auto update_result = UpdateSession(response.designer_session);
    if (!update_result.ok()) {
        return Result<DesignerGlobalPipelineApplyResponse>::MakeError(
            update_result.error_code(),
            update_result.error_message()
        );
    }

    return Result<DesignerGlobalPipelineApplyResponse>::MakeOk(response);
}

Result<DesignerStructAnalyzeResponse> CoDesignerManager::AnalyzeStructural(const DesignerStructAnalyzeRequest& request) {
    try {
        // Get the session
        auto session_result = GetSession(request.designer_session_id);
        if (!session_result.ok) {
            return Result<DesignerStructAnalyzeResponse>::MakeError(
                session_result.error_code(),
                session_result.error_message()
            );
        }
        CoDesignerSessionState session = session_result.data();

        // Validate request
        if (request.target != "block") {
            return Result<DesignerStructAnalyzeResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "Structural analysis target must be 'block'"
            );
        }

        if (request.block_id.empty()) {
            return Result<DesignerStructAnalyzeResponse>::MakeError(
                ErrorCode::InvalidArgument,
                "Block ID is required for structural analysis"
            );
        }

        // Get the session metadata to pass to the circuit facade
        SessionStore store;
        auto metadata_result = store.LoadSession(session.proto_session_id);
        if (!metadata_result.ok) {
            return Result<DesignerStructAnalyzeResponse>::MakeError(
                metadata_result.error_code,
                metadata_result.error_message
            );
        }

        SessionMetadata metadata = metadata_result.data;
        std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

        DesignerStructAnalyzeResponse response;
        response.designer_session = session;

        // Perform the structural analysis
        auto analysis_result = circuit_facade_->AnalyzeBlockStructureInBranch(
            metadata,
            session_dir,
            session.branch,
            request.block_id
        );

        if (!analysis_result.ok) {
            return Result<DesignerStructAnalyzeResponse>::MakeError(
                analysis_result.error_code,
                analysis_result.error_message
            );
        }

        response.structural_refactor_plan = analysis_result.data;

        return Result<DesignerStructAnalyzeResponse>::MakeOk(response);
    } catch (const std::exception& e) {
        return Result<DesignerStructAnalyzeResponse>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in AnalyzeStructural: ") + e.what()
        );
    }
}

Result<DesignerStructApplyResponse> CoDesignerManager::ApplyStructural(const DesignerStructApplyRequest& request) {
    try {
        // Get the session
        auto session_result = GetSession(request.designer_session_id);
        if (!session_result.ok) {
            return Result<DesignerStructApplyResponse>::MakeError(
                session_result.error_code(),
                session_result.error_message()
            );
        }
        CoDesignerSessionState session = session_result.data();

        // Get the session metadata to pass to the circuit facade
        SessionStore store;
        auto metadata_result = store.LoadSession(session.proto_session_id);
        if (!metadata_result.ok) {
            return Result<DesignerStructApplyResponse>::MakeError(
                metadata_result.error_code,
                metadata_result.error_message
            );
        }

        SessionMetadata metadata = metadata_result.data;
        std::string session_dir = metadata.workspace_path + "/sessions/" + std::to_string(session.proto_session_id);

        DesignerStructApplyResponse response;
        response.designer_session = session;

        // In a full implementation, we would retrieve the plan by ID from a persistent store
        // For this implementation, we'll analyze the same block again to get the plan with the matching ID
        // This is a simplification for this implementation

        // Since we don't have a separate plan store, we'll need to somehow retrieve the plan
        // that was previously generated. For the purposes of this implementation, we'll simulate
        // this by creating a simple plan with the requested ID. In a real implementation,
        // we would have a persistent store for plans.

        // For now, we'll just create a dummy plan with the given ID to pass to the apply function
        StructuralRefactorPlan plan;
        plan.id = Upp::String(request.plan_id.c_str());
        plan.target_block_id = Upp::String("PLACEHOLDER_BLOCK"); // This would be filled in properly in a real implementation

        // Apply the structural refactor plan
        auto apply_result = circuit_facade_->ApplyStructuralRefactorPlanInBranch(
            metadata,
            session_dir,
            session.branch,
            plan,
            request.apply_only_safe
        );

        if (!apply_result.ok) {
            return Result<DesignerStructApplyResponse>::MakeError(
                apply_result.error_code(),
                apply_result.error_message()
            );
        }

        response.application_result = apply_result.data;

        return Result<DesignerStructApplyResponse>::MakeOk(response);
    } catch (const std::exception& e) {
        return Result<DesignerStructApplyResponse>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in ApplyStructural: ") + e.what()
        );
    }
}

Result<DesignerDspGraphInspectResponse> CoDesignerManager::InspectDspGraph(const DesignerDspGraphInspectRequest& request) {
    try {
        // Get the designer session
        auto session_result = GetSession(request.designer_session_id);
        if (!session_result.ok) {
            return Result<DesignerDspGraphInspectResponse>::MakeError(
                session_result.error_code,
                "Failed to get designer session: " + session_result.error_message
            );
        }

        CoDesignerSessionState session = session_result.data;

        // Get the current session metadata
        auto session_load_result = circuit_facade_->GetSessionStore()->LoadSession(session.proto_session_id);
        if (!session_load_result.ok) {
            return Result<DesignerDspGraphInspectResponse>::MakeError(
                session_load_result.error_code,
                "Failed to load ProtoVM session: " + session_load_result.error_message
            );
        }

        SessionMetadata proto_session = session_load_result.data;

        // Build the AudioDslGraph
        auto graph_result = circuit_facade_->BuildAudioDslForOscillatorBlockInBranch(
            proto_session,
            circuit_facade_->GetSessionStore()->GetWorkspacePath() + "/sessions/" + std::to_string(session.proto_session_id),
            session.branch,
            request.block_id,
            request.freq_hz,
            request.pan_lfo_hz,
            request.sample_rate,
            request.duration_sec
        );

        if (!graph_result.ok) {
            return Result<DesignerDspGraphInspectResponse>::MakeError(
                graph_result.error_code,
                "Failed to build Audio DSL for oscillator: " + graph_result.error_message
            );
        }

        // Build the DSP graph
        auto dsp_graph_result = circuit_facade_->BuildDspGraphForOscillatorBlockInBranch(
            proto_session,
            circuit_facade_->GetSessionStore()->GetWorkspacePath() + "/sessions/" + std::to_string(session.proto_session_id),
            session.branch,
            request.block_id,
            graph_result.data
        );

        if (!dsp_graph_result.ok) {
            return Result<DesignerDspGraphInspectResponse>::MakeError(
                dsp_graph_result.error_code,
                "Failed to build DSP graph: " + dsp_graph_result.error_message
            );
        }

        // Build the response
        DesignerDspGraphInspectResponse response;
        response.designer_session = session;
        response.dsp_graph = dsp_graph_result.data;

        return Result<DesignerDspGraphInspectResponse>::MakeOk(response);
    } catch (const std::exception& e) {
        return Result<DesignerDspGraphInspectResponse>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in InspectDspGraph: ") + e.what()
        );
    }
}

Result<DesignerDspRenderOscResponse> CoDesignerManager::RenderDspOsc(const DesignerDspRenderOscRequest& request) {
    try {
        // Get the designer session
        auto session_result = GetSession(request.designer_session_id);
        if (!session_result.ok) {
            return Result<DesignerDspRenderOscResponse>::MakeError(
                session_result.error_code,
                "Failed to get designer session: " + session_result.error_message
            );
        }

        CoDesignerSessionState session = session_result.data;

        // Get the current session metadata
        auto session_load_result = circuit_facade_->GetSessionStore()->LoadSession(session.proto_session_id);
        if (!session_load_result.ok) {
            return Result<DesignerDspRenderOscResponse>::MakeError(
                session_load_result.error_code,
                "Failed to load ProtoVM session: " + session_load_result.error_message
            );
        }

        SessionMetadata proto_session = session_load_result.data;

        // Build the AudioDslGraph
        auto graph_result = circuit_facade_->BuildAudioDslForOscillatorBlockInBranch(
            proto_session,
            circuit_facade_->GetSessionStore()->GetWorkspacePath() + "/sessions/" + std::to_string(session.proto_session_id),
            session.branch,
            request.block_id,
            request.freq_hz,
            request.pan_lfo_hz,
            request.sample_rate,
            request.duration_sec
        );

        if (!graph_result.ok) {
            return Result<DesignerDspRenderOscResponse>::MakeError(
                graph_result.error_code,
                "Failed to build Audio DSL for oscillator: " + graph_result.error_message
            );
        }

        // Render the DSP graph
        std::vector<float> out_left, out_right;
        auto render_result = circuit_facade_->RenderDspGraphForOscillatorBlockInBranch(
            proto_session,
            circuit_facade_->GetSessionStore()->GetWorkspacePath() + "/sessions/" + std::to_string(session.proto_session_id),
            session.branch,
            request.block_id,
            graph_result.data,
            out_left,
            out_right
        );

        if (!render_result.ok) {
            return Result<DesignerDspRenderOscResponse>::MakeError(
                render_result.error_code,
                "Failed to render DSP graph: " + render_result.error_message
            );
        }

        // Calculate statistics
        double left_rms = 0.0, right_rms = 0.0;
        double left_sum = 0.0, right_sum = 0.0;

        for (float sample : out_left) {
            left_sum += sample * sample;
        }
        if (!out_left.empty()) {
            left_rms = std::sqrt(left_sum / out_left.size());
        }

        for (float sample : out_right) {
            right_sum += sample * sample;
        }
        if (!out_right.empty()) {
            right_rms = std::sqrt(right_sum / out_right.size());
        }

        // Build the response
        DesignerDspRenderOscResponse response;
        response.designer_session = session;
        response.left_samples = out_left;
        response.right_samples = out_right;

        // Set up render stats
        response.render_stats.sample_rate_hz = request.sample_rate;
        response.render_stats.duration_sec = request.duration_sec;
        response.render_stats.left_rms = left_rms;
        response.render_stats.right_rms = right_rms;
        response.render_stats.left_min = out_left.empty() ? 0.0 : *std::min_element(out_left.begin(), out_left.end());
        response.render_stats.left_max = out_left.empty() ? 0.0 : *std::max_element(out_left.begin(), out_left.end());
        response.render_stats.right_min = out_right.empty() ? 0.0 : *std::min_element(out_right.begin(), out_right.end());
        response.render_stats.right_max = out_right.empty() ? 0.0 : *std::max_element(out_right.begin(), out_right.end());
        response.render_stats.total_samples = static_cast<int>(out_left.size());

        return Result<DesignerDspRenderOscResponse>::MakeOk(response);
    } catch (const std::exception& e) {
        return Result<DesignerDspRenderOscResponse>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in RenderDspOsc: ") + e.what()
        );
    }
}

    Result<DesignerAnalogModelInspectResponse> CoDesignerManager::InspectAnalogModel(const DesignerAnalogModelInspectRequest& request) {
        try {
            // Get the designer session
            auto session_result = GetSession(request.designer_session_id);
            if (!session_result.ok) {
                return Result<DesignerAnalogModelInspectResponse>::MakeError(
                    session_result.error_code,
                    session_result.error_message
                );
            }

            CoDesignerSessionState session = session_result.data;

            // Extract the analog model for the specified block
            auto model_result = circuit_facade_->ExtractAnalogModelForBlockInBranch(
                session.proto_session_id,
                "PLACEHOLDER_PATH",  // This would be resolved from the session
                session.branch,
                request.block_id
            );

            if (!model_result.ok) {
                return Result<DesignerAnalogModelInspectResponse>::MakeError(
                    model_result.error_code,
                    "Failed to extract analog model: " + model_result.error_message
                );
            }

            // Build the response
            DesignerAnalogModelInspectResponse response;
            response.designer_session = session;
            response.analog_model = model_result.data;

            return Result<DesignerAnalogModelInspectResponse>::MakeOk(response);
        } catch (const std::exception& e) {
            return Result<DesignerAnalogModelInspectResponse>::MakeError(
                ErrorCode::InternalError,
                std::string("Exception in InspectAnalogModel: ") + e.what()
            );
        }
    }

    Result<DesignerAnalogRenderOscResponse> CoDesignerManager::RenderAnalogOsc(const DesignerAnalogRenderOscRequest& request) {
        try {
            // Get the designer session
            auto session_result = GetSession(request.designer_session_id);
            if (!session_result.ok) {
                return Result<DesignerAnalogRenderOscResponse>::MakeError(
                    session_result.error_code,
                    session_result.error_message
                );
            }

            CoDesignerSessionState session = session_result.data;

            // Build the AudioDslGraph with parameters
            AudioDslGraph audio_graph;
            audio_graph.block_id = request.block_id;
            audio_graph.osc.id = "analog_osc_1";
            audio_graph.osc.frequency_hz = 440.0; // Will be overridden by analog model
            audio_graph.pan_lfo.id = "pan_lfo_1";
            audio_graph.pan_lfo.rate_hz = request.pan_lfo_hz;
            audio_graph.output.sample_rate_hz = request.sample_rate_hz;
            audio_graph.output.duration_sec = request.duration_sec;

            // Render the analog block as audio
            std::vector<float> out_left, out_right;
            auto render_result = circuit_facade_->RenderAnalogBlockAsAudioInBranch(
                session.proto_session_id,
                "PLACEHOLDER_PATH",  // This would be resolved from the session
                session.branch,
                request.block_id,
                audio_graph,
                out_left,
                out_right
            );

            if (!render_result.ok) {
                return Result<DesignerAnalogRenderOscResponse>::MakeError(
                    render_result.error_code,
                    "Failed to render analog oscillator: " + render_result.error_message
                );
            }

            // Calculate statistics
            double left_rms = 0.0, right_rms = 0.0;
            double left_sum = 0.0, right_sum = 0.0;

            for (float sample : out_left) {
                left_sum += sample * sample;
            }
            if (!out_left.empty()) {
                left_rms = std::sqrt(left_sum / out_left.size());
            }

            for (float sample : out_right) {
                right_sum += sample * sample;
            }
            if (!out_right.empty()) {
                right_rms = std::sqrt(right_sum / out_right.size());
            }

            // Get the analog model to extract the estimated frequency
            auto model_result = circuit_facade_->ExtractAnalogModelForBlockInBranch(
                session.proto_session_id,
                "PLACEHOLDER_PATH",  // This would be resolved from the session
                session.branch,
                request.block_id
            );

            double estimated_freq = -1.0; // Default if model extraction fails
            if (model_result.ok) {
                estimated_freq = model_result.data.estimated_freq_hz;
            }

            // Build the response
            DesignerAnalogRenderOscResponse response;
            response.designer_session = session;
            response.left_samples = out_left;
            response.right_samples = out_right;

            // Set up render stats
            response.render_stats.sample_rate_hz = request.sample_rate_hz;
            response.render_stats.duration_sec = request.duration_sec;
            response.render_stats.estimated_freq_hz = estimated_freq;
            response.render_stats.pan_lfo_hz = request.pan_lfo_hz;
            response.render_stats.left_rms = left_rms;
            response.render_stats.right_rms = right_rms;
            response.render_stats.left_min = out_left.empty() ? 0.0 : *std::min_element(out_left.begin(), out_left.end());
            response.render_stats.left_max = out_left.empty() ? 0.0 : *std::max_element(out_left.begin(), out_left.end());
            response.render_stats.right_min = out_right.empty() ? 0.0 : *std::min_element(out_right.begin(), out_right.end());
            response.render_stats.right_max = out_right.empty() ? 0.0 : *std::max_element(out_right.begin(), out_right.end());
            response.render_stats.total_samples = static_cast<int>(out_left.size());

            return Result<DesignerAnalogRenderOscResponse>::MakeOk(response);
        } catch (const std::exception& e) {
            return Result<DesignerAnalogRenderOscResponse>::MakeError(
                ErrorCode::InternalError,
                std::string("Exception in RenderAnalogOsc: ") + e.what()
            );
        }
    }

} // namespace ProtoVMCLI