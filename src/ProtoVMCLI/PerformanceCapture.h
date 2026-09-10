#ifndef PROTOVM_PERFORMANCE_CAPTURE_H
#define PROTOVM_PERFORMANCE_CAPTURE_H

#include <Upp/Upp.h>
#include "ControlMapping.h"
#include "InstrumentRuntime.h"
#include "AutomationExport.h"
#include "IntentSessions.h"
#include "InstrumentGraph.h"
#include "JsonIO.h"

NAMESPACE_PROTOVMCLI

// Represents a single captured performance event
struct PerformanceEvent {
    double time_sec;
    ControlSource source;
    double normalized_value;
    
    PerformanceEvent() : time_sec(0.0), normalized_value(0.0) {}
    PerformanceEvent(double time_sec, const ControlSource& source, double normalized_value)
        : time_sec(time_sec), source(source), normalized_value(normalized_value) {}
};

// Container for a complete performance recording
struct PerformanceRecording {
    String recording_id;
    String instrument_id;
    int sample_rate;
    double duration_sec;
    
    Vector<PerformanceEvent> events;
    
    // Optional derived artifacts:
    Optional<AutomationClip> derived_automation;
    Optional<IntentSessionState> derived_intent_session;
    
    PerformanceRecording()
        : sample_rate(44100), duration_sec(0.0) {}
};

// Thread-safe performance capture engine
class PerformanceCaptureEngine {
private:
    static volatile bool capturing;
    static Vector<PerformanceEvent> captured_events;
    static double start_time_sec;
    static int sample_rate;
    static String instrument_id;
    
public:
    // Start capturing performance data
    static Result<void> BeginCapture(InstrumentRuntime& runtime, int sample_rate);
    
    // Capture a control event during performance
    static Result<void> CaptureControlEvent(const ControlSource& source, 
                                           double normalized_value, 
                                           double time_sec);
    
    // Stop capturing and return the performance recording
    static Result<PerformanceRecording> EndCapture(double duration_sec);
    
    // Check if capture is currently active
    static bool IsCapturing();
    
    // Get reference to captured events for internal use
    static const Vector<PerformanceEvent>& GetEvents();
    
    // Clear all captured events
    static void ClearEvents();
};

// Derive automation from performance data
Result<AutomationClip> DeriveAutomationFromPerformance(
    const PerformanceRecording& recording,
    double smoothing_sec = 0.01,
    double min_delta = 0.001
);

// Heuristic mapping: convert performance to intent session
Result<IntentSessionState> DeriveIntentSessionFromPerformance(
    const PerformanceRecording& recording,
    const InstrumentGraph& base_instrument
);

END_NAMESPACE

#endif