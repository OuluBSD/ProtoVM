#include "AutomationExport.h"
#include "MusicalIntent.h"
#include "IntentSessions.h"
#include "IntentProfiles.h"
#include <iostream>

namespace ProtoVMCLI {

// Simple test for the AutomationExport module
bool TestAutomationExport() {
    std::cout << "Running AutomationExport tests..." << std::endl;
    
    // Test 1: BuildClipFromIntentPlan determinism
    try {
        IntentPlan plan;
        plan.request.intents.push_back(IntentKind::Warmer);
        plan.request.strength = 0.7;
        
        IntentAction action;
        action.kind = IntentActionKind::SetInstrumentParam;
        action.target = "gain";
        action.value = 0.8;
        action.note = "Increase gain for warmer sound";
        plan.actions.push_back(action);
        
        auto clip_result = AutomationExport::BuildClipFromIntentPlan(plan, 3.0, 48000);
        if (!clip_result.IsOk()) {
            std::cout << "FAIL: BuildClipFromIntentPlan failed: " << clip_result.GetError() << std::endl;
            return false;
        }
        
        auto clip = clip_result.Unwrap();
        if (clip.lanes.GetCount() != 1) {
            std::cout << "FAIL: Expected 1 lane, got " << clip.lanes.GetCount() << std::endl;
            return false;
        }
        
        if (clip.lanes[0].param_name != "gain") {
            std::cout << "FAIL: Expected param_name 'gain', got '" << clip.lanes[0].param_name << "'" << std::endl;
            return false;
        }
        
        std::cout << "PASS: BuildClipFromIntentPlan test" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "FAIL: BuildClipFromIntentPlan threw exception: " << e.what() << std::endl;
        return false;
    }
    
    // Test 2: BuildClipFromIntentSession
    try {
        IntentSessionState session;
        session.meta.intent_session_id = "test-session-001";
        
        IntentStepRecord step;
        step.step_index = 0;
        
        IntentPlan plan;
        plan.request.intents.push_back(IntentKind::Wider);
        plan.request.strength = 0.5;
        
        IntentAction action;
        action.kind = IntentActionKind::AdjustMix;
        action.target = "stereo_width";
        action.value = 0.9;
        action.note = "Increase stereo width";
        plan.actions.push_back(action);
        
        step.plan = plan;
        step.applied = true;
        
        session.steps.Add(step);
        
        auto clip_result = AutomationExport::BuildClipFromIntentSession(session, Null, 3.0, 48000, false);
        if (!clip_result.IsOk()) {
            std::cout << "FAIL: BuildClipFromIntentSession failed: " << clip_result.GetError() << std::endl;
            return false;
        }
        
        auto clip = clip_result.Unwrap();
        if (clip.lanes.GetCount() != 1) {
            std::cout << "FAIL: Expected 1 lane, got " << clip.lanes.GetCount() << std::endl;
            return false;
        }
        
        if (clip.lanes[0].param_name != "stereo_width") {
            std::cout << "FAIL: Expected param_name 'stereo_width', got '" << clip.lanes[0].param_name << "'" << std::endl;
            return false;
        }
        
        std::cout << "PASS: BuildClipFromIntentSession test" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "FAIL: BuildClipFromIntentSession threw exception: " << e.what() << std::endl;
        return false;
    }
    
    // Test 3: BuildClipFromIntentProfile
    try {
        IntentProfile profile;
        profile.meta.profile_id = "test-profile-001";
        
        IntentProfileStep step;
        step.request.intents.push_back(IntentKind::MoreMovement);
        step.request.strength = 0.6;
        step.note = "Add more movement via LFO";
        
        profile.steps.Add(step);
        
        auto clip_result = AutomationExport::BuildClipFromIntentProfile(profile, 3.0, 48000);
        if (!clip_result.IsOk()) {
            std::cout << "FAIL: BuildClipFromIntentProfile failed: " << clip_result.GetError() << std::endl;
            return false;
        }
        
        auto clip = clip_result.Unwrap();
        // The function maps intent kinds to parameter names like "modulation_amount"
        bool found_modulation_lane = false;
        for (const auto& lane : clip.lanes) {
            if (lane.param_name == "modulation_amount") {
                found_modulation_lane = true;
                break;
            }
        }
        
        if (!found_modulation_lane) {
            std::cout << "FAIL: Expected to find 'modulation_amount' lane" << std::endl;
            return false;
        }
        
        std::cout << "PASS: BuildClipFromIntentProfile test" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "FAIL: BuildClipFromIntentProfile threw exception: " << e.what() << std::endl;
        return false;
    }
    
    // Test 4: ExportClip basic export
    try {
        AutomationClip clip;
        clip.clip_id = "test-clip-001";
        clip.duration_sec = 3.0;
        clip.sample_rate = 48000;
        
        AutomationLane lane;
        lane.param_name = "gain";
        lane.points.Add(AutomationPoint(0.0, 0.0));
        lane.points.Add(AutomationPoint(0.02, 0.8));
        lane.note = "Gain automation test";
        clip.lanes.Add(lane);
        
        auto result = AutomationExport::ExportClip(clip, AutomationTargetKind::GenericJson, "./test_output", false);
        if (!result.IsOk()) {
            std::cout << "FAIL: ExportClip failed: " << result.GetError() << std::endl;
            return false;
        }
        
        auto export_result = result.Unwrap();
        if (export_result.written_files.GetCount() == 0) {
            std::cout << "FAIL: Expected at least one written file" << std::endl;
            return false;
        }
        
        std::cout << "PASS: ExportClip test" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "FAIL: ExportClip threw exception: " << e.what() << std::endl;
        return false;
    }
    
    std::cout << "All AutomationExport tests passed!" << std::endl;
    return true;
}

} // namespace ProtoVMCLI

int main() {
    return ProtoVMCLI::TestAutomationExport() ? 0 : 1;
}