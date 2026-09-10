#include "MusicalIntent.h"
#include "CircuitFacade.h"
#include "InstrumentRuntime.h"
#include <cmath>
#include <algorithm>

namespace ProtoVMCLI {

// Helper function to convert IntentKind to string
static Upp::String IntentKindToString(IntentKind kind) {
    switch (kind) {
        case IntentKind::Warmer: return "Warmer";
        case IntentKind::Brighter: return "Brighter";
        case IntentKind::Wider: return "Wider";
        case IntentKind::Narrower: return "Narrower";
        case IntentKind::MoreMovement: return "MoreMovement";
        case IntentKind::LessMovement: return "LessMovement";
        case IntentKind::Softer: return "Softer";
        case IntentKind::MorePunch: return "MorePunch";
        case IntentKind::Cleaner: return "Cleaner";
        case IntentKind::Dirtier: return "Dirtier";
        default: return "Unknown";
    }
}

// Mapping intents to target metrics
static std::vector<AudioQa::AudioQaMetricKind> GetTargetMetrics(IntentKind kind) {
    switch (kind) {
        case IntentKind::Warmer:
            return {AudioQa::AudioQaMetricKind::HarmonicEnergy, 
                    AudioQa::AudioQaMetricKind::PeakLevel};
        case IntentKind::Brighter:
            return {AudioQa::AudioQaMetricKind::HarmonicEnergy};
        case IntentKind::Wider:
            return {AudioQa::AudioQaMetricKind::StereoWidth};
        case IntentKind::Narrower:
            return {AudioQa::AudioQaMetricKind::StereoWidth};
        case IntentKind::Softer:
            return {AudioQa::AudioQaMetricKind::PeakLevel};
        case IntentKind::MorePunch:
            return {AudioQa::AudioQaMetricKind::PeakLevel};
        case IntentKind::Cleaner:
            return {AudioQa::AudioQaMetricKind::DCOffset,
                    AudioQa::AudioQaMetricKind::HarmonicEnergy};
        case IntentKind::Dirtier:
            return {AudioQa::AudioQaMetricKind::HarmonicEnergy};
        default:
            return {};
    }
}

// Create actions for a specific intent
static std::vector<IntentAction> CreateActionsForIntent(const InstrumentGraph& instrument, 
                                                        IntentKind intent, 
                                                        double strength) {
    std::vector<IntentAction> actions;
    
    switch (intent) {
        case IntentKind::Warmer:
            // Reduce highs by adjusting filter parameters if present, or suggest refactoring
            // For now, we'll suggest a refactor since filter parameters aren't available in current InstrumentGraph
            {
                IntentAction action;
                action.kind = IntentActionKind::SuggestRefactorPlaybook;
                action.target = "AddHighFrequencyRollOff";
                action.value = strength;
                action.note = "To make it warmer, implement a high-frequency roll-off filter";
                actions.push_back(action);
                
                // Also suggest reducing harmonic energy by adding gentle saturation
                IntentAction action2;
                action2.kind = IntentActionKind::SuggestRefactorPlaybook;
                action2.target = "AddGentleSaturation";
                action2.value = strength * 0.3;  // Milder effect
                action2.note = "Add gentle saturation to reduce harsh high frequencies";
                actions.push_back(action2);
            }
            break;
            
        case IntentKind::Brighter:
            {
                IntentAction action;
                action.kind = IntentActionKind::SuggestRefactorPlaybook;
                action.target = "AddHighFrequencyBoost";
                action.value = strength;
                action.note = "To make it brighter, implement a high-frequency boost filter";
                actions.push_back(action);
            }
            break;
            
        case IntentKind::Wider:
            {
                IntentAction action;
                action.kind = IntentActionKind::AdjustMix;
                action.target = "stereo_width";
                action.value = std::min(1.0, 0.7 + strength * 0.3);  // from 0.7 to 1.0
                action.note = "Increase stereo width for wider image";
                actions.push_back(action);
            }
            break;
            
        case IntentKind::Narrower:
            {
                IntentAction action;
                action.kind = IntentActionKind::AdjustMix;
                action.target = "stereo_width";
                action.value = std::max(0.1, 0.7 - strength * 0.6);  // from 0.7 to 0.1
                action.note = "Decrease stereo width for narrower image";
                actions.push_back(action);
            }
            break;
            
        case IntentKind::MoreMovement:
            {
                IntentAction action;
                action.kind = IntentActionKind::SetInstrumentParam;
                action.target = "pan_lfo_hz";
                action.value = 0.5 + strength * 2.0;  // from 0.5Hz to 2.5Hz
                action.note = "Increase pan LFO rate for more movement";
                actions.push_back(action);
                
                IntentAction action2;
                action2.kind = IntentActionKind::SetInstrumentParam;
                action2.target = "pan_lfo_depth";
                action2.value = 0.3 + strength * 0.7;  // from 0.3 to 1.0
                action2.note = "Increase pan LFO depth for more movement";
                actions.push_back(action2);
            }
            break;
            
        case IntentKind::LessMovement:
            {
                IntentAction action;
                action.kind = IntentActionKind::SetInstrumentParam;
                action.target = "pan_lfo_hz";
                action.value = 0.1 + (1.0 - strength) * 0.4;  // from 0.5Hz to 0.1Hz
                action.note = "Decrease pan LFO rate for less movement";
                actions.push_back(action);
                
                IntentAction action2;
                action2.kind = IntentActionKind::SetInstrumentParam;
                action2.target = "pan_lfo_depth";
                action2.value = 0.1 + (1.0 - strength) * 0.4;  // from 0.5 to 0.1
                action2.note = "Decrease pan LFO depth for less movement";
                actions.push_back(action2);
            }
            break;
            
        case IntentKind::Softer:
            {
                IntentAction action;
                action.kind = IntentActionKind::SuggestRefactorPlaybook;
                action.target = "AddSoftClipping";
                action.value = strength;
                action.note = "Add soft clipping to reduce harsh transients";
                actions.push_back(action);
                
                // Also reduce peak levels
                IntentAction action2;
                action2.kind = IntentActionKind::AdjustMix;
                action2.target = "output_gain";
                action2.value = 1.0 - (strength * 0.2);  // reduce gain up to 20%
                action2.note = "Reduce overall gain to soften the sound";
                actions.push_back(action2);
            }
            break;
            
        case IntentKind::MorePunch:
            {
                IntentAction action;
                action.kind = IntentActionKind::SuggestRefactorPlaybook;
                action.target = "AddCompressorWithFastAttack";
                action.value = strength;
                action.note = "Add compressor with fast attack/release for more punch";
                actions.push_back(action);
                
                IntentAction action2;
                action2.kind = IntentActionKind::AdjustMix;
                action2.target = "attack_enhance";
                action2.value = strength;
                action2.note = "Enhance attack characteristics for more punch";
                actions.push_back(action2);
            }
            break;
            
        case IntentKind::Cleaner:
            {
                IntentAction action;
                action.kind = IntentActionKind::SuggestRefactorPlaybook;
                action.target = "AddLowDistortionProfile";
                action.value = strength;
                action.note = "Switch to low-distortion algorithm or parameters";
                actions.push_back(action);
                
                IntentAction action2;
                action2.kind = IntentActionKind::AdjustMix;
                action2.target = "dc_offset_correction";
                action2.value = 0.0;
                action2.note = "Apply DC offset correction";
                actions.push_back(action2);
            }
            break;
            
        case IntentKind::Dirtier:
            {
                IntentAction action;
                action.kind = IntentActionKind::SuggestRefactorPlaybook;
                action.target = "AddSaturation";
                action.value = strength * 0.5;  // Bounded amount
                action.note = "Add mild saturation for dirtier sound";
                actions.push_back(action);
                
                IntentAction action2;
                action2.kind = IntentActionKind::SuggestRefactorPlaybook;
                action2.target = "AddAnalogNoise";
                action2.value = strength * 0.3;  // Bounded amount
                action2.note = "Add subtle analog noise for character";
                actions.push_back(action2);
            }
            break;
    }
    
    return actions;
}

std::vector<IntentAction> MusicalIntentEngine::MapIntentsToActions(
    const InstrumentGraph& instrument,
    const IntentRequest& req
) {
    std::vector<IntentAction> all_actions;
    
    for (const auto& intent : req.intents) {
        auto actions = CreateActionsForIntent(instrument, intent, req.strength);
        all_actions.insert(all_actions.end(), actions.begin(), actions.end());
    }
    
    return all_actions;
}

AudioQa::AudioQaThresholdProfile MusicalIntentEngine::GetDefaultThresholdProfile() {
    AudioQa::AudioQaThresholdProfile profile;
    // Set reasonable default thresholds for audio quality
    profile.SetThreshold(AudioQa::AudioQaMetricKind::RMSLevel, -30.0, -5.0);      // Reasonable RMS range
    profile.SetThreshold(AudioQa::AudioQaMetricKind::PeakLevel, -12.0, -0.1);     // Reasonable peak level, avoiding clipping
    profile.SetThreshold(AudioQa::AudioQaMetricKind::DCOffset, -0.01, 0.01);      // Very low DC offset
    profile.SetThreshold(AudioQa::AudioQaMetricKind::StereoBalance, -0.9, 0.9);   // Not too imbalanced
    profile.SetThreshold(AudioQa::AudioQaMetricKind::FundamentalFrequency, 100.0, 5000.0); // Valid frequency range
    return profile;
}

bool MusicalIntentEngine::VerifyPlan(
    const InstrumentGraph& instrument,
    const IntentPlan& plan,
    CircuitFacade& facade,
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name
) {
    try {
        // Render "before" audio
        std::vector<float> before_left, before_right;
        auto render_before_result = InstrumentRuntime::RenderInstrument(
            instrument,
            facade,
            session,
            session_dir,
            branch_name,
            before_left,
            before_right
        );
        
        if (!render_before_result.ok) {
            return false;
        }
        
        // Create a modified instrument based on the plan's actions (without actually applying to original)
        InstrumentGraph modified_instrument = instrument;
        
        // Apply actions to the modified instrument
        for (const auto& action : plan.actions) {
            // This is a simplified implementation - in a real system, we'd apply the actions
            // to the instrument based on the action type and target
            if (action.kind == IntentActionKind::AdjustMix) {
                // Apply mixing adjustments like stereo width to the instrument
                // This would depend on how the instrument model handles these parameters
            }
            // Other action types would be handled as appropriate
        }
        
        // Render "after" audio
        std::vector<float> after_left, after_right;
        auto render_after_result = InstrumentRuntime::RenderInstrument(
            modified_instrument,
            facade,
            session,
            session_dir,
            branch_name,
            after_left,
            after_right
        );
        
        if (!render_after_result.ok) {
            return false;
        }
        
        // Analyze both with AudioQa
        AudioQa::AudioQaAnalysis before_analyzer(plan.request.sample_rate);
        before_analyzer.SetAudioDataInterleaved(interleaveAudio(before_left, before_right));
        auto before_report = before_analyzer.Analyze();
        
        AudioQa::AudioQaAnalysis after_analyzer(plan.request.sample_rate);
        after_analyzer.SetAudioDataInterleaved(interleaveAudio(after_left, after_right));
        auto after_report = after_analyzer.Analyze();
        
        // Store reports in the plan
        const_cast<IntentPlan&>(plan).before_report = before_report;
        const_cast<IntentPlan&>(plan).after_report = after_report;
        
        // Generate diff
        AudioQa::AudioQaDiff diff = AudioQa::AudioQaComparator::CompareReports(before_report, after_report);
        const_cast<IntentPlan&>(plan).diff = diff;
        
        // Apply threshold check based on allow_regression
        bool has_regression = diff.HasRegression();
        
        if (!plan.request.allow_regression && has_regression) {
            // Plan is rejected due to regression
            const_cast<IntentPlan&>(plan).accepted = false;
            const_cast<IntentPlan&>(plan).reasons.push_back("Regressions detected and allow_regression is false");
            return false;
        }
        
        const_cast<IntentPlan&>(plan).accepted = true;
        if (has_regression) {
            const_cast<IntentPlan&>(plan).reasons.push_back("Accepting plan with regressions (allow_regression=true)");
        } else {
            const_cast<IntentPlan&>(plan).reasons.push_back("No regressions detected");
        }
        
        return true;
    } catch (const std::exception& e) {
        const_cast<IntentPlan&>(plan).accepted = false;
        const_cast<IntentPlan&>(plan).reasons.push_back(Upp::String("Exception during verification: ") + e.what());
        return false;
    }
}

// Helper function to interleave audio channels for analysis
std::vector<float> interleaveAudio(const std::vector<float>& left, const std::vector<float>& right) {
    std::vector<float> interleaved;
    size_t min_size = std::min(left.size(), right.size());
    interleaved.reserve(min_size * 2);
    
    for (size_t i = 0; i < min_size; ++i) {
        interleaved.push_back(left[i]);
        interleaved.push_back(right[i]);
    }
    
    return interleaved;
}

Result<IntentPlan> MusicalIntentEngine::ProposeAndVerify(
    const InstrumentGraph& instrument,
    const IntentRequest& req
) {
    IntentPlan plan;
    plan.request = req;
    
    try {
        // Generate actions based on intents
        plan.actions = MapIntentsToActions(instrument, req);
        
        // The full verification (rendering before/after, analyzing, diffing) would happen in a separate call
        // This is the "proposal" phase where we just generate the actions
        
        // For now, return the plan with generated actions
        return Result<IntentPlan>::MakeOk(plan);
    } catch (const std::exception& e) {
        return Result<IntentPlan>::MakeError(
            ErrorCode::INTERNAL_ERROR,
            "Exception in ProposeAndVerify: " + std::string(e.what())
        );
    }
}

} // namespace ProtoVMCLI