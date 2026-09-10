#include "IntentProfiles.h"
#include "IntentSessions.h"
#include "JsonIO.h"
#include <iostream>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

namespace ProtoVMCLI {

// Test helper function to create test instrument
InstrumentGraph CreateTestInstrument() {
    InstrumentGraph instrument;
    instrument.instrument_id = "test-instrument";
    instrument.sample_rate_hz = 48000.0;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;  // A4
    instrument.note.velocity = 0.8;
    instrument.note.duration_sec = 3.0;
    return instrument;
}

// Test helper to create test session
IntentSessionState CreateTestSession() {
    InstrumentGraph test_inst = CreateTestInstrument();
    
    IntentSessionState session;
    session.current_instrument = test_inst;
    session.meta.intent_session_id = "is-test-session";
    
    // Add a test step
    IntentStepRecord step;
    step.step_index = 0;
    step.user_id = "test-user";
    step.request.intents = {IntentKind::Warmer};
    step.request.strength = 0.5;
    step.request.duration_sec = 3.0;
    step.request.sample_rate = 48000;
    step.request.block_size = 512;
    step.request.allow_regression = false;
    step.request.qa_profile_name = "default";
    step.note = "Test step";
    step.applied = true;
    
    // Fill in plan details
    step.plan.request = step.request;
    step.plan.accepted = true;
    step.plan.reasons.Add("Test reason");
    
    session.steps.Add(step);
    
    return session;
}

// Test IntentProfileManager functionality
void TestIntentProfileManager() {
    std::cout << "Testing IntentProfileManager..." << std::endl;
    
    // Create a temporary directory for testing
    fs::path temp_dir = fs::temp_directory_path() / "protovm_intent_profile_test";
    if (fs::exists(temp_dir)) {
        fs::remove_all(temp_dir);
    }
    fs::create_directories(temp_dir);
    
    // Create profile metadata
    IntentProfileMetadata meta;
    meta.profile_id = "ip-test-profile";
    meta.name = "Test Profile";
    meta.description = "A test profile for validation";
    meta.scope = IntentProfileScope::Workspace;
    meta.author = "Test Author";
    meta.tags.Add("test");
    meta.tags.Add("validation");
    
    // Create profile steps
    Vector<IntentProfileStep> steps;
    
    IntentProfileStep step1;
    step1.request.intents = {IntentKind::Warmer};
    step1.request.strength = 0.5;
    step1.request.duration_sec = 3.0;
    step1.request.sample_rate = 48000;
    step1.request.block_size = 512;
    step1.request.allow_regression = false;
    step1.request.qa_profile_name = "default";
    step1.note = "Make warmer";
    steps.Add(step1);
    
    // Test profile creation
    auto create_result = IntentProfileManager::CreateProfile(meta, steps, temp_dir.string());
    assert(create_result.ok());
    std::cout << "  CreateProfile: OK" << std::endl;
    
    // Test profile loading
    auto load_result = IntentProfileManager::LoadProfile(temp_dir.string(), "ip-test-profile");
    assert(load_result.ok());
    assert(load_result.value().meta.profile_id == "ip-test-profile");
    assert(load_result.value().meta.name == "Test Profile");
    assert(load_result.value().steps.GetCount() == 1);
    std::cout << "  LoadProfile: OK" << std::endl;
    
    // Test listing profiles
    auto list_result = IntentProfileManager::ListProfiles(temp_dir.string());
    assert(list_result.ok());
    assert(list_result.value().GetCount() == 1);
    assert(list_result.value()[0].profile_id == "ip-test-profile");
    std::cout << "  ListProfiles: OK" << std::endl;
    
    // Test listing with tag filter
    auto list_filtered_result = IntentProfileManager::ListProfiles(temp_dir.string(), "test");
    assert(list_filtered_result.ok());
    assert(list_filtered_result.value().GetCount() == 1);
    std::cout << "  ListProfiles with tag filter: OK" << std::endl;
    
    // Test listing with non-existent tag
    auto list_nonexistent_result = IntentProfileManager::ListProfiles(temp_dir.string(), "nonexistent");
    assert(list_nonexistent_result.ok());
    assert(list_nonexistent_result.value().GetCount() == 0);
    std::cout << "  ListProfiles with non-existent tag: OK" << std::endl;
    
    // Test scale intent request
    IntentRequest original_request;
    original_request.intents = {IntentKind::Warmer, IntentKind::Softer};
    original_request.strength = 0.6;
    
    auto scaled_result = ScaleIntentRequest(original_request, 0.5);
    assert(scaled_result.ok());
    assert(scaled_result.value().strength == 0.3);  // 0.6 * 0.5
    std::cout << "  ScaleIntentRequest: OK" << std::endl;
    
    // Verify bounds checking for scaling
    auto scaled_result_low = ScaleIntentRequest(original_request, 0.0);
    assert(scaled_result_low.ok());
    assert(scaled_result_low.value().strength == 0.0);
    
    auto scaled_result_high = ScaleIntentRequest(original_request, 2.0);
    assert(scaled_result_high.ok());
    assert(scaled_result_high.value().strength == 1.0);  // Clamped
    std::cout << "  ScaleIntentRequest bounds checking: OK" << std::endl;
    
    // Clean up
    fs::remove_all(temp_dir);
    
    std::cout << "IntentProfileManager tests completed successfully!" << std::endl;
}

// Test CircuitFacade integration
void TestCircuitFacadeIntegration() {
    std::cout << "Testing CircuitFacade integration..." << std::endl;
    
    // Create a test session
    IntentSessionState test_session = CreateTestSession();
    
    // Create profile metadata
    IntentProfileMetadata meta;
    meta.profile_id = "ip-test-from-session";
    meta.name = "Test Profile from Session";
    meta.description = "A test profile created from a session";
    meta.scope = IntentProfileScope::Workspace;
    
    // Create step indices to include - just the first step
    Vector<int> step_indices;
    step_indices.Add(0);
    
    // Test creating profile from session
    CircuitFacade facade;
    auto profile_result = facade.CreateIntentProfileFromSession(test_session, step_indices, meta);
    assert(profile_result.ok());
    assert(profile_result.value().meta.profile_id == "ip-test-from-session");
    assert(profile_result.value().steps.GetCount() == 1);
    std::cout << "  CreateIntentProfileFromSession: OK" << std::endl;
    
    // Test applying profile to session
    IntentSessionState session_copy = test_session;
    auto apply_result = facade.ApplyIntentProfileToSession(session_copy, "ip-test-from-session", 1.0, false);
    assert(apply_result.ok());
    std::cout << "  ApplyIntentProfileToSession: OK" << std::endl;
    
    std::cout << "CircuitFacade integration tests completed successfully!" << std::endl;
}

// Test JSON serialization
void TestJsonSerialization() {
    std::cout << "Testing JSON serialization..." << std::endl;
    
    // Create a test profile
    IntentProfile profile;
    profile.meta.profile_id = "ip-json-test";
    profile.meta.name = "JSON Test Profile";
    profile.meta.description = "Profile for JSON serialization test";
    profile.meta.scope = IntentProfileScope::Global;
    profile.meta.author = "Test Author";
    profile.meta.tags.Add("json");
    profile.meta.tags.Add("test");
    
    IntentProfileStep step;
    step.request.intents = {IntentKind::Warmer, IntentKind::Wider};
    step.request.strength = 0.7;
    step.request.duration_sec = 2.5;
    step.request.sample_rate = 44100;
    step.request.block_size = 256;
    step.request.allow_regression = true;
    step.request.qa_profile_name = "strict";
    step.note = "Test JSON serialization";
    
    profile.steps.Add(step);
    
    // Test serialization
    Upp::ValueMap serialized = JsonIO::IntentProfileToValueMap(profile);
    assert(!serialized.IsEmpty());
    assert(serialized.Get("meta")("profile_id") == "ip-json-test");
    std::cout << "  IntentProfileToValueMap: OK" << std::endl;
    
    // Test deserialization
    Result<IntentProfile> deserialized_result = JsonIO::IntentProfileFromValueMap(serialized);
    assert(deserialized_result.ok());
    IntentProfile deserialized = deserialized_result.value();
    assert(deserialized.meta.profile_id == "ip-json-test");
    assert(deserialized.meta.name == "JSON Test Profile");
    assert(deserialized.meta.tags.GetCount() == 2);
    assert(deserialized.steps.GetCount() == 1);
    std::cout << "  IntentProfileFromValueMap: OK" << std::endl;
    
    // Test metadata serialization/deserialization
    Upp::ValueMap meta_serialized = JsonIO::IntentProfileMetadataToValueMap(profile.meta);
    Result<IntentProfileMetadata> meta_deserialized_result = 
        JsonIO::IntentProfileMetadataFromValueMap(meta_serialized);
    assert(meta_deserialized_result.ok());
    IntentProfileMetadata meta_deserialized = meta_deserialized_result.value();
    assert(meta_deserialized.profile_id == "ip-json-test");
    std::cout << "  IntentProfileMetadata serialization: OK" << std::endl;
    
    // Test step serialization/deserialization
    Upp::ValueMap step_serialized = JsonIO::IntentProfileStepToValueMap(step);
    Result<IntentProfileStep> step_deserialized_result = 
        JsonIO::IntentProfileStepFromValueMap(step_serialized);
    assert(step_deserialized_result.ok());
    std::cout << "  IntentProfileStep serialization: OK" << std::endl;
    
    std::cout << "JSON serialization tests completed successfully!" << std::endl;
}

// Test profile persistence
void TestProfilePersistence() {
    std::cout << "Testing profile persistence..." << std::endl;
    
    // Create a temporary directory for testing
    fs::path temp_dir = fs::temp_directory_path() / "protovm_persistence_test";
    if (fs::exists(temp_dir)) {
        fs::remove_all(temp_dir);
    }
    fs::create_directories(temp_dir);
    
    // Create a test profile
    IntentProfileMetadata meta;
    meta.profile_id = "ip-persistence-test";
    meta.name = "Persistence Test Profile";
    meta.description = "Profile for persistence testing";
    meta.scope = IntentProfileScope::Workspace;
    
    Vector<IntentProfileStep> steps;
    
    IntentProfileStep step;
    step.request.intents = {IntentKind::Softer};
    step.request.strength = 0.3;
    step.note = "Soften the sound";
    steps.Add(step);
    
    // Create and save profile
    auto create_result = IntentProfileManager::CreateProfile(meta, steps, temp_dir.string());
    assert(create_result.ok());
    std::cout << "  Profile creation: OK" << std::endl;
    
    // Verify file exists
    fs::path profile_file = temp_dir / "ip-persistence-test" / "profile.json";
    assert(fs::exists(profile_file));
    std::cout << "  File creation: OK" << std::endl;
    
    // Load and verify profile
    auto load_result = IntentProfileManager::LoadProfile(temp_dir.string(), "ip-persistence-test");
    assert(load_result.ok());
    assert(load_result.value().meta.profile_id == "ip-persistence-test");
    assert(load_result.value().meta.name == "Persistence Test Profile");
    assert(load_result.value().steps.GetCount() == 1);
    assert(load_result.value().steps[0].request.strength == 0.3);
    std::cout << "  Profile loading: OK" << std::endl;
    
    // Test that we can't create a duplicate profile
    auto duplicate_result = IntentProfileManager::CreateProfile(meta, steps, temp_dir.string());
    assert(!duplicate_result.ok());  // Should fail because ID already exists
    std::cout << "  Duplicate prevention: OK" << std::endl;
    
    // Clean up
    fs::remove_all(temp_dir);
    
    std::cout << "Profile persistence tests completed successfully!" << std::endl;
}

// Run all tests
void RunIntentProfileTests() {
    std::cout << "Running Intent Profile Tests..." << std::endl;
    std::cout << "=================================" << std::endl;
    
    TestIntentProfileManager();
    std::cout << std::endl;
    
    TestJsonSerialization();
    std::cout << std::endl;
    
    TestProfilePersistence();
    std::cout << std::endl;
    
    TestCircuitFacadeIntegration();
    std::cout << std::endl;
    
    std::cout << "All Intent Profile Tests completed successfully!" << std::endl;
}

} // namespace ProtoVMCLI

int main() {
    ProtoVMCLI::RunIntentProfileTests();
    return 0;
}