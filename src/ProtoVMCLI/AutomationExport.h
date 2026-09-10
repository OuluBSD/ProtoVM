#ifndef _PROTOVM_AUTOMATIONEXPORT_H
#define _PROTOVM_AUTOMATIONEXPORT_H

#include "String.h"
#include "Vector.h"
#include "Value.h"
#include "Result.h"
#include "ProtoVM/ProtoVM.h"
#include "MusicalIntent.h"
#include "IntentSessions.h"
#include "IntentProfiles.h"
#include "InstrumentGraph.h"
#include "AudioQa.h"  // For AudioQaReport and AudioQaDiff
#include "AudioQaAnalysis.h"
#include "AudioQaDiff.h"
#include "PluginHostHarness.h"  // For plugin verification
#include "CircuitFacade.h"
#include <vector>
#include <string>

namespace ProtoVMCLI {

enum class AutomationTargetKind {
    ProtoVmInstrument,    // apply to InstrumentGraph/runtime
    Clap,                 // export CLAP param automation metadata
    Ladspa,               // export LADSPA control automation
    Lv2,                  // export LV2 control automation (optional)
    Vst3,                 // export VST3 automation (stub/optional)
    GenericJson           // always supported
};

struct AutomationPoint {
    double time_sec = 0.0;
    double value = 0.0;
    
    AutomationPoint() = default;
    AutomationPoint(double time_sec, double value) : time_sec(time_sec), value(value) {}
    
    bool operator==(const AutomationPoint& other) const {
        return time_sec == other.time_sec && value == other.value;
    }
};

struct AutomationLane {
    String param_name;            // canonical name
    int param_index = -1;         // optional
    Vector<AutomationPoint> points;
    String note;                  // intent rationale
    
    AutomationLane() = default;
    AutomationLane(String param_name) : param_name(param_name) {}
    
    bool operator==(const AutomationLane& other) const {
        return param_name == other.param_name && 
               param_index == other.param_index &&
               points == other.points &&
               note == other.note;
    }
};

struct AutomationClip {
    String clip_id;
    double duration_sec = 0.0;
    int sample_rate = 48000;

    Vector<AutomationLane> lanes;

    // Optional static snapshot (initial params):
    ValueMap initial_params;

    // Provenance:
    String source_intent_session_id;
    Vector<int> source_step_indices;
    String source_profile_id;
    
    AutomationClip() = default;
    
    bool operator==(const AutomationClip& other) const {
        return clip_id == other.clip_id &&
               duration_sec == other.duration_sec &&
               sample_rate == other.sample_rate &&
               lanes == other.lanes &&
               initial_params == other.initial_params &&
               source_intent_session_id == other.source_intent_session_id &&
               source_step_indices == other.source_step_indices &&
               source_profile_id == other.source_profile_id;
    }
};

struct AutomationExportResult {
    AutomationTargetKind target;
    String output_dir;
    Vector<String> written_files;
    AudioQa::AudioQaReport before_report;
    AudioQa::AudioQaReport after_report;
    AudioQa::AudioQaDiff diff;
    bool verified = false;
    Vector<String> warnings;
    
    AutomationExportResult() = default;
    
    bool operator==(const AutomationExportResult& other) const {
        return target == other.target &&
               output_dir == other.output_dir &&
               written_files == other.written_files &&
               before_report == other.before_report &&
               after_report == other.after_report &&
               diff == other.diff &&
               verified == other.verified &&
               warnings == other.warnings;
    }
};

class AutomationExport {
public:
    // Main export methods
    static Result<AutomationClip> BuildClipFromIntentPlan(
        const IntentPlan& plan,
        double duration_sec,
        int sample_rate
    );

    static Result<AutomationClip> BuildClipFromIntentSession(
        const IntentSessionState& session,
        Optional<int> up_to_step,        // -1 = applied head
        double duration_sec,
        int sample_rate,
        bool require_accepted
    );

    static Result<AutomationClip> BuildClipFromIntentProfile(
        const IntentProfile& profile,
        double duration_sec,
        int sample_rate
    );

    static Result<AutomationExportResult> ExportClip(
        const AutomationClip& clip,
        AutomationTargetKind target,
        const String& output_dir,
        bool verify_with_audio_qa,
        Optional<String> plugin_path = Null,    // for live verification via PluginHostHarness
        Optional<String> plugin_id = Null
    );

    // Verification utilities
    static Result<AudioQa::AudioQaReport> ApplyAutomationAndAnalyze(
        const InstrumentGraph& instrument,
        const AutomationClip& clip,
        double sample_rate_hz = 48000.0,
        double duration_sec = 1.0
    );

    // Helper for plugin automation verification
    static Result<HostRunResult> ApplyAutomationToPlugin(
        const String& plugin_path,
        const String& plugin_id,
        HostPluginKind plugin_kind,
        const AutomationClip& clip,
        double sample_rate_hz = 48000.0,
        double duration_sec = 1.0
    );
};

// CircuitFacade integration methods
class CircuitFacade;
Result<AutomationClip> ExportAutomationFromIntentPlan(
    const IntentPlan& plan,
    double duration_sec,
    int sample_rate
);

Result<AutomationClip> ExportAutomationFromIntentSessionInBranch(
    CircuitFacade& facade,
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& intent_session_id,
    Optional<int> up_to_step,        // -1 = applied head
    double duration_sec,
    int sample_rate,
    bool require_accepted
);

Result<AutomationClip> ExportAutomationFromIntentProfileInBranch(
    CircuitFacade& facade,
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& profile_id,
    double duration_sec,
    int sample_rate
);

} // namespace ProtoVMCLI

#endif // _PROTOVM_AUTOMATIONEXPORT_H