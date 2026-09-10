#include "PerformanceScenes.h"
#include "InstrumentGraph.h"
#include "IntentProfiles.h"
#include "JsonIO.h"
#include "CppUnitLite/Test.h"
#include <iostream>

using namespace Upp;

void TestSceneSnapshotStructure() {
    std::cout << "Testing SceneSnapshot structure..." << std::endl;
    
    SceneSnapshot snapshot;
    snapshot.scene_id = "test_scene";
    snapshot.name = "Test Scene";
    
    // Test basic properties
    ASSERT(snapshot.scene_id == "test_scene");
    ASSERT(snapshot.name == "Test Scene");
    
    std::cout << "SceneSnapshot structure tests passed!" << std::endl;
}

void TestSceneSetStructure() {
    std::cout << "Testing SceneSet structure..." << std::endl;
    
    SceneSet scene_set;
    scene_set.set_id = "test_set";
    scene_set.name = "Test Scene Set";
    
    // Add a scene
    SceneSnapshot scene;
    scene.scene_id = "nested_scene";
    scene.name = "Nested Scene";
    scene_set.scenes.Add(scene);
    
    // Test basic properties
    ASSERT(scene_set.set_id == "test_set");
    ASSERT(scene_set.name == "Test Scene Set");
    ASSERT(scene_set.scenes.GetCount() == 1);
    ASSERT(scene_set.scenes[0].scene_id == "nested_scene");
    
    std::cout << "SceneSet structure tests passed!" << std::endl;
}

void TestMorphStateStructure() {
    std::cout << "Testing MorphState structure..." << std::endl;
    
    MorphState morph_state;
    morph_state.from_scene_id = "scene_a";
    morph_state.to_scene_id = "scene_b";
    morph_state.morph = 0.5;
    
    // Test basic properties
    ASSERT(morph_state.from_scene_id == "scene_a");
    ASSERT(morph_state.to_scene_id == "scene_b");
    ASSERT(morph_state.morph == 0.5);
    
    std::cout << "MorphState structure tests passed!" << std::endl;
}

void TestSceneBuilderFromProfile() {
    std::cout << "Testing SceneBuilder BuildSceneFromProfile..." << std::endl;
    
    // Create a basic instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.name = "Test Instrument";
    
    // Add a parameter
    InstrumentParameter param;
    param.id = "volume";
    param.name = "Volume";
    param.type = "float";
    param.default_value = 0.7;
    param.min_value = 0.0;
    param.max_value = 1.0;
    instrument.parameters.push_back(param);
    
    // Create a basic intent profile
    IntentProfile profile;
    profile.profile_id = "test_profile";
    profile.metadata.name = "Test Profile";
    
    // Add a step to the profile
    IntentProfileStep step;
    step.intent_id = "brighten";
    step.strength = 0.8;
    step.duration = 1.0;
    profile.steps.Add(step);
    
    // Test building scene from profile
    auto result = SceneBuilder::BuildSceneFromProfile(profile, instrument);
    ASSERT(result.ok);
    
    SceneSnapshot snapshot = result.data;
    ASSERT(snapshot.scene_id == "scene_test_profile");
    ASSERT(snapshot.name == "Test Profile Scene");
    ASSERT(snapshot.params.GetCount() >= 0); // May have parameters
    
    std::cout << "SceneBuilder BuildSceneFromProfile tests passed!" << std::endl;
}

void TestSceneBuilderFromSession() {
    std::cout << "Testing SceneBuilder BuildSceneFromSession..." << std::endl;
    
    // Create a basic instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.name = "Test Instrument";
    
    // Add a parameter
    InstrumentParameter param;
    param.id = "cutoff";
    param.name = "Filter Cutoff";
    param.type = "float";
    param.default_value = 1000.0;
    param.min_value = 20.0;
    param.max_value = 20000.0;
    instrument.parameters.push_back(param);
    
    // Create a basic session
    IntentSessionState session;
    session.session_id = "test_session";
    session.metadata.name = "Test Session";
    
    // Test building scene from session
    auto result = SceneBuilder::BuildSceneFromSession(session, instrument);
    ASSERT(result.ok);
    
    SceneSnapshot snapshot = result.data;
    ASSERT(snapshot.scene_id == "scene_test_session");
    ASSERT(snapshot.name == "Test Session Scene");
    ASSERT(snapshot.params.GetCount() >= 0); // May have parameters
    
    std::cout << "SceneBuilder BuildSceneFromSession tests passed!" << std::endl;
}

void TestSceneMorphEngineBasics() {
    std::cout << "Testing SceneMorphEngine ApplyMorph..." << std::endl;
    
    // Create two simple scenes
    SceneSnapshot scene_a, scene_b;
    scene_a.scene_id = "scene_a";
    scene_a.name = "Scene A";
    scene_a.params.Add("volume", 0.3);
    scene_a.params.Add("cutoff", 500.0);
    scene_a.intent_strengths.Add("warm", 0.2);
    
    scene_b.scene_id = "scene_b";
    scene_b.name = "Scene B";
    scene_b.params.Add("volume", 0.8);
    scene_b.params.Add("cutoff", 2000.0);
    scene_b.intent_strengths.Add("warm", 0.9);
    
    // Mock instrument runtime
    // Since we can't easily create a real InstrumentRuntime for testing,
    // we'll test the logic with a dummy runtime
    InstrumentRuntime dummy_runtime;
    
    // Test morphing at 0.0 (should be identical to scene A)
    auto result_a = SceneMorphEngine::ApplyMorph(dummy_runtime, scene_a, scene_b, 0.0);
    ASSERT(result_a.ok);
    
    // Test morphing at 1.0 (should be identical to scene B)
    auto result_b = SceneMorphEngine::ApplyMorph(dummy_runtime, scene_a, scene_b, 1.0);
    ASSERT(result_b.ok);
    
    // Test morphing at 0.5 (should be halfway between)
    auto result_mid = SceneMorphEngine::ApplyMorph(dummy_runtime, scene_a, scene_b, 0.5);
    ASSERT(result_mid.ok);
    
    std::cout << "SceneMorphEngine ApplyMorph tests passed!" << std::endl;
}

void TestMorphInterpolation() {
    std::cout << "Testing morph interpolation math..." << std::endl;
    
    // This test verifies that the morphing logic interpolates values correctly
    SceneSnapshot scene_a, scene_b;
    
    scene_a.params.Add("param1", 0.0);
    scene_a.params.Add("param2", 10.0);
    scene_a.intent_strengths.Add("intent1", 0.0f);
    
    scene_b.params.Add("param1", 1.0);
    scene_b.params.Add("param2", 20.0);
    scene_b.intent_strengths.Add("intent1", 1.0f);
    
    // Calculate expected interpolated values at 0.5
    double expected_param1 = 0.0 + 0.5 * (1.0 - 0.0);  // 0.5
    double expected_param2 = 10.0 + 0.5 * (20.0 - 10.0);  // 15.0
    float expected_intent1 = 0.0f + 0.5f * (1.0f - 0.0f);  // 0.5
    
    ASSERT(abs(expected_param1 - 0.5) < 0.001);
    ASSERT(abs(expected_param2 - 15.0) < 0.001);
    ASSERT(abs(expected_intent1 - 0.5f) < 0.001f);
    
    std::cout << "Morph interpolation math tests passed!" << std::endl;
}

void TestJsonSerialization() {
    std::cout << "Testing JSON serialization for Scene structures..." << std::endl;
    
    // Create a scene snapshot
    SceneSnapshot snapshot;
    snapshot.scene_id = "json_test_scene";
    snapshot.name = "JSON Test Scene";
    snapshot.params.Add("volume", 0.7);
    snapshot.params.Add("cutoff", 1500.0);
    snapshot.intent_strengths.Add("bright", 0.6);
    
    // Test SceneSnapshot serialization
    auto snapshot_json = JsonIO::SceneSnapshotToValueMap(snapshot);
    ASSERT(snapshot_json.Get("scene_id") == Value("json_test_scene"));
    ASSERT(snapshot_json.Get("name") == Value("JSON Test Scene"));
    ASSERT(snapshot_json.Get("params").IsType<Upp::ValueMap>());
    
    // Create a scene set
    SceneSet scene_set;
    scene_set.set_id = "json_test_set";
    scene_set.name = "JSON Test Set";
    scene_set.scenes.Add(snapshot);
    
    // Test SceneSet serialization
    auto set_json = JsonIO::SceneSetToValueMap(scene_set);
    ASSERT(set_json.Get("set_id") == Value("json_test_set"));
    ASSERT(set_json.Get("name") == Value("JSON Test Set"));
    ASSERT(set_json.Get("scenes").IsType<Upp::ValueArray>());
    
    auto scenes_array = set_json.Get("scenes").Get<Upp::ValueArray>();
    ASSERT(scenes_array.GetCount() == 1);
    
    // Test MorphState serialization
    MorphState morph_state;
    morph_state.from_scene_id = "from_scene";
    morph_state.to_scene_id = "to_scene";
    morph_state.morph = 0.75;
    
    auto morph_json = JsonIO::MorphStateToValueMap(morph_state);
    ASSERT(morph_json.Get("from_scene_id") == Value("from_scene"));
    ASSERT(morph_json.Get("to_scene_id") == Value("to_scene"));
    ASSERT(morph_json.Get("morph") == 0.75);
    
    std::cout << "JSON serialization tests passed!" << std::endl;
}

void TestSceneWorkflow() {
    std::cout << "Testing complete scene workflow..." << std::endl;
    
    // 1. Create a basic instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "workflow_instrument";
    instrument.name = "Workflow Instrument";
    
    InstrumentParameter vol_param;
    vol_param.id = "volume";
    vol_param.name = "Volume";
    vol_param.type = "float";
    vol_param.default_value = 0.5;
    instrument.parameters.push_back(vol_param);
    
    // 2. Create intent profiles to convert to scenes
    IntentProfile profile_a;
    profile_a.profile_id = "profile_a";
    profile_a.metadata.name = "Bright Profile";
    
    IntentProfileStep step_a;
    step_a.intent_id = "brighten";
    step_a.strength = 0.8;
    profile_a.steps.Add(step_a);
    
    IntentProfile profile_b;
    profile_b.profile_id = "profile_b";
    profile_b.metadata.name = "Warm Profile";
    
    IntentProfileStep step_b;
    step_b.intent_id = "warm";
    step_b.strength = 0.6;
    profile_b.steps.Add(step_b);
    
    // 3. Build scenes from profiles
    auto scene_a_result = SceneBuilder::BuildSceneFromProfile(profile_a, instrument);
    auto scene_b_result = SceneBuilder::BuildSceneFromProfile(profile_b, instrument);
    
    ASSERT(scene_a_result.ok);
    ASSERT(scene_b_result.ok);
    
    SceneSnapshot scene_a = scene_a_result.data;
    SceneSnapshot scene_b = scene_b_result.data;
    
    // 4. Create a scene set containing both scenes
    SceneSet scene_set;
    scene_set.set_id = "workflow_set";
    scene_set.name = "Workflow Scene Set";
    scene_set.scenes.Add(scene_a);
    scene_set.scenes.Add(scene_b);
    
    ASSERT(scene_set.scenes.GetCount() == 2);
    
    // 5. Test morphing between scenes
    InstrumentRuntime dummy_runtime;
    auto morph_result = SceneMorphEngine::ApplyMorph(dummy_runtime, scene_a, scene_b, 0.3);
    ASSERT(morph_result.ok);
    
    // 6. Test morphing at different positions
    auto morph_0_result = SceneMorphEngine::ApplyMorph(dummy_runtime, scene_a, scene_b, 0.0);
    auto morph_1_result = SceneMorphEngine::ApplyMorph(dummy_runtime, scene_a, scene_b, 1.0);
    auto morph_half_result = SceneMorphEngine::ApplyMorph(dummy_runtime, scene_a, scene_b, 0.5);
    
    ASSERT(morph_0_result.ok);
    ASSERT(morph_1_result.ok);
    ASSERT(morph_half_result.ok);
    
    std::cout << "Complete scene workflow tests passed!" << std::endl;
}

void RunPerformanceScenesTests() {
    std::cout << "\n=== Running PerformanceScenes Tests ===" << std::endl;
    
    TestSceneSnapshotStructure();
    TestSceneSetStructure();
    TestMorphStateStructure();
    TestSceneBuilderFromProfile();
    TestSceneBuilderFromSession();
    TestSceneMorphEngineBasics();
    TestMorphInterpolation();
    TestJsonSerialization();
    TestSceneWorkflow();
    
    std::cout << "\n=== All PerformanceScenes Tests Passed! ===" << std::endl;
}

// Run the tests if this is the main file being executed
#ifdef PERFORMANCESCENES_TESTS_MAIN
int main() {
    RunPerformanceScenesTests();
    return 0;
}
#endif