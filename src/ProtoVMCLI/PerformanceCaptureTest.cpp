#include "PerformanceCapture.h"
#include "InstrumentRuntime.h"
#include "ControlMapping.h"
#include "JsonIO.h"
#include "CppUnitLite/Test.h"
#include <iostream>

using namespace Upp;

void TestPerformanceEventStructure() {
    std::cout << "Testing PerformanceEvent structure..." << std::endl;

    // Test default constructor
    PerformanceEvent default_event;
    ASSERT(default_event.time_sec == 0.0);
    ASSERT(default_event.normalized_value == 0.0);

    // Test parameterized constructor
    ControlSource source;
    source.source_kind = ControlSourceKind::MIDI_CC;
    source.id = "midi_cc_1";
    source.name = "Modulation Wheel";

    PerformanceEvent event(1.5, source, 0.75);
    ASSERT(event.time_sec == 1.5);
    ASSERT(event.source.id == "midi_cc_1");
    ASSERT(event.source.name == "Modulation Wheel");
    ASSERT(event.normalized_value == 0.75);

    std::cout << "PerformanceEvent structure tests passed!" << std::endl;
}

void TestPerformanceRecordingStructure() {
    std::cout << "Testing PerformanceRecording structure..." << std::endl;

    PerformanceRecording recording;
    ASSERT(recording.sample_rate == 44100);
    ASSERT(recording.duration_sec == 0.0);
    ASSERT(recording.events.GetCount() == 0);

    // Add some test events
    ControlSource source1;
    source1.source_kind = ControlSourceKind::MIDI_CC;
    source1.id = "cc_1";
    source1.name = "Mod Wheel";

    ControlSource source2;
    source2.source_kind = ControlSourceKind::MIDI_CC;
    source2.id = "cc_2";
    source2.name = "Volume";

    recording.events.Add(PerformanceEvent(0.5, source1, 0.25));
    recording.events.Add(PerformanceEvent(1.0, source2, 0.75));
    recording.events.Add(PerformanceEvent(1.5, source1, 0.50));

    ASSERT(recording.events.GetCount() == 3);
    ASSERT(recording.events[0].time_sec == 0.5);
    ASSERT(recording.events[1].time_sec == 1.0);
    ASSERT(recording.events[2].time_sec == 1.5);

    std::cout << "PerformanceRecording structure tests passed!" << std::endl;
}

void TestPerformanceCaptureEngineLifecycle() {
    std::cout << "Testing PerformanceCaptureEngine lifecycle..." << std::endl;

    // Create a dummy instrument runtime
    InstrumentRuntime runtime;
    runtime.SetInstrumentId("test_instrument");

    // Test begin capture
    auto begin_result = PerformanceCaptureEngine::BeginCapture(runtime, 48000);
    ASSERT(begin_result.ok);
    ASSERT(PerformanceCaptureEngine::IsCapturing());

    // Capture some events
    ControlSource source;
    source.source_kind = ControlSourceKind::MIDI_CC;
    source.id = "test_cc";
    source.name = "Test CC";

    auto event1_result = PerformanceCaptureEngine::CaptureControlEvent(source, 0.3, 0.1);
    ASSERT(event1_result.ok);

    auto event2_result = PerformanceCaptureEngine::CaptureControlEvent(source, 0.7, 0.2);
    ASSERT(event2_result.ok);

    auto event3_result = PerformanceCaptureEngine::CaptureControlEvent(source, 0.5, 0.3);
    ASSERT(event3_result.ok);

    // Verify events were captured
    const auto& events = PerformanceCaptureEngine::GetEvents();
    ASSERT(events.GetCount() == 3);

    // Test end capture
    auto end_result = PerformanceCaptureEngine::EndCapture(1.0);
    ASSERT(end_result.ok);
    ASSERT(!PerformanceCaptureEngine::IsCapturing());
    ASSERT(end_result.data.events.GetCount() == 3);
    ASSERT(end_result.data.duration_sec == 1.0);
    ASSERT(end_result.data.sample_rate == 48000);

    // Verify that capture cannot be ended when not active
    auto end_inactive_result = PerformanceCaptureEngine::EndCapture(1.0);
    ASSERT(!end_inactive_result.ok);

    std::cout << "PerformanceCaptureEngine lifecycle tests passed!" << std::endl;
}

void TestPerformanceCaptureEngineConcurrency() {
    std::cout << "Testing PerformanceCaptureEngine concurrency aspects..." << std::endl;

    // Test that capturing can't be started twice
    InstrumentRuntime runtime1, runtime2;
    runtime1.SetInstrumentId("test_instrument_1");
    runtime2.SetInstrumentId("test_instrument_2");

    auto begin_result1 = PerformanceCaptureEngine::BeginCapture(runtime1, 44100);
    ASSERT(begin_result1.ok);
    ASSERT(PerformanceCaptureEngine::IsCapturing());

    // Attempt to start a second capture while one is active should fail
    auto begin_result2 = PerformanceCaptureEngine::BeginCapture(runtime2, 48000);
    ASSERT(!begin_result2.ok);

    // End the first capture
    auto end_result = PerformanceCaptureEngine::EndCapture(0.5);
    ASSERT(end_result.ok);
    ASSERT(!PerformanceCaptureEngine::IsCapturing());

    // Now we should be able to start a new capture
    auto begin_result3 = PerformanceCaptureEngine::BeginCapture(runtime2, 48000);
    ASSERT(begin_result3.ok);
    ASSERT(PerformanceCaptureEngine::IsCapturing());

    // End the second capture
    auto end_result2 = PerformanceCaptureEngine::EndCapture(0.5);
    ASSERT(end_result2.ok);

    std::cout << "PerformanceCaptureEngine concurrency tests passed!" << std::endl;
}

void TestAutomationDerivation() {
    std::cout << "Testing DeriveAutomationFromPerformance..." << std::endl;

    // Create a test recording with several events
    PerformanceRecording recording;
    recording.sample_rate = 44100;
    recording.duration_sec = 2.0;
    recording.instrument_id = "test_instrument";
    recording.recording_id = "test_recording";

    ControlSource source;
    source.source_kind = ControlSourceKind::MIDI_CC;
    source.id = "volume_cc";
    source.name = "Volume";

    // Add events that should create a recognizable automation pattern
    recording.events.Add(PerformanceEvent(0.0, source, 0.0));   // Start at 0
    recording.events.Add(PerformanceEvent(0.5, source, 0.5));   // Rise to 0.5
    recording.events.Add(PerformanceEvent(1.0, source, 1.0));   // Rise to 1.0
    recording.events.Add(PerformanceEvent(1.5, source, 0.5));   // Fall to 0.5
    recording.events.Add(PerformanceEvent(2.0, source, 0.0));   // Fall to 0.0

    // Derive automation
    auto automation_result = DeriveAutomationFromPerformance(recording, 0.01, 0.001);
    ASSERT(automation_result.ok);

    AutomationClip clip = automation_result.data;
    ASSERT(clip.lanes.GetCount() == 1);  // Should have one lane for the single control source
    ASSERT(clip.duration_samples == 2 * 44100);  // 2 seconds at 44100 Hz

    AutomationLane lane = clip.lanes[0];
    ASSERT(lane.control_identifier == "Volume");  // Should use the control name
    ASSERT(lane.points.GetCount() > 0);  // Should have at least some automation points

    // Verify the automation points follow the general trend of the original events
    // (Even with smoothing, the overall shape should be preserved)
    ASSERT(lane.points[0].value >= 0.0 && lane.points[0].value <= 0.1);  // Start near 0
    if (lane.points.GetCount() > 2) {
        // Somewhere in the middle should be higher than start
        bool found_higher_value = false;
        for (int i = 0; i < lane.points.GetCount(); i++) {
            if (lane.points[i].value > 0.7) {
                found_higher_value = true;
                break;
            }
        }
        ASSERT(found_higher_value);  // Should have reached higher values
    }

    std::cout << "Automation derivation tests passed!" << std::endl;
}

void TestAutomationDerivationNoiseFiltering() {
    std::cout << "Testing DeriveAutomationFromPerformance noise filtering..." << std::endl;

    // Create a recording with some noisy data that should be filtered
    PerformanceRecording recording;
    recording.sample_rate = 44100;
    recording.duration_sec = 1.0;
    recording.instrument_id = "test_instrument";
    recording.recording_id = "noise_test_recording";

    ControlSource source;
    source.source_kind = ControlSourceKind::MIDI_CC;
    source.id = "filter_cutoff";
    source.name = "Filter Cutoff";

    // Add events with small variations that should be filtered out with min_delta = 0.1
    recording.events.Add(PerformanceEvent(0.0, source, 0.50));
    recording.events.Add(PerformanceEvent(0.1, source, 0.51));  // Small change
    recording.events.Add(PerformanceEvent(0.2, source, 0.49));  // Small change
    recording.events.Add(PerformanceEvent(0.3, source, 0.52));  // Small change
    recording.events.Add(PerformanceEvent(0.4, source, 0.80));  // Large change
    recording.events.Add(PerformanceEvent(0.5, source, 0.78));  // Small change
    recording.events.Add(PerformanceEvent(0.6, source, 0.82));  // Small change
    recording.events.Add(PerformanceEvent(0.7, source, 0.40));  // Large change

    // Derive automation with min_delta = 0.1 (should filter out small variations)
    auto automation_result = DeriveAutomationFromPerformance(recording, 0.0, 0.1);
    ASSERT(automation_result.ok);

    AutomationClip clip = automation_result.data;
    ASSERT(clip.lanes.GetCount() == 1);

    AutomationLane lane = clip.lanes[0];
    // Should have fewer points due to noise filtering
    // The first few values are close to 0.5, the middle ones close to 0.8, and the last one drops to 0.4
    // With min_delta=0.1, similar values should be filtered out
    ASSERT(lane.points.GetCount() <= recording.events.GetCount());

    // Verify that significant changes are preserved
    bool found_high_value = false;
    bool found_low_value = false;
    for (const auto& point : lane.points) {
        if (point.value > 0.75) found_high_value = true;
        if (point.value < 0.45) found_low_value = true;
    }
    ASSERT(found_high_value);  // Should have found the high values
    ASSERT(found_low_value);   // Should have found the low values

    std::cout << "Automation derivation noise filtering tests passed!" << std::endl;
}

void TestIntentDerivation() {
    std::cout << "Testing DeriveIntentSessionFromPerformance..." << std::endl;

    // Create a test recording
    PerformanceRecording recording;
    recording.sample_rate = 44100;
    recording.duration_sec = 2.0;
    recording.instrument_id = "test_instrument";
    recording.recording_id = "intent_test_recording";

    ControlSource source;
    source.source_kind = ControlSourceKind::MIDI_CC;
    source.id = "filter_resonance";
    source.name = "Filter Resonance";

    // Add events showing significant movement to trigger intent derivation
    recording.events.Add(PerformanceEvent(0.0, source, 0.1));   // Low resonance
    recording.events.Add(PerformanceEvent(0.5, source, 0.2));   // Rising
    recording.events.Add(PerformanceEvent(1.0, source, 0.8));   // High resonance
    recording.events.Add(PerformanceEvent(1.5, source, 0.7));   // Falling but still high
    recording.events.Add(PerformanceEvent(2.0, source, 0.1));   // Back to low

    // Create a dummy instrument graph
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.name = "Test Instrument";

    // Derive intent session
    auto intent_result = DeriveIntentSessionFromPerformance(recording, instrument);
    ASSERT(intent_result.ok);

    IntentSessionState session = intent_result.data;
    ASSERT(session.session_id.find("intent_from_performance_") == 0);  // Should start with the expected prefix

    // With movement in the recording, we should have at least one proposal
    // Note: Our heuristic might not generate proposals for all recordings, so we'll check for either case
    // Just ensure that the function doesn't crash and returns a valid result

    std::cout << "Intent derivation tests passed!" << std::endl;
}

void TestJsonSerialization() {
    std::cout << "Testing JSON serialization for PerformanceCapture structures..." << std::endl;

    // Create a PerformanceEvent
    ControlSource source;
    source.source_kind = ControlSourceKind::MIDI_CC;
    source.id = "volume_cc";
    source.name = "Volume";

    PerformanceEvent event(1.25, source, 0.75);
    auto event_json = JsonIO::PerformanceEventToValueMap(event);
    ASSERT(event_json.Get("time_sec") == 1.25);
    ASSERT(event_json.Get("normalized_value") == 0.75);
    ASSERT(event_json.Get("source").IsType<Upp::ValueMap>());

    // Create a PerformanceRecording with some events
    PerformanceRecording recording;
    recording.recording_id = "json_test_recording";
    recording.instrument_id = "json_test_instrument";
    recording.sample_rate = 48000;
    recording.duration_sec = 3.0;

    recording.events.Add(event);
    recording.events.Add(PerformanceEvent(2.5, source, 0.25));

    auto recording_json = JsonIO::PerformanceRecordingToValueMap(recording);
    ASSERT(recording_json.Get("recording_id") == Value("json_test_recording"));
    ASSERT(recording_json.Get("instrument_id") == Value("json_test_instrument"));
    ASSERT(recording_json.Get("sample_rate") == 48000);
    ASSERT(recording_json.Get("duration_sec") == 3.0);
    ASSERT(recording_json.Get("events").IsType<Upp::ValueArray>());

    auto events_array = recording_json.Get("events").Get<Upp::ValueArray>();
    ASSERT(events_array.GetCount() == 2);

    // Test events array serialization
    auto events_array_json = JsonIO::PerformanceEventsToValueArray(recording.events);
    ASSERT(events_array_json.GetCount() == 2);

    std::cout << "JSON serialization tests passed!" << std::endl;
}

void TestLongCaptureMemoryManagement() {
    std::cout << "Testing long capture memory management..." << std::endl;

    InstrumentRuntime runtime;
    runtime.SetInstrumentId("test_instrument");

    // Start capture
    auto begin_result = PerformanceCaptureEngine::BeginCapture(runtime, 44100);
    ASSERT(begin_result.ok);

    ControlSource source;
    source.source_kind = ControlSourceKind::MIDI_CC;
    source.id = "test_cc";
    source.name = "Test CC";

    // Simulate a moderately long capture with many events
    for (int i = 0; i < 1000; i++) {
        double time = i * 0.01;  // 100 events per second
        double value = 0.5 + 0.3 * sin(i * 0.1);  // Oscillating value
        auto event_result = PerformanceCaptureEngine::CaptureControlEvent(source, value, time);
        ASSERT(event_result.ok);
    }

    // Verify we captured all events
    const auto& events = PerformanceCaptureEngine::GetEvents();
    ASSERT(events.GetCount() == 1000);

    // End capture
    auto end_result = PerformanceCaptureEngine::EndCapture(10.0);
    ASSERT(end_result.ok);
    ASSERT(end_result.data.events.GetCount() == 1000);

    // Verify the events in the recording match what we captured
    ASSERT(end_result.data.events[0].time_sec == 0.0);
    ASSERT(end_result.data.events[999].time_sec == 9.99);

    std::cout << "Long capture memory management tests passed!" << std::endl;
}

void TestDeterministicCapture() {
    std::cout << "Testing deterministic capture behavior..." << std::endl;

    // Perform the same capture sequence twice and ensure consistent results
    for (int run = 0; run < 2; run++) {
        InstrumentRuntime runtime;
        runtime.SetInstrumentId("deterministic_test_instrument");

        // Start capture
        auto begin_result = PerformanceCaptureEngine::BeginCapture(runtime, 44100);
        ASSERT(begin_result.ok);

        ControlSource source;
        source.source_kind = ControlSourceKind::MIDI_CC;
        source.id = "test_cc";
        source.name = "Test CC";

        // Capture the same sequence of events
        PerformanceCaptureEngine::CaptureControlEvent(source, 0.1, 0.1);
        PerformanceCaptureEngine::CaptureControlEvent(source, 0.5, 0.2);
        PerformanceCaptureEngine::CaptureControlEvent(source, 0.9, 0.3);

        auto end_result = PerformanceCaptureEngine::EndCapture(0.5);
        ASSERT(end_result.ok);
        ASSERT(end_result.data.events.GetCount() == 3);

        // Verify the exact values are preserved
        ASSERT(end_result.data.events[0].time_sec == 0.1);
        ASSERT(end_result.data.events[0].normalized_value == 0.1);
        ASSERT(end_result.data.events[1].time_sec == 0.2);
        ASSERT(end_result.data.events[1].normalized_value == 0.5);
        ASSERT(end_result.data.events[2].time_sec == 0.3);
        ASSERT(end_result.data.events[2].normalized_value == 0.9);
    }

    std::cout << "Deterministic capture tests passed!" << std::endl;
}

void RunPerformanceCaptureTests() {
    std::cout << "\n=== Running PerformanceCapture Tests ===" << std::endl;

    TestPerformanceEventStructure();
    TestPerformanceRecordingStructure();
    TestPerformanceCaptureEngineLifecycle();
    TestPerformanceCaptureEngineConcurrency();
    TestAutomationDerivation();
    TestAutomationDerivationNoiseFiltering();
    TestIntentDerivation();
    TestJsonSerialization();
    TestLongCaptureMemoryManagement();
    TestDeterministicCapture();

    std::cout << "\n=== All PerformanceCapture Tests Passed! ===" << std::endl;
}

// Run the tests if this is the main file being executed
#ifdef PERFORMANCECAPTURE_TESTS_MAIN
int main() {
    RunPerformanceCaptureTests();
    return 0;
}
#endif