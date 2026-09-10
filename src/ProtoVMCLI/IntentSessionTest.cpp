#include "IntentSessions.h"
#include "InstrumentGraph.h"
#include "MusicalIntent.h"
#include <iostream>
#include <cassert>

using namespace ProtoVMCLI;

void TestCreateIntentSession() {
    std::cout << "Testing Create IntentSession..." << std::endl;
    
    // Create a basic instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.sample_rate_hz = 48000;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;  // A4
    instrument.note.velocity = 0.8;
    instrument.note.duration_sec = 1.0;
    
    // Create a session in memory (not persisted)
    auto result = IntentSessionManager::Create("./test_session", "main", instrument, "test_user");
    
    if (result.ok) {
        std::cout << "✓ Intent session created successfully with ID: " << result.data.meta.intent_session_id << std::endl;
        assert(result.data.meta.intent_session_id.StartsWith("is-"));
        assert(result.data.current_instrument.instrument_id == "test_instrument");
        std::cout << "✓ All assertions passed" << std::endl;
    } else {
        std::cout << "✗ Intent session creation failed: " << result.error_message << std::endl;
    }
}

void TestProposeAndApplyStep() {
    std::cout << "Testing Propose & Apply Step..." << std::endl;
    
    // Create a basic instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.sample_rate_hz = 48000;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;
    instrument.note.velocity = 0.8;
    instrument.note.duration_sec = 1.0;
    
    // First create a session
    auto session_result = IntentSessionManager::Create("./test_session", "main", instrument, "test_user");
    assert(session_result.ok);
    IntentSessionState session = session_result.data;
    
    // Create an intent request
    IntentRequest req;
    req.intents.push_back(IntentKind::Warmer);
    req.strength = 0.5;
    req.duration_sec = 1.0;
    
    // Propose a step (this should add it to the steps but not apply it)
    auto propose_result = IntentSessionManager::ProposeStep(session, req, "test_user");
    
    if (propose_result.ok) {
        IntentSessionState updated_session = propose_result.data;
        std::cout << "✓ Step proposed successfully. Step count: " << updated_session.steps.GetCount() << std::endl;
        assert(updated_session.steps.GetCount() == 1);
        assert(updated_session.steps[0].applied == false);
        assert(updated_session.steps[0].step_index == 0);
        std::cout << "✓ All proposal assertions passed" << std::endl;
        
        // Now apply the step
        auto apply_result = IntentSessionManager::ApplyStep(updated_session, 0, false); // require_accepted = false for test
        if (apply_result.ok) {
            IntentSessionState applied_session = apply_result.data;
            std::cout << "✓ Step applied successfully. Head index: " << applied_session.meta.head_step_index << std::endl;
            assert(applied_session.meta.head_step_index == 0);
            assert(applied_session.steps[0].applied == true);
            std::cout << "✓ All apply assertions passed" << std::endl;
        } else {
            std::cout << "✗ Step application failed: " << apply_result.error_message << std::endl;
        }
    } else {
        std::cout << "✗ Step proposal failed: " << propose_result.error_message << std::endl;
    }
}

void TestUndoRedo() {
    std::cout << "Testing Undo/Redo..." << std::endl;
    
    // Create a basic instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.sample_rate_hz = 48000;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;
    instrument.note.velocity = 0.8;
    instrument.note.duration_sec = 1.0;
    
    // First create a session
    auto session_result = IntentSessionManager::Create("./test_session", "main", instrument, "test_user");
    assert(session_result.ok);
    IntentSessionState session = session_result.data;
    
    // Create and apply multiple steps
    IntentRequest req1;
    req1.intents.push_back(IntentKind::Warmer);
    req1.strength = 0.3;
    req1.duration_sec = 1.0;
    
    auto propose_result1 = IntentSessionManager::ProposeStep(session, req1, "test_user");
    assert(propose_result1.ok);
    
    IntentSessionState session_with_step1 = propose_result1.data;
    auto apply_result1 = IntentSessionManager::ApplyStep(session_with_step1, 0, false);
    assert(apply_result1.ok);
    IntentSessionState session_after_step1 = apply_result1.data;
    
    // Add another step
    IntentRequest req2;
    req2.intents.push_back(IntentKind::Brighter);
    req2.strength = 0.4;
    req2.duration_sec = 1.0;
    
    auto propose_result2 = IntentSessionManager::ProposeStep(session_after_step1, req2, "test_user");
    assert(propose_result2.ok);
    
    IntentSessionState session_with_step2 = propose_result2.data;
    auto apply_result2 = IntentSessionManager::ApplyStep(session_with_step2, 1, false);
    assert(apply_result2.ok);
    IntentSessionState session_after_step2 = apply_result2.data;
    
    std::cout << "✓ Two steps applied. Head index: " << session_after_step2.meta.head_step_index << std::endl;
    assert(session_after_step2.meta.head_step_index == 1);
    
    // Test undo
    auto undo_result = IntentSessionManager::Undo(session_after_step2);
    if (undo_result.ok) {
        IntentSessionState after_undo = undo_result.data;
        std::cout << "✓ Undo successful. Head index after undo: " << after_undo.meta.head_step_index << std::endl;
        assert(after_undo.meta.head_step_index == 0); // Now only the first step should be applied
        std::cout << "✓ Undo assertion passed" << std::endl;
        
        // Test redo
        auto redo_result = IntentSessionManager::Redo(after_undo);
        if (redo_result.ok) {
            IntentSessionState after_redo = redo_result.data;
            std::cout << "✓ Redo successful. Head index after redo: " << after_redo.meta.head_step_index << std::endl;
            assert(after_redo.meta.head_step_index == 1); // Back to both steps applied
            std::cout << "✓ Redo assertion passed" << std::endl;
        } else {
            std::cout << "✗ Redo failed: " << redo_result.error_message << std::endl;
        }
    } else {
        std::cout << "✗ Undo failed: " << undo_result.error_message << std::endl;
    }
}

void TestReplay() {
    std::cout << "Testing Replay..." << std::endl;
    
    // Create a basic instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.sample_rate_hz = 48000;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;
    instrument.note.velocity = 0.8;
    instrument.note.duration_sec = 1.0;
    
    // First create a session
    auto session_result = IntentSessionManager::Create("./test_session", "main", instrument, "test_user");
    assert(session_result.ok);
    IntentSessionState session = session_result.data;
    
    // Create and apply a few steps
    IntentRequest req1;
    req1.intents.push_back(IntentKind::Warmer);
    req1.strength = 0.3;
    req1.duration_sec = 1.0;
    
    auto propose_result1 = IntentSessionManager::ProposeStep(session, req1, "test_user");
    assert(propose_result1.ok);
    IntentSessionState session_with_step1 = propose_result1.data;
    auto apply_result1 = IntentSessionManager::ApplyStep(session_with_step1, 0, false);
    assert(apply_result1.ok);
    
    IntentRequest req2;
    req2.intents.push_back(IntentKind::Brighter);
    req2.strength = 0.4;
    req2.duration_sec = 1.0;
    
    auto propose_result2 = IntentSessionManager::ProposeStep(apply_result1.data, req2, "test_user");
    assert(propose_result2.ok);
    IntentSessionState session_with_step2 = propose_result2.data;
    auto apply_result2 = IntentSessionManager::ApplyStep(session_with_step2, 1, false);
    assert(apply_result2.ok);
    
    IntentSessionState final_session = apply_result2.data;
    std::cout << "✓ Created session with two applied steps" << std::endl;
    
    // Create a different initial instrument
    InstrumentGraph target_instrument;
    target_instrument.instrument_id = "target_instrument";
    target_instrument.sample_rate_hz = 48000;
    target_instrument.voice_count = 2;  // Different from original
    target_instrument.note.base_freq_hz = 880.0;  // Different from original (A5 instead of A4)
    target_instrument.note.velocity = 0.9;
    target_instrument.note.duration_sec = 2.0;
    
    // Replay the same steps to the target instrument
    auto replay_result = IntentSessionManager::ReplayStepsToInstrument(
        final_session,
        target_instrument,
        -1,  // All steps
        false  // Don't require accepted
    );
    
    if (replay_result.ok) {
        std::cout << "✓ Replay successful. Derived instrument ID: " << replay_result.data.instrument_id << std::endl;
        // The instrument should have the same structure as the target but with the applied modifications
        std::cout << "✓ Replay test completed" << std::endl;
    } else {
        std::cout << "✗ Replay failed: " << replay_result.error_message << std::endl;
    }
}

int main() {
    std::cout << "Running Intent Session Tests..." << std::endl;
    
    TestCreateIntentSession();
    std::cout << std::endl;
    
    TestProposeAndApplyStep();
    std::cout << std::endl;
    
    TestUndoRedo();
    std::cout << std::endl;
    
    TestReplay();
    std::cout << std::endl;
    
    std::cout << "Intent Session Tests Completed!" << std::endl;
    
    return 0;
}