#include "IntentProfiles.h"
#include <ProtoVM/JsonIO.h>
#include <ProtoVM/Util.h>
#ifdef USE_UPP
    #include <ProtoVM/ProtoVM.h>  // Include U++ types
#else
    #include "../ProtoVM/MockProtoVM.h"  // Include mock types
#endif
#include <fstream>
#include <filesystem>

namespace ProtoVMCLI {

// Helper function to scale intent requests
Result<IntentRequest> ScaleIntentRequest(
    const IntentRequest& original,
    double strength_scale
) {
    IntentRequest scaled = original;
    scaled.strength *= strength_scale;
    // Clamp strength between 0 and 1
    if (scaled.strength < 0.0) scaled.strength = 0.0;
    if (scaled.strength > 1.0) scaled.strength = 1.0;
    
    return scaled;
}

Result<IntentProfile> IntentProfileManager::CreateProfile(
    const IntentProfileMetadata& meta,
    const Vector<IntentProfileStep>& steps,
    const String& base_dir
) {
    // Validate inputs
    if (!IsValidProfileId(meta.profile_id)) {
        return Result<IntentProfile>::Error("Invalid profile ID format");
    }

    // Check if profile already exists
    String profile_path = base_dir + "/" + meta.profile_id + "/profile.json";
    if (Upp::GetFileTime(profile_path) >= 0) {  // File exists
        return Result<IntentProfile>::Error("Profile already exists with this ID");
    }

    IntentProfile profile;
    profile.meta = meta;
    profile.steps = steps;

    // Ensure the directory exists
    String dir_path = base_dir + "/" + meta.profile_id;
    if (!Upp::DirectoryCreate(dir_path)) {
        return Result<IntentProfile>::Error("Failed to create profile directory: " + dir_path);
    }

    // Save the profile
    Result<void> save_result = SaveProfile(profile, base_dir);
    if (!save_result.ok()) {
        return Result<IntentProfile>::Error(save_result.error());
    }

    return profile;
}

Result<IntentProfile> IntentProfileManager::LoadProfile(
    const String& base_dir,
    const String& profile_id
) {
    if (!IsValidProfileId(profile_id)) {
        return Result<IntentProfile>::Error("Invalid profile ID format");
    }

    String profile_path = base_dir + "/" + profile_id + "/profile.json";
    
    Upp::Value json_value;
    Result<Upp::Value> load_result = LoadJsonFromFile(profile_path);
    if (!load_result.ok()) {
        return Result<IntentProfile>::Error("Failed to load profile from " + profile_path + ": " + load_result.error());
    }

    json_value = load_result.value();

    // Use the deserialization function from JsonIO
    if (!json_value.IsMap()) {
        return Result<IntentProfile>::Error("Profile JSON root is not an object");
    }

    Upp::ValueMap profile_map = json_value;
    Result<IntentProfile> profile_result = JsonIO::IntentProfileFromValueMap(profile_map);

    if (!profile_result.ok()) {
        return Result<IntentProfile>::Error("Failed to deserialize profile: " + profile_result.error());
    }

    return profile_result.value();
}

Result<Vector<IntentProfileMetadata>> IntentProfileManager::ListProfiles(
    const String& base_dir,
    Optional<String> tag_filter
) {
    Vector<IntentProfileMetadata> profiles;
    
    // List subdirectories in base_dir
    Upp::FindFile ff(base_dir + "/*");
    while(ff.Find()) {
        if (ff.IsFolder()) {
            String profile_id = ff.GetName();
            String profile_path = base_dir + "/" + profile_id + "/profile.json";
            
            if (Upp::GetFileTime(profile_path) >= 0) {  // File exists
                // Load just the metadata to avoid loading entire profiles
                Upp::Value json_value;
                Result<Upp::Value> load_result = LoadJsonFromFile(profile_path);
                if (load_result.ok()) {
                    json_value = load_result.value();
                    
                    IntentProfileMetadata meta;
                    meta.profile_id = json_value.Get("meta")("profile_id");
                    meta.name = json_value.Get("meta")("name");
                    meta.description = json_value.Get("meta")("description");
                    
                    String scope_str = json_value.Get("meta")("scope");
                    if (scope_str == "Global") {
                        meta.scope = IntentProfileScope::Global;
                    } else {
                        meta.scope = IntentProfileScope::Workspace;
                    }
                    
                    meta.created_at = json_value.Get("meta")("created_at");
                    meta.created_with = json_value.Get("meta")("created_with");
                    meta.author = json_value.Get("meta")("author");

                    // Load tags to check filter
                    Upp::Value tags_value = json_value.Get("meta")("tags");
                    if (tags_value.IsArray()) {
                        for (int i = 0; i < tags_value.GetCount(); i++) {
                            meta.tags.Add(tags_value[i]);
                        }
                    }
                    
                    // Check tag filter if provided
                    if (tag_filter) {
                        bool has_tag = false;
                        for (const auto& tag : meta.tags) {
                            if (tag == tag_filter.Get()) {
                                has_tag = true;
                                break;
                            }
                        }
                        if (!has_tag) continue;  // Skip this profile
                    }
                    
                    profiles.Add(meta);
                }
            }
        }
    }
    
    return profiles;
}

Result<IntentSessionState> IntentProfileManager::ApplyProfileToSession(
    IntentSessionState& session,
    const IntentProfile& profile,
    const String& user_id,
    double strength_scale
) {
    // For each step in the profile, apply the scaled request to the session
    for (const auto& profile_step : profile.steps) {
        // Scale the request strength
        Result<IntentRequest> scaled_request_result = ScaleIntentRequest(profile_step.request, strength_scale);
        if (!scaled_request_result.ok()) {
            return Result<IntentSessionState>::Error("Failed to scale intent request: " + scaled_request_result.error());
        }
        
        IntentRequest scaled_request = scaled_request_result.value();
        
        // Propose the step based on the scaled request
        Result<IntentSessionState> proposed_session = IntentSessionManager::ProposeStep(session, scaled_request, user_id);
        if (!proposed_session.ok()) {
            return Result<IntentSessionState>::Error("Failed to propose step for profile: " + proposed_session.error());
        }
        
        // Update session with the proposed state
        session = proposed_session.value();
        
        // The most recently added step is the one we just proposed
        int last_step_idx = session.steps.GetCount() - 1;
        if (last_step_idx >= 0) {
            // Apply the proposed step to make it permanent
            Result<IntentSessionState> applied_session = IntentSessionManager::ApplyStep(session, last_step_idx, true);
            if (!applied_session.ok()) {
                return Result<IntentSessionState>::Error("Failed to apply step from profile: " + applied_session.error());
            }
            session = applied_session.value();
        }
    }
    
    return session;
}

Result<InstrumentGraph> IntentProfileManager::ApplyProfileToInstrument(
    const InstrumentGraph& initial,
    const IntentProfile& profile,
    double strength_scale,
    bool require_accepted
) {
    // Start with the initial instrument
    InstrumentGraph current_instrument = initial;
    
    // Apply each step in the profile
    for (const auto& profile_step : profile.steps) {
        // Scale the request strength
        Result<IntentRequest> scaled_request_result = ScaleIntentRequest(profile_step.request, strength_scale);
        if (!scaled_request_result.ok()) {
            return Result<InstrumentGraph>::Error("Failed to scale intent request: " + scaled_request_result.error());
        }
        
        IntentRequest scaled_request = scaled_request_result.value();
        
        // Propose the transformation
        Result<IntentPlan> plan_result = MusicalIntentEngine::ProposeAndVerify(current_instrument, scaled_request);
        if (!plan_result.ok()) {
            return Result<InstrumentGraph>::Error("Failed to propose intent plan: " + plan_result.error());
        }
        
        IntentPlan plan = plan_result.value();
        
        // If we require acceptance and the plan wasn't accepted, stop
        if (require_accepted && !plan.accepted) {
            return Result<InstrumentGraph>::Error("Intent plan was not accepted");
        }
        
        // Apply the plan actions to get the new instrument
        // This would require a helper function to apply IntentActions to an InstrumentGraph
        Result<InstrumentGraph> updated_instrument = ApplyIntentPlanToInstrument(current_instrument, plan);
        if (!updated_instrument.ok()) {
            return Result<InstrumentGraph>::Error("Failed to apply intent plan: " + updated_instrument.error());
        }
        
        current_instrument = updated_instrument.value();
    }
    
    return current_instrument;
}

Result<void> IntentProfileManager::SaveProfile(
    const IntentProfile& profile,
    const String& base_dir
) {
    try {
        Upp::Value json_value;
        
        // Use the JSON serialization functions from JsonIO
        json_value = JsonIO::IntentProfileToValueMap(profile);
        
        // Write to file atomically
        String dir_path = base_dir + "/" + profile.meta.profile_id;
        String tmp_path = dir_path + "/profile.tmp.json";
        String final_path = dir_path + "/profile.json";
        
        // Attempt to write to temporary file first
        Result<void> write_result = SaveJsonToFile(json_value, tmp_path);
        if (!write_result.ok()) {
            return Result<void>::Error("Failed to write temporary profile: " + write_result.error());
        }
        
        // Atomically rename the temporary file to final file
        if (!Upp::RenameFile(tmp_path, final_path)) {
            Upp::DeleteFile(tmp_path);  // Clean up temp file if rename fails
            return Result<void>::Error("Failed to atomically update profile file");
        }
        
        return Result<void>::Success();
    } catch (const std::exception& e) {
        return Result<void>::Error("Exception occurred while saving profile: " + Upp::String(e.what()));
    }
}

bool IntentProfileManager::IsValidProfileId(const String& profile_id) {
    // Check if profile ID follows naming convention: starts with "ip-" followed by alphanumeric and hyphens
    if (profile_id.GetLength() < 4 || profile_id.Left(3) != "ip-") {
        return false;
    }
    
    for (int i = 3; i < profile_id.GetLength(); i++) {
        char c = profile_id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-')) {
            return false;
        }
    }
    
    return true;
}

// Helper function that applies an IntentPlan to an InstrumentGraph
Result<InstrumentGraph> ApplyIntentPlanToInstrument(const InstrumentGraph& original, const IntentPlan& plan) {
    // This function would implement the changes described in the plan to the instrument
    // For now, we'll return the original instrument as a placeholder
    // In a real implementation, this would iterate through the actions in the plan
    // and apply them to the instrument graph

    InstrumentGraph result = original;

    // Actually apply the actions from the plan to the instrument
    for (const auto& action : plan.actions) {
        // Apply each action to modify the instrument graph
        // This would require implementing an action application system
        // For now, we'll just return the original to avoid compilation errors
    }

    return result;
}

} // namespace ProtoVMCLI