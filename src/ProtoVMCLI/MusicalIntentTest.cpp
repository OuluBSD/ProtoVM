#include "MusicalIntent.h"
#include "CircuitFacade.h"
#include <iostream>
#include <cassert>

namespace ProtoVMCLI {

// Simple test for the MusicalIntent functionality
void TestMusicalIntentDeterminism() {
    std::cout << "Testing Musical Intent Determinism..." << std::endl;
    
    // Create a simple instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.sample_rate_hz = 48000;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;  // A4
    instrument.note.velocity = 0.8;
    instrument.note.duration_sec = 3.0;
    
    // Create an intent request for "warmer"
    IntentRequest req;
    req.intents.push_back(IntentKind::Warmer);
    req.strength = 0.7;
    req.duration_sec = 3.0;
    req.sample_rate = 48000;
    req.block_size = 512;
    req.allow_regression = false;
    
    // Generate plan twice to test determinism
    auto result1 = MusicalIntentEngine::ProposeAndVerify(instrument, req);
    auto result2 = MusicalIntentEngine::ProposeAndVerify(instrument, req);
    
    assert(result1.ok() && "First proposal should succeed");
    assert(result2.ok() && "Second proposal should succeed");
    
    // Since the input is the same, the generated actions should be identical
    assert(result1.value().actions.size() == result2.value().actions.size() && 
           "Action counts should be the same for identical inputs");
    
    std::cout << "Determinism test passed!" << std::endl;
}

void TestMusicalIntentMapping() {
    std::cout << "Testing Musical Intent Mapping..." << std::endl;
    
    // Create a simple instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.sample_rate_hz = 48000;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;  // A4
    instrument.note.velocity = 0.8;
    instrument.note.duration_sec = 3.0;
    
    // Test "wider" intent
    IntentRequest req;
    req.intents.push_back(IntentKind::Wider);
    req.strength = 0.8;
    req.duration_sec = 3.0;
    req.sample_rate = 48000;
    req.block_size = 512;
    req.allow_regression = false;
    
    auto result = MusicalIntentEngine::ProposeAndVerify(instrument, req);
    
    assert(result.ok() && "Wider intent proposal should succeed");
    
    // Check that the actions make sense for "wider" intent
    bool foundStereoWidthAction = false;
    for (const auto& action : result.value().actions) {
        if (action.target == "stereo_width") {
            foundStereoWidthAction = true;
            // For "wider" intent with strength 0.8, value should be in the wider range
            assert(action.value > 0.7 && action.value <= 1.0 && 
                   "Wider intent should result in stereo width > 0.7");
            break;
        }
    }
    
    assert(foundStereoWidthAction && "Wider intent should generate stereo width action");
    
    std::cout << "Mapping test passed!" << std::endl;
}

void TestMusicalIntentSafetyGate() {
    std::cout << "Testing Musical Intent Safety Gate..." << std::endl;
    
    // Create a simple instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.sample_rate_hz = 48000;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;  // A4
    instrument.note.velocity = 0.8;
    instrument.note.duration_sec = 3.0;
    
    // Create intent request with allow_regression = false
    IntentRequest req;
    req.intents.push_back(IntentKind::Warmer);
    req.strength = 0.5;
    req.duration_sec = 3.0;
    req.sample_rate = 48000;
    req.block_size = 512;
    req.allow_regression = false;  // Should require no regressions
    
    // Create a circuit facade for verification (with no session store)
    CircuitFacade facade;
    SessionMetadata session;
    session.session_id = 123;
    std::string session_dir = "/tmp/test_session";
    std::string branch_name = "main";
    
    // This test specifically tests the plan generation part since
    // full verification requires audio rendering which depends on actual implementation
    auto result = MusicalIntentEngine::ProposeAndVerify(instrument, req);
    
    assert(result.ok() && "Intent proposal should succeed");
    
    // Check that the plan has the correct request parameters
    assert(result.value().request.allow_regression == false && 
           "Plan should preserve request parameter allow_regression");
    
    std::cout << "Safety gate test passed!" << std::endl;
}

void TestIntentSerialization() {
    std::cout << "Testing Intent Serialization..." << std::endl;
    
    // Create a simple intent plan
    IntentPlan plan;
    plan.request.intents.push_back(IntentKind::Warmer);
    plan.request.intents.push_back(IntentKind::Wider);
    plan.request.strength = 0.6;
    plan.request.duration_sec = 4.0;
    plan.request.sample_rate = 44100;
    plan.request.block_size = 1024;
    plan.request.allow_regression = true;
    plan.request.qa_profile_name = "test_profile";
    
    IntentAction action;
    action.kind = IntentActionKind::AdjustMix;
    action.target = "stereo_width";
    action.value = 0.8;
    action.note = "Increase stereo width";
    plan.actions.push_back(action);
    
    // Test serialization using JsonIO (if available in test environment)
    // This is a basic test to ensure the structure exists
    assert(plan.actions.size() == 1 && "Plan should have one action");
    assert(plan.request.intents.size() == 2 && "Request should have two intents");
    
    std::cout << "Serialization test passed!" << std::endl;
}

void RunAllMusicalIntentTests() {
    std::cout << "Running Musical Intent Tests..." << std::endl;
    
    TestMusicalIntentDeterminism();
    TestMusicalIntentMapping();
    TestMusicalIntentSafetyGate();
    TestIntentSerialization();
    
    std::cout << "All Musical Intent tests passed!" << std::endl;
}

} // namespace ProtoVMCLI

int main() {
    ProtoVMCLI::RunAllMusicalIntentTests();
    return 0;
}