#include "PerformanceCapture.h"
#include <algorithm>
#include <cmath>

NAMESPACE_PROTOVMCLI

// Static member definitions
volatile bool PerformanceCaptureEngine::capturing = false;
Vector<PerformanceEvent> PerformanceCaptureEngine::captured_events;
double PerformanceCaptureEngine::start_time_sec = 0.0;
int PerformanceCaptureEngine::sample_rate = 44100;
String PerformanceCaptureEngine::instrument_id;

Result<void> PerformanceCaptureEngine::BeginCapture(InstrumentRuntime& runtime, int sample_rate_arg) {
    if (capturing) {
        return Result<void>::Error("Performance capture already in progress");
    }
    
    capturing = true;
    sample_rate = sample_rate_arg;
    captured_events.Clear();
    start_time_sec = 0.0; // Will be set to actual start time when first event occurs
    instrument_id = runtime.GetInstrumentId();
    
    LOG("Performance capture started for instrument: " << instrument_id);
    return Result<void>::Success();
}

Result<void> PerformanceCaptureEngine::CaptureControlEvent(const ControlSource& source, 
                                                          double normalized_value, 
                                                          double time_sec) {
    if (!capturing) {
        return Result<void>::Error("No active performance capture");
    }
    
    // Make sure time is non-decreasing for deterministic capture
    if (captured_events.GetCount() > 0) {
        double last_time = captured_events.Top().time_sec;
        if (time_sec < last_time) {
            time_sec = last_time; // Clamp to previous time to maintain order
        }
    } else {
        start_time_sec = time_sec; // Set the start time to first event time
    }
    
    PerformanceEvent event(time_sec, source, normalized_value);
    captured_events.Add(event);
    
    return Result<void>::Success();
}

Result<PerformanceRecording> PerformanceCaptureEngine::EndCapture(double duration_sec) {
    if (!capturing) {
        return Result<PerformanceRecording>::Error("No active performance capture");
    }
    
    PerformanceRecording recording;
    recording.recording_id = Upp::AsString(TimeStop().GetSystemTimeHash());
    recording.instrument_id = instrument_id;
    recording.sample_rate = sample_rate;
    recording.duration_sec = duration_sec;
    recording.events = captured_events;  // Copy events
    
    capturing = false;
    captured_events.Clear();  // Clear static storage
    
    LOG("Performance capture ended. Recorded " << recording.events.GetCount() << " events over " 
         << duration_sec << " seconds.");
    
    return Result<PerformanceRecording>::Success(recording);
}

bool PerformanceCaptureEngine::IsCapturing() {
    return capturing;
}

const Vector<PerformanceEvent>& PerformanceCaptureEngine::GetEvents() {
    return captured_events;
}

void PerformanceCaptureEngine::ClearEvents() {
    captured_events.Clear();
}

Result<AutomationClip> DeriveAutomationFromPerformance(
    const PerformanceRecording& recording,
    double smoothing_sec,
    double min_delta) {
    
    if (recording.events.IsEmpty()) {
        return Result<AutomationClip>::Error("No events in recording to derive automation from");
    }
    
    // Group events by ControlSource
    Map<String, Vector<PerformanceEvent>> grouped_events;
    
    for (const auto& event : recording.events) {
        String source_key = event.source.name;  // Using name as identifier
        grouped_events.GetAdd(source_key).Add(event);
    }
    
    AutomationClip clip;
    clip.id = "derived_" + recording.recording_id;
    clip.duration_samples = (int)(recording.duration_sec * recording.sample_rate);
    
    for (auto& pair : grouped_events) {
        String control_name = pair.key;
        Vector<PerformanceEvent>& events = pair.value;
        
        // Sort events by time to ensure chronological order
        events.Sort([](const PerformanceEvent& a, const PerformanceEvent& b) {
            return a.time_sec < b.time_sec;
        });
        
        // Build automation points
        Vector<AutomationPoint> automation_points;
        
        for (const auto& event : events) {
            // Convert time from seconds to samples
            int sample_pos = (int)(event.time_sec * recording.sample_rate);
            
            // Apply minimum delta filtering to reduce noise
            if (automation_points.IsEmpty() || 
                std::abs(automation_points.Top().value - event.normalized_value) >= min_delta) {
                
                AutomationPoint point;
                point.position_sample = sample_pos;
                point.value = event.normalized_value;
                point.mode = AutomationMode::LINEAR;  // Default to linear interpolation
                
                automation_points.Add(point);
            }
        }
        
        // Apply smoothing if specified
        if (smoothing_sec > 0.0) {
            // Apply a basic smoothing algorithm to reduce jitter
            Vector<AutomationPoint> smoothed_points;
            
            for (int i = 0; i < automation_points.GetCount(); i++) {
                auto& current_point = automation_points[i];
                
                // Calculate smoothing window in samples
                int smoothing_window_samples = (int)(smoothing_sec * recording.sample_rate);
                
                // Find points within smoothing window
                double sum = current_point.value;
                int count = 1;
                
                // Look at nearby points for smoothing
                for (int j = std::max(0, i - 2); j < automation_points.GetCount() && j < i + 3; j++) {
                    if (j != i) {
                        int time_diff = std::abs(automation_points[j].position_sample - current_point.position_sample);
                        if (time_diff <= smoothing_window_samples) {
                            sum += automation_points[j].value;
                            count++;
                        }
                    }
                }
                
                AutomationPoint smoothed_point = current_point;
                smoothed_point.value = sum / count;
                
                // Only add if significantly different from last (to reduce density)
                if (smoothed_points.IsEmpty() || 
                    std::abs(smoothed_points.Top().value - smoothed_point.value) >= min_delta) {
                    smoothed_points.Add(smoothed_point);
                }
            }
            
            automation_points = smoothed_points;
        }
        
        // Create automation lane for this control
        AutomationLane lane;
        lane.control_identifier = control_name;
        lane.points = automation_points;
        lane.enabled = true;
        
        clip.lanes.Add(lane);
    }
    
    return Result<AutomationClip>::Success(clip);
}

Result<IntentSessionState> DeriveIntentSessionFromPerformance(
    const PerformanceRecording& recording,
    const InstrumentGraph& base_instrument) {
    
    IntentSessionState intent_session;
    intent_session.session_id = "intent_from_performance_" + recording.recording_id;
    intent_session.timestamp = Upp::AsString(TimeStop().GetSystemTime());
    
    if (recording.events.IsEmpty()) {
        return Result<IntentSessionState>::Success(intent_session);
    }
    
    // Group events by ControlSource
    Map<String, Vector<PerformanceEvent>> grouped_events;
    
    for (const auto& event : recording.events) {
        String source_key = event.source.name;  // Using name as identifier
        grouped_events.GetAdd(source_key).Add(event);
    }
    
    // Process each control separately
    for (auto& pair : grouped_events) {
        String control_name = pair.key;
        Vector<PerformanceEvent>& events = pair.value;
        
        // Sort events by time
        events.Sort([](const PerformanceEvent& a, const PerformanceEvent& b) {
            return a.time_sec < b.time_sec;
        });
        
        // Detect if this control had significant movement
        double min_val = 1.0, max_val = 0.0;
        for (const auto& event : events) {
            min_val = std::min(min_val, event.normalized_value);
            max_val = std::max(max_val, event.normalized_value);
        }
        
        // If there was significant movement, consider adding intent
        if (max_val - min_val > 0.1) {  // Threshold for "significant" movement
            // Look for patterns:
            // 1. Stabilizing values (suggesting intent proposals)
            // 2. Continuous sweeping (suggesting morph sweeps)
            
            // Detect stable regions - where value stays roughly the same for a period
            Vector<std::pair<double, double>> stable_regions; // (time, value)
            
            for (int i = 0; i < events.GetCount(); i += std::max(1, events.GetCount()/10)) { // Sample 10 points
                stable_regions.Add(std::make_pair(events[i].time_sec, events[i].normalized_value));
            }
            
            // Create a proposal based on detected behaviors
            IntentProposal proposal;
            proposal.proposal_id = "proposal_for_" + control_name;
            proposal.proposal_type = "control_derived";
            proposal.description = "Derived from performance capture of control: " + control_name;
            proposal.timestamp = Upp::AsString(TimeStop().GetSystemTime());
            
            // Create a parameter adjustment intent
            ParameterAdjustmentIntent param_intent;
            param_intent.parameter_name = control_name;
            param_intent.target_value = events.Top().normalized_value; // Use final value
            param_intent.start_time_sec = events[0].time_sec;
            param_intent.end_time_sec = events.Top().time_sec;
            
            proposal.intents.Add(param_intent);
            intent_session.proposals.Add(proposal);
        }
    }
    
    return Result<IntentSessionState>::Success(intent_session);
}

END_NAMESPACE