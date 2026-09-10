#include "PerformanceScenes.h"
#include "IntentProfiles.h"
#include "IntentSessions.h"
#include "InstrumentGraph.h"
#include "AutomationExport.h"
#include <cmath>

using namespace Upp;

Result<SceneSnapshot> SceneBuilder::BuildSceneFromProfile(
    const IntentProfile& profile,
    const InstrumentGraph& base_instrument
) {
    try {
        SceneSnapshot snapshot;
        snapshot.scene_id = "scene_" + profile.profile_id;
        snapshot.name = profile.metadata.name + " Scene";
        
        // Copy the resolved parameters from the profile
        for (const auto& param : base_instrument.parameters) {
            // We'll set default values from the base instrument
            // In a real implementation, these would be resolved based on the intent profile
            snapshot.params.Add(param.id.c_str(), param.default_value);
        }
        
        // Add automation lanes from the profile if available
        for (const auto& lane : profile.automation_lanes) {
            snapshot.automation.Add(lane);
        }
        
        // Set intent strengths from the profile
        for (const auto& step : profile.steps) {
            // The strength value would come from the intent profile
            // For now, we'll use a placeholder value
            snapshot.intent_strengths.Add(step.intent_id.c_str(), step.strength);
        }
        
        return Result<SceneSnapshot>::Success(snapshot);
    }
    catch (const std::exception& e) {
        return Result<SceneSnapshot>::Error(std::string("Failed to build scene from profile: ") + e.what());
    }
}

Result<SceneSnapshot> SceneBuilder::BuildSceneFromSession(
    const IntentSessionState& session,
    const InstrumentGraph& base_instrument
) {
    try {
        SceneSnapshot snapshot;
        snapshot.scene_id = "scene_" + session.session_id;
        snapshot.name = session.metadata.name + " Scene";
        
        // Copy parameters from the base instrument
        for (const auto& param : base_instrument.parameters) {
            // In a real implementation, this would reflect the current state after applying intents
            snapshot.params.Add(param.id.c_str(), param.default_value);
        }
        
        // Add any automation lanes from the session
        // This would need to be adapted based on how sessions store automation data
        // For now, we'll add empty automation lanes
        for (const auto& param : base_instrument.parameters) {
            AutomationLane lane;
            lane.id = param.id + "_baseline";
            lane.name = param.name + " Baseline";
            lane.target_id = param.id;
            lane.target_kind = AutomationTargetKind::Parameter;
            
            // Add a baseline automation point
            AutomationPoint baseline_point;
            baseline_point.x = 0.0;  // Start of timeline
            baseline_point.y = param.default_value;  // Default value
            baseline_point.curve = 1.0;  // Linear
            lane.points.Add(baseline_point);
            
            snapshot.automation.Add(lane);
        }
        
        // Set intent strengths based on applied steps in the session
        for (const auto& step : session.steps) {
            if (step.status == IntentStatus::Applied) {
                // For now, we'll just store the strength from the plan
                // In a real implementation, this would be more nuanced
                snapshot.intent_strengths.Add(step.step_id.c_str(), step.strength);
            }
        }
        
        return Result<SceneSnapshot>::Success(snapshot);
    }
    catch (const std::exception& e) {
        return Result<SceneSnapshot>::Error(std::string("Failed to build scene from session: ") + e.what());
    }
}

Result<void> SceneMorphEngine::ApplyMorph(
    InstrumentRuntime& runtime,
    const SceneSnapshot& a,
    const SceneSnapshot& b,
    double morph
) {
    try {
        // Clamp morph value between 0.0 and 1.0
        double clamped_morph = std::max(0.0, std::min(1.0, morph));
        
        // Interpolate parameters between scene a and scene b
        for (int i = 0; i < a.params.GetCount(); ++i) {
            Upp::String key = a.params.GetKey(i);
            
            // Check if both scenes have this parameter
            if (b.params.Find(key) >= 0) {
                // Get values from both scenes
                double value_a = a.params.Get(key).GetDouble();
                double value_b = b.params.Get(key).GetDouble();
                
                // Calculate interpolated value
                double interpolated_value = value_a + clamped_morph * (value_b - value_a);
                
                // Apply to runtime
                runtime.SetParameter(key, interpolated_value);
            } else {
                // If parameter exists only in scene a, use its value directly
                runtime.SetParameter(key, a.params.Get(key).GetDouble());
            }
        }
        
        // Also consider parameters that exist only in scene b
        for (int i = 0; i < b.params.GetCount(); ++i) {
            Upp::String key = b.params.GetKey(i);
            
            // Only handle parameters that don't exist in scene a
            if (a.params.Find(key) < 0) {
                // For parameters in scene b but not in scene a,
                // we'll set them to the scene b value scaled by morph amount
                double value_b = b.params.Get(key).GetDouble();
                double scaled_value = clamped_morph * value_b;
                
                runtime.SetParameter(key, scaled_value);
            }
        }
        
        // Interpolate intent strengths
        for (int i = 0; i < a.intent_strengths.GetCount(); ++i) {
            Upp::String key = a.intent_strengths.GetKey(i);
            
            // Check if both scenes have this intent strength
            if (b.intent_strengths.Find(key) >= 0) {
                // Get values from both scenes
                float strength_a = a.intent_strengths.Get(key).GetDouble();
                float strength_b = b.intent_strengths.Get(key).GetDouble();
                
                // Calculate interpolated value
                float interpolated_strength = strength_a + clamped_morph * (strength_b - strength_a);
                
                // Apply to runtime
                runtime.SetIntentionStrength(key, interpolated_strength);
            } else {
                // If intent strength exists only in scene a, use its value directly
                runtime.SetIntentionStrength(key, a.intent_strengths.Get(key).GetDouble());
            }
        }
        
        // Also consider intent strengths that exist only in scene b
        for (int i = 0; i < b.intent_strengths.GetCount(); ++i) {
            Upp::String key = b.intent_strengths.GetKey(i);
            
            // Only handle intent strengths that don't exist in scene a
            if (a.intent_strengths.Find(key) < 0) {
                // For intent strengths in scene b but not in scene a,
                // we'll set them to the scene b value scaled by morph amount
                float strength_b = b.intent_strengths.Get(key).GetDouble();
                float scaled_strength = clamped_morph * strength_b;
                
                runtime.SetIntentionStrength(key, scaled_strength);
            }
        }
        
        // Morph between automation lanes
        // This is more complex and would involve morphing between different automation curves
        // For now, we'll apply a simple morph between baseline automation lanes
        for (int i = 0; i < std::min(a.automation.GetCount(), b.automation.GetCount()); ++i) {
            const auto& lane_a = a.automation[i];
            const auto& lane_b = b.automation[i];
            
            // Create a new lane that represents the morphed state
            AutomationLane morphed_lane = lane_a; // Start with lane a as base
            
            // Morph the points between lane_a and lane_b
            for (int j = 0; j < std::min(lane_a.points.GetCount(), lane_b.points.GetCount()); ++j) {
                const auto& point_a = lane_a.points[j];
                const auto& point_b = lane_b.points[j];
                
                // Linearly interpolate between the points
                AutomationPoint morphed_point;
                morphed_point.x = point_a.x + clamped_morph * (point_b.x - point_a.x);
                morphed_point.y = point_a.y + clamped_morph * (point_b.y - point_a.y);
                morphed_point.curve = point_a.curve + clamped_morph * (point_b.curve - point_a.curve);
                
                // Use the morphed point
                if (j < morphed_lane.points.GetCount()) {
                    morphed_lane.points[j] = morphed_point;
                } else {
                    morphed_lane.points.Add(morphed_point);
                }
            }
            
            // Apply the morphed lane to the runtime
            runtime.SetAutomationLaneValue(morphed_lane.id, morphed_lane.points[0].y); // Use first point as current value
        }
        
        return Result<void>::Success();
    }
    catch (const std::exception& e) {
        return Result<void>::Error(std::string("Failed to apply morph: ") + e.what());
    }
}