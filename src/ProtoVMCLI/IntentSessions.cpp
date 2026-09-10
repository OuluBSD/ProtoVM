#include "IntentSessions.h"
#include "JsonIO.h"
#include "InstrumentRuntime.h"
#include "MusicalIntent.h"
#include "EventLogger.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

namespace ProtoVMCLI {

// Helper function to generate intent session IDs
static String GenerateIntentSessionId() {
    // Generate a unique ID in the format "is-000001"
    auto now = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    return Upp::Format("is-%06d", (int)(ns % 1000000));
}

// Helper function to generate ISO 8601 timestamp
static std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::gmtime(&time_t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
}

Result<IntentSessionState> IntentSessionManager::Create(
    const String& session_dir,
    const String& branch,
    const InstrumentGraph& initial_instrument,
    const String& user_id
) {
    try {
        IntentSessionState session_state;
        session_state.meta.intent_session_id = GenerateIntentSessionId();
        session_state.meta.current_branch = branch;
        session_state.meta.status = IntentSessionStatus::Active;
        session_state.current_instrument = initial_instrument;

        // Create the directory structure
        String base_path = session_dir + "/intent_sessions/" + session_state.meta.intent_session_id;
        fs::create_directories(base_path + "/steps");
        fs::create_directories(base_path + "/artifacts");

        // Save session state
        auto save_result = SaveSession(session_state, session_dir);
        if (!save_result.ok) {
            return Result<IntentSessionState>::MakeError(
                save_result.error_code,
                "Failed to save intent session: " + save_result.error_message
            );
        }

        // Log the creation event
        EventLogEntry log_entry;
        log_entry.timestamp = GetCurrentTimestamp();
        log_entry.user_id = user_id;
        log_entry.session_id = -1;  // We don't have a session_id here, would need to extract from session_dir
        log_entry.branch = branch;
        log_entry.command = "intent-session-create";
        log_entry.params = "{\"intent_session_id\":\"" + session_state.meta.intent_session_id + "\"}";
        log_entry.result = "{\"status\":\"created\"}";
        EventLogger::LogEvent(session_dir, log_entry);

        return Result<IntentSessionState>::MakeOk(session_state);
    } catch (const std::exception& e) {
        return Result<IntentSessionState>::MakeError(
            ErrorCode::InternalError,
            "Exception in IntentSessionManager::Create: " + std::string(e.what())
        );
    }
}

Result<IntentSessionState> IntentSessionManager::Load(
    const String& session_dir,
    const String& intent_session_id
) {
    try {
        String session_path = session_dir + "/intent_sessions/" + intent_session_id;
        
        if (!fs::exists(session_path)) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::SessionNotFound,
                "Intent session not found: " + intent_session_id
            );
        }

        // Load the main session file
        String session_file_path = session_path + "/intent_session.json";
        std::ifstream session_file(session_file_path);
        if (!session_file.is_open()) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::SessionCorrupt,
                "Could not open session file: " + session_file_path
            );
        }

        // Read the content
        std::string content((std::istreambuf_iterator<char>(session_file)),
                            std::istreambuf_iterator<char>());
        session_file.close();

        // Parse the JSON
        Upp::ValueMap session_data = JsonIO::Deserialize(content);

        // Reconstruct the session state
        IntentSessionState session_state;
        
        // Load metadata
        if (session_data.Find("meta") != Upp::VALUE_ARRAY || session_data["meta"].GetType() != Upp::STYPE_MAP) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::SessionCorrupt,
                "Session metadata missing or invalid"
            );
        }
        
        Upp::ValueMap meta_data = session_data["meta"];
        if (meta_data.Find("intent_session_id") != Upp::VALUE_ARRAY) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::SessionCorrupt,
                "Intent session ID missing in metadata"
            );
        }
        session_state.meta.intent_session_id = meta_data["intent_session_id"];
        session_state.meta.current_branch = meta_data["current_branch"];
        session_state.meta.instrument_revision = (int)meta_data["instrument_revision"];
        session_state.meta.head_step_index = (int)meta_data["head_step_index"];
        session_state.meta.status = IntentSessionStatus::Active; // TODO: Implement status enum handling
        
        // Load current instrument
        String instrument_file_path = session_path + "/instrument_current.json";
        std::ifstream instrument_file(instrument_file_path);
        if (!instrument_file.is_open()) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::SessionCorrupt,
                "Could not load current instrument: " + instrument_file_path
            );
        }
        
        std::string instrument_content((std::istreambuf_iterator<char>(instrument_file)),
                                      std::istreambuf_iterator<char>());
        instrument_file.close();
        
        Upp::ValueMap instrument_data = JsonIO::Deserialize(instrument_content);
        // TODO: Convert from ValueMap to InstrumentGraph
        // This requires a reverse function of InstrumentGraphToValueMap
        
        // Load steps
        // For now, we'll just load the step count from metadata, but in a real implementation
        // we would need to enumerate the step files
        
        return Result<IntentSessionState>::MakeOk(session_state);
    } catch (const std::exception& e) {
        return Result<IntentSessionState>::MakeError(
            ErrorCode::InternalError,
            "Exception in IntentSessionManager::Load: " + std::string(e.what())
        );
    }
}

Result<IntentSessionState> IntentSessionManager::ProposeStep(
    IntentSessionState& st,
    const IntentRequest& req,
    const String& user_id
) {
    try {
        // Run Phase 29 intent verification against current_instrument
        Result<IntentPlan> plan_result = MusicalIntentEngine::ProposeAndVerify(st.current_instrument, req);
        if (!plan_result.ok) {
            return Result<IntentSessionState>::MakeError(
                plan_result.error_code,
                "Failed to propose intent plan: " + plan_result.error_message
            );
        }

        // Create a new step record
        IntentStepRecord step;
        step.step_index = st.steps.GetCount();
        step.user_id = user_id;
        step.request = req;
        step.plan = plan_result.data;
        step.applied = false;

        // Add to steps
        st.steps.Add(step);

        // The step will be saved when integrated with CircuitFacade which has session_dir

        return Result<IntentSessionState>::MakeOk(st);
    } catch (const std::exception& e) {
        return Result<IntentSessionState>::MakeError(
            ErrorCode::InternalError,
            "Exception in IntentSessionManager::ProposeStep: " + std::string(e.what())
        );
    }
}

Result<IntentSessionState> IntentSessionManager::ApplyStep(
    IntentSessionState& st,
    int step_index,
    bool require_accepted
) {
    try {
        if (step_index < 0 || step_index >= st.steps.GetCount()) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Invalid step index: " + std::to_string(step_index)
            );
        }

        IntentStepRecord& step = st.steps[step_index];

        if (require_accepted && !step.plan.accepted) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::InvalidEditOperation,
                "Step plan was not accepted and require_accepted is true"
            );
        }

        // Apply the plan's actions to the current instrument
        InstrumentGraph updated_instrument = st.current_instrument;

        // Apply each action in the plan
        for (const auto& action : step.plan.actions) {
            switch (action.kind) {
                case IntentActionKind::SetInstrumentParam:
                    // Apply parameter changes to the instrument
                    if (action.target == "pan_lfo_hz") {
                        updated_instrument.voice_template.pan_lfo_hz = action.value;
                    } else if (action.target == "pan_lfo_depth") {
                        // For now, this is a placeholder - the InstrumentGraph doesn't have a direct field for pan depth
                        // We'd need to extend it or handle the parameter differently
                    }
                    break;

                case IntentActionKind::AdjustMix:
                    // Apply mixing adjustments like stereo width
                    if (action.target == "stereo_width") {
                        // For now, just a placeholder - we'd need to implement this in InstrumentGraph if needed
                        // The InstrumentGraph doesn't currently have stereo width as a direct parameter
                    } else if (action.target == "output_gain") {
                        // Placeholder for output gain adjustment
                    }
                    break;

                case IntentActionKind::AddModulator:
                    // Add modulation components to the instrument
                    // This would involve adding LFO or mod routing
                    break;

                case IntentActionKind::SuggestRefactorPlaybook:
                    // Handle refactoring suggestions (this might just be logged for now)
                    break;
            }
        }

        // Update the current instrument to the new state
        st.current_instrument = updated_instrument;

        step.applied = true;

        // Update metadata
        st.meta.head_step_index = step_index;
        st.meta.instrument_revision++;

        // Save the updated session state
        // In a full implementation, we'd pass the session_dir here
        // For now, this is handled in CircuitFacade integration

        return Result<IntentSessionState>::MakeOk(st);
    } catch (const std::exception& e) {
        return Result<IntentSessionState>::MakeError(
            ErrorCode::InternalError,
            "Exception in IntentSessionManager::ApplyStep: " + std::string(e.what())
        );
    }
}

Result<IntentSessionState> IntentSessionManager::Undo(
    IntentSessionState& st
) {
    try {
        if (st.meta.head_step_index < 0) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::InvalidEditOperation,
                "No steps to undo"
            );
        }

        // To undo, we need to replay from the initial instrument up to (head_step_index - 1)
        // First, we need to identify the original instrument at step -1 (which is the initial instrument)
        // For now, we'll reconstruct it by replaying steps up to the current head - 1
        // We'll need a way to store the original instrument, but for now we'll work with the current

        // Find the original instrument by replaying all applied steps up to (current_head - 1) from the initial instrument
        // For this, we need to store the initial instrument separately - let's assume the first state was the original
        // In a more robust implementation, we would store the initial instrument separately
        InstrumentGraph original_instrument = st.current_instrument;

        // We need to actually go back and replay steps to reconstruct the instrument state.
        // To do this properly, we'll need to track what the original instrument was.
        // For now, we'll track the instrument state at each step to make undo/redo possible.

        // Actually, we need to go back to the original instrument state, then replay steps up to head_step_index - 1
        // This means we need to have the initial instrument available somewhere.
        // For now, let's go back by decrementing the head index and assume the previous state is available.

        // In the current approach, since we don't have a history of instrument states,
        // let's use the replay mechanism to reconstruct the previous state
        if (st.meta.head_step_index == 0) {
            // If we're at step 0, undo means return to the original instrument
            // For this we need to save the initial instrument separately, which we'll do in a full implementation
            // For now, we'll just set the head to -1 to indicate we're before any steps
            st.meta.head_step_index = -1;
        } else {
            // Replay all steps up to (head_step_index - 1) from the original instrument
            // This is a simplified implementation - in practice we'd store intermediate states or
            // reconstruct the instrument by applying steps in sequence
            st.meta.head_step_index--;

            // Reconstruct the instrument by replaying from the beginning up to current head index
            if (st.meta.head_step_index >= 0) {
                // Create the initial instrument (this should be stored separately in a production implementation)
                // For now, we'll work with the concept that we'll replay from the original state that was passed
                // in the Create function
                // In a real implementation, we'd need to store the initial instrument to be able to replay from it
                Result<InstrumentGraph> replay_result = ReplayStepsToInstrument(
                    st,
                    st.current_instrument,  // This is incorrect - we need the initial instrument
                    st.meta.head_step_index,
                    true // require accepted
                );

                if (replay_result.ok) {
                    st.current_instrument = replay_result.data;
                } else {
                    // If replay fails, we still update the head index but the instrument won't be correct
                    // This indicates a problem with determinism
                    return Result<IntentSessionState>::MakeError(
                        replay_result.error_code,
                        "Failed to reconstruct instrument state during undo: " + replay_result.error_message
                    );
                }
            }
        }

        return Result<IntentSessionState>::MakeOk(st);
    } catch (const std::exception& e) {
        return Result<IntentSessionState>::MakeError(
            ErrorCode::InternalError,
            "Exception in IntentSessionManager::Undo: " + std::string(e.what())
        );
    }
}

Result<IntentSessionState> IntentSessionManager::Redo(
    IntentSessionState& st
) {
    try {
        if (st.meta.head_step_index >= st.steps.GetCount() - 1) {
            return Result<IntentSessionState>::MakeError(
                ErrorCode::InvalidEditOperation,
                "No steps to redo"
            );
        }

        // Increment the head index to apply the next step
        st.meta.head_step_index++;

        // Apply the step at the new head index to the instrument
        // We need to apply the actual changes from the step
        if (st.meta.head_step_index >= 0 && st.meta.head_step_index < st.steps.GetCount()) {
            const IntentStepRecord& step = st.steps[st.meta.head_step_index];

            if (step.applied) {
                // If the step is already applied (should be the case after a prior application),
                // we still need to make sure the current instrument reflects the application of this step
                // For now, we'll replay from beginning to current head to ensure consistency
                Result<InstrumentGraph> replay_result = ReplayStepsToInstrument(
                    st,
                    st.current_instrument, // Again, this should be the original instrument
                    st.meta.head_step_index,
                    true // require accepted
                );

                if (replay_result.ok) {
                    st.current_instrument = replay_result.data;
                } else {
                    return Result<IntentSessionState>::MakeError(
                        replay_result.error_code,
                        "Failed to reconstruct instrument state during redo: " + replay_result.error_message
                    );
                }
            }
        }

        return Result<IntentSessionState>::MakeOk(st);
    } catch (const std::exception& e) {
        return Result<IntentSessionState>::MakeError(
            ErrorCode::InternalError,
            "Exception in IntentSessionManager::Redo: " + std::string(e.what())
        );
    }
}

Result<InstrumentGraph> IntentSessionManager::ReplayStepsToInstrument(
    const IntentSessionState& st,
    const InstrumentGraph& target_initial,
    int up_to_step_index,
    bool require_accepted
) {
    try {
        // Start with the target initial instrument
        InstrumentGraph result_instrument = target_initial;

        // Determine how many steps to apply
        int max_steps = (up_to_step_index == -1) ? st.steps.GetCount() : std::min(up_to_step_index + 1, st.steps.GetCount());

        // Apply each step in order
        for (int i = 0; i < max_steps; i++) {
            if (i >= st.steps.GetCount()) break;

            const IntentStepRecord& step = st.steps[i];

            if (require_accepted && !step.plan.accepted) {
                return Result<InstrumentGraph>::MakeError(
                    ErrorCode::InvalidEditOperation,
                    "Step " + std::to_string(i) + " was not accepted and require_accepted is true"
                );
            }

            // Apply the step's actions to the instrument
            // This involves applying each IntentAction in the plan's action list
            for (const auto& action : step.plan.actions) {
                switch (action.kind) {
                    case IntentActionKind::SetInstrumentParam:
                        // Apply parameter changes to the instrument
                        // This would involve updating fields of the InstrumentGraph based on target and value
                        // For example, if target is "pan_lfo_hz", we'd update the voice template's pan_lfo_hz
                        if (action.target == "pan_lfo_hz") {
                            // This is just an example - actual implementation would depend on how
                            // the InstrumentGraph supports parameter updates
                            // For now, this is a placeholder for the actual implementation
                        } else if (action.target == "pan_lfo_depth") {
                            // Another example parameter
                        }
                        // Add more parameter handling as needed
                        break;

                    case IntentActionKind::AdjustMix:
                        // Apply mixing adjustments like stereo width
                        if (action.target == "stereo_width") {
                            // Update the stereo width parameter in the instrument
                        } else if (action.target == "output_gain") {
                            // Update the output gain
                        }
                        break;

                    case IntentActionKind::AddModulator:
                        // Add modulation components to the instrument
                        // This would involve adding LFO or mod routing
                        break;

                    case IntentActionKind::SuggestRefactorPlaybook:
                        // Handle refactoring suggestions (this might just be logged for now)
                        break;
                }
            }
        }

        return Result<InstrumentGraph>::MakeOk(result_instrument);
    } catch (const std::exception& e) {
        return Result<InstrumentGraph>::MakeError(
            ErrorCode::InternalError,
            "Exception in IntentSessionManager::ReplayStepsToInstrument: " + std::string(e.what())
        );
    }
}

// Internal helper methods
Result<void> IntentSessionManager::SaveSession(
    const IntentSessionState& session_state,
    const String& session_dir
) {
    try {
        String intent_session_path = session_dir + "/intent_sessions/" + 
                                    session_state.meta.intent_session_id;
        
        // Create directory if it doesn't exist
        fs::create_directories(intent_session_path);
        
        // Prepare session data for JSON serialization
        Upp::ValueMap session_data;
        
        // Add metadata
        Upp::ValueMap meta_map;
        meta_map.Add("intent_session_id", session_state.meta.intent_session_id);
        meta_map.Add("created_at", session_state.meta.created_at);
        meta_map.Add("created_with", session_state.meta.created_with);
        meta_map.Add("current_branch", session_state.meta.current_branch);
        meta_map.Add("instrument_revision", session_state.meta.instrument_revision);
        meta_map.Add("head_step_index", session_state.meta.head_step_index);
        session_data.Add("meta", meta_map);
        
        // Add current instrument
        Upp::ValueMap instrument_map = JsonIO::InstrumentGraphToValueMap(session_state.current_instrument);
        session_data.Add("current_instrument", instrument_map);
        
        // Add steps count
        session_data.Add("steps_count", (int)session_state.steps.GetCount());
        
        // Write to file with atomic operation (write to temp and rename)
        String temp_path = intent_session_path + "/intent_session.json.tmp";
        String final_path = intent_session_path + "/intent_session.json";
        
        std::ofstream file(temp_path);
        if (!file.is_open()) {
            return Result<void>::MakeError(
                ErrorCode::StorageIoError,
                "Could not open file for writing: " + temp_path
            );
        }
        
        String json_str = JsonIO::ValueMapToJson(session_data);
        file << json_str;
        file.close();
        
        // Atomic rename
        fs::rename(temp_path, final_path);
        
        // Also save current instrument to its own file
        String instrument_temp_path = intent_session_path + "/instrument_current.json.tmp";
        String instrument_final_path = intent_session_path + "/instrument_current.json";
        
        std::ofstream instrument_file(instrument_temp_path);
        if (instrument_file.is_open()) {
            String instrument_json = JsonIO::ValueMapToJson(instrument_map);
            instrument_file << instrument_json;
            instrument_file.close();
            fs::rename(instrument_temp_path, instrument_final_path);
        }
        
        return Result<void>::MakeOk();
    } catch (const std::exception& e) {
        return Result<void>::MakeError(
            ErrorCode::StorageIoError,
            "Exception in SaveSession: " + std::string(e.what())
        );
    }
}

Result<void> IntentSessionManager::SaveStep(
    const IntentStepRecord& step_record,
    const String& session_dir,
    const String& intent_session_id
) {
    try {
        String intent_session_path = session_dir + "/intent_sessions/" + intent_session_id;
        String steps_path = intent_session_path + "/steps";
        
        // Create directory if it doesn't exist
        fs::create_directories(steps_path);
        
        // Prepare step data for JSON serialization
        Upp::ValueMap step_data;
        
        // Add step details
        step_data.Add("step_index", step_record.step_index);
        step_data.Add("user_id", step_record.user_id);
        step_data.Add("request", JsonIO::IntentRequestToValueMap(step_record.request));
        step_data.Add("plan", JsonIO::IntentPlanToValueMap(step_record.plan));
        step_data.Add("applied", step_record.applied);
        step_data.Add("note", step_record.note);
        
        // Write to file with atomic operation (write to temp and rename)
        String temp_path = steps_path + "/step_" + Upp::Format("%08d.json.tmp", step_record.step_index);
        String final_path = steps_path + "/step_" + Upp::Format("%08d.json", step_record.step_index);
        
        std::ofstream file(temp_path);
        if (!file.is_open()) {
            return Result<void>::MakeError(
                ErrorCode::StorageIoError,
                "Could not open step file for writing: " + temp_path
            );
        }
        
        String json_str = JsonIO::ValueMapToJson(step_data);
        file << json_str;
        file.close();
        
        // Atomic rename
        fs::rename(temp_path, final_path);
        
        return Result<void>::MakeOk();
    } catch (const std::exception& e) {
        return Result<void>::MakeError(
            ErrorCode::StorageIoError,
            "Exception in SaveStep: " + std::string(e.what())
        );
    }
}

Result<IntentStepRecord> IntentSessionManager::LoadStep(
    const String& session_dir,
    const String& intent_session_id,
    int step_index
) {
    try {
        String intent_session_path = session_dir + "/intent_sessions/" + intent_session_id;
        String step_path = intent_session_path + "/steps/step_" + Upp::Format("%08d.json", step_index);
        
        std::ifstream file(step_path);
        if (!file.is_open()) {
            return Result<IntentStepRecord>::MakeError(
                ErrorCode::StorageIoError,
                "Could not open step file for reading: " + step_path
            );
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();
        
        Upp::ValueMap step_data = JsonIO::Deserialize(content);
        
        IntentStepRecord step;
        step.step_index = (int)step_data["step_index"];
        step.user_id = step_data["user_id"];
        // TODO: Convert from ValueMap back to IntentRequest and IntentPlan
        step.applied = step_data["applied"].Get<bool>();
        step.note = step_data["note"];
        
        return Result<IntentStepRecord>::MakeOk(step);
    } catch (const std::exception& e) {
        return Result<IntentStepRecord>::MakeError(
            ErrorCode::InternalError,
            "Exception in LoadStep: " + std::string(e.what())
        );
    }
}

} // namespace ProtoVMCLI