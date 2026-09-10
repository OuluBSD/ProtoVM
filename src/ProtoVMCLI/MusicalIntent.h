#ifndef _ProtoVM_MusicalIntent_h_
#define _ProtoVM_MusicalIntent_h_

#ifdef USE_UPP
    #include <ProtoVM/ProtoVM.h>  // Include U++ types
#else
    #include "../ProtoVM/MockProtoVM.h"  // Include mock types
#endif  // Include U++ types
#include "SessionTypes.h"
#include "InstrumentGraph.h"
#include "InstrumentRuntime.h"
#include "../ProtoVM/AudioQa.h"
#include "../ProtoVM/AudioQaAnalysis.h"
#include "../ProtoVM/AudioQaDiff.h"
#include <vector>
#include <string>

namespace ProtoVMCLI {

// Musical intent vocabulary
enum class IntentKind {
    Warmer,        // reduce highs / add gentle saturation
    Brighter,      // increase highs / reduce low-mid
    Wider,         // increase stereo width
    Narrower,      // reduce stereo width
    MoreMovement,  // increase modulation depth/rate (safe)
    LessMovement,
    Softer,        // reduce harsh transients / reduce peakiness
    MorePunch,     // increase transient/attack (bounded)
    Cleaner,       // reduce harmonic distortion / DC / noise
    Dirtier        // add mild saturation/noise (bounded)
};

// Action types for implementing musical intents
enum class IntentActionKind {
    SetInstrumentParam,        // change InstrumentGraph / engine param
    AddModulator,              // add LFO or mod routing
    AdjustMix,                 // gain/pan/width adjustments
    SuggestRefactorPlaybook    // suggest running a safe refactor playbook
};

// Action to implement an intent
struct IntentAction {
    IntentActionKind kind;
    Upp::String target;            // param name or node id
    double value;                  // normalized or absolute depending on target
    Upp::String note;              // human-readable reason
};

// Request for musical intent transformation
struct IntentRequest {
    std::vector<IntentKind> intents;
    double strength = 0.5;        // 0..1
    double duration_sec = 3.0;
    int sample_rate = 48000;
    int block_size = 512;

    // Safety policy:
    bool allow_regression = false;      // if false, must not worsen QA thresholds
    Upp::String qa_profile_name;        // optional: choose a threshold profile/scenario
};

// Plan for intent transformation with verification results
struct IntentPlan {
    IntentRequest request;
    std::vector<IntentAction> actions;
    AudioQa::AudioQaReport before_report;
    AudioQa::AudioQaReport after_report;
    AudioQa::AudioQaDiff diff;
    bool accepted = false;
    std::vector<Upp::String> reasons;   // why accepted/rejected
};

// Core engine for mapping musical intents to concrete actions
class MusicalIntentEngine {
public:
    static Result<IntentPlan> ProposeAndVerify(
        const InstrumentGraph& instrument,
        const IntentRequest& req
    );
    
    // Helper methods
    static std::vector<IntentAction> MapIntentsToActions(
        const InstrumentGraph& instrument,
        const IntentRequest& req
    );
    
    static bool VerifyPlan(
        const InstrumentGraph& instrument,
        const IntentPlan& plan,
        CircuitFacade& facade,
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name
    );
    
    static AudioQa::AudioQaThresholdProfile GetDefaultThresholdProfile();
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_MusicalIntent_h_