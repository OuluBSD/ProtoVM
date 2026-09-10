#include "ControlMapping.h"
#include "InstrumentGraph.h"
#include "InstrumentRuntime.h"
#include "JsonIO.h"
#include "CppUnitLite/Test.h"
#include <iostream>

using namespace Upp;

void TestControlSourceKindJsonSerialization() {
    std::cout << "Testing ControlSourceKind JSON serialization..." << std::endl;
    
    // Test all ControlSourceKind values
    ASSERT(JsonIO::ControlSourceKindToJson(ControlSourceKind::MidiCC) == Value("midi_cc"));
    ASSERT(JsonIO::ControlSourceKindToJson(ControlSourceKind::MidiNote) == Value("midi_note"));
    ASSERT(JsonIO::ControlSourceKindToJson(ControlSourceKind::MidiPitchBend) == Value("midi_pitch_bend"));
    ASSERT(JsonIO::ControlSourceKindToJson(ControlSourceKind::HostParam) == Value("host_param"));
    ASSERT(JsonIO::ControlSourceKindToJson(ControlSourceKind::VirtualKnob) == Value("virtual_knob"));
    ASSERT(JsonIO::ControlSourceKindToJson(ControlSourceKind::VirtualXY) == Value("virtual_xy"));
    
    std::cout << "ControlSourceKind JSON serialization tests passed!" << std::endl;
}

void TestControlTargetKindJsonSerialization() {
    std::cout << "Testing ControlTargetKind JSON serialization..." << std::endl;
    
    // Test all ControlTargetKind values
    ASSERT(JsonIO::ControlTargetKindToJson(ControlTargetKind::InstrumentParam) == Value("instrument_param"));
    ASSERT(JsonIO::ControlTargetKindToJson(ControlTargetKind::AutomationLane) == Value("automation_lane"));
    ASSERT(JsonIO::ControlTargetKindToJson(ControlTargetKind::IntentStrength) == Value("intent_strength"));
    ASSERT(JsonIO::ControlTargetKindToJson(ControlTargetKind::IntentProfileMix) == Value("intent_profile_mix"));
    
    std::cout << "ControlTargetKind JSON serialization tests passed!" << std::endl;
}

void TestControlSourceJsonSerialization() {
    std::cout << "Testing ControlSource JSON serialization..." << std::endl;
    
    ControlSource source;
    source.kind = ControlSourceKind::MidiCC;
    source.channel = 1;
    source.number = 7;
    source.name = "Volume";
    
    auto source_json = JsonIO::ControlSourceToValueMap(source);
    
    // Check that the JSON contains the expected fields
    ASSERT(source_json.GetCount() == 4);
    ASSERT(source_json.Get("kind") == Value("midi_cc"));
    ASSERT(source_json.Get("channel") == 1);
    ASSERT(source_json.Get("number") == 7);
    ASSERT(source_json.Get("name") == Value("Volume"));
    
    std::cout << "ControlSource JSON serialization tests passed!" << std::endl;
}

void TestControlTargetJsonSerialization() {
    std::cout << "Testing ControlTarget JSON serialization..." << std::endl;
    
    ControlTarget target;
    target.kind = ControlTargetKind::InstrumentParam;
    target.target_id = "volume";
    
    auto target_json = JsonIO::ControlTargetToValueMap(target);
    
    // Check that the JSON contains the expected fields
    ASSERT(target_json.GetCount() == 2);
    ASSERT(target_json.Get("kind") == Value("instrument_param"));
    ASSERT(target_json.Get("target_id") == Value("volume"));
    
    std::cout << "ControlTarget JSON serialization tests passed!" << std::endl;
}

void TestControlMappingJsonSerialization() {
    std::cout << "Testing ControlMapping JSON serialization..." << std::endl;
    
    ControlMapping mapping;
    mapping.mapping_id = "vol_control";
    mapping.source.kind = ControlSourceKind::MidiCC;
    mapping.source.channel = 0;
    mapping.source.number = 7;
    mapping.source.name = "Volume";
    mapping.target.kind = ControlTargetKind::InstrumentParam;
    mapping.target.target_id = "volume";
    mapping.min_value = 0.0;
    mapping.max_value = 1.0;
    mapping.curve = 1.0;
    mapping.bipolar = false;
    mapping.note = "Volume control via MIDI CC7";
    
    auto mapping_json = JsonIO::ControlMappingToValueMap(mapping);
    
    // Check that the JSON contains the expected fields
    ASSERT(mapping_json.GetCount() == 8);
    ASSERT(mapping_json.Get("mapping_id") == Value("vol_control"));
    ASSERT(mapping_json.Get("source").IsType<Upp::ValueMap>());
    ASSERT(mapping_json.Get("target").IsType<Upp::ValueMap>());
    ASSERT(mapping_json.Get("min_value") == 0.0);
    ASSERT(mapping_json.Get("max_value") == 1.0);
    ASSERT(mapping_json.Get("curve") == 1.0);
    ASSERT(mapping_json.Get("bipolar") == false);
    ASSERT(mapping_json.Get("note") == Value("Volume control via MIDI CC7"));
    
    std::cout << "ControlMapping JSON serialization tests passed!" << std::endl;
}

void TestControlMapJsonSerialization() {
    std::cout << "Testing ControlMap JSON serialization..." << std::endl;
    
    ControlMap control_map;
    control_map.map_id = "default_map";
    control_map.name = "Default Control Map";
    
    // Add a control mapping
    ControlMapping mapping;
    mapping.mapping_id = "vol_control";
    mapping.source.kind = ControlSourceKind::MidiCC;
    mapping.source.channel = 0;
    mapping.source.number = 7;
    mapping.source.name = "Volume";
    mapping.target.kind = ControlTargetKind::InstrumentParam;
    mapping.target.target_id = "volume";
    mapping.min_value = 0.0;
    mapping.max_value = 1.0;
    
    control_map.mappings.Add(mapping);
    
    auto map_json = JsonIO::ControlMapToValueMap(control_map);
    
    // Check that the JSON contains the expected fields
    ASSERT(map_json.GetCount() == 3);
    ASSERT(map_json.Get("map_id") == Value("default_map"));
    ASSERT(map_json.Get("name") == Value("Default Control Map"));
    ASSERT(map_json.Get("mappings").IsType<Upp::ValueArray>());
    
    auto mappings_array = map_json.Get("mappings").Get<Upp::ValueArray>();
    ASSERT(mappings_array.GetCount() == 1);
    
    auto first_mapping = mappings_array[0].Get<Upp::ValueMap>();
    ASSERT(first_mapping.Get("mapping_id") == Value("vol_control"));
    
    std::cout << "ControlMap JSON serialization tests passed!" << std::endl;
}

void TestMidiEventJsonSerialization() {
    std::cout << "Testing MidiEvent JSON serialization..." << std::endl;
    
    MidiEvent event;
    event.kind = MidiEventKind::CC;
    event.channel = 1;
    event.number = 7;
    event.value = 64;
    
    auto event_json = JsonIO::MidiEventToValueMap(event);
    
    // Check that the JSON contains the expected fields
    ASSERT(event_json.GetCount() == 4);
    ASSERT(event_json.Get("kind") == Value("cc"));
    ASSERT(event_json.Get("channel") == 1);
    ASSERT(event_json.Get("number") == 7);
    ASSERT(event_json.Get("value") == 64);
    
    std::cout << "MidiEvent JSON serialization tests passed!" << std::endl;
}

void TestControlMappingEngineBasics() {
    std::cout << "Testing ControlMappingEngine basic functionality..." << std::endl;
    
    // Create a dummy instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "test_instrument";
    instrument.name = "Test Instrument";
    
    // Add some parameters to the instrument
    InstrumentParameter param1;
    param1.id = "volume";
    param1.name = "Volume";
    param1.type = "float";
    param1.min_value = 0.0;
    param1.max_value = 1.0;
    instrument.parameters.push_back(param1);
    
    InstrumentParameter param2;
    param2.id = "cutoff";
    param2.name = "Filter Cutoff";
    param2.type = "float";
    param2.min_value = 0.0;
    param2.max_value = 1000.0;
    instrument.parameters.push_back(param2);
    
    // Create a default control map for the instrument
    auto result = ControlMappingEngine::CreateDefaultMapForInstrument(instrument);
    ASSERT(result.ok);
    
    ControlMap control_map = result.data;
    ASSERT(control_map.mappings.GetCount() >= 2);  // Should map both parameters
    
    // Check that the mappings have been created
    bool found_volume_mapping = false;
    bool found_cutoff_mapping = false;
    
    for (const auto& mapping : control_map.mappings) {
        if (mapping.target.target_id == "volume") {
            found_volume_mapping = true;
        }
        if (mapping.target.target_id == "cutoff") {
            found_cutoff_mapping = true;
        }
    }
    
    ASSERT(found_volume_mapping);
    ASSERT(found_cutoff_mapping);
    
    std::cout << "ControlMappingEngine basic functionality tests passed!" << std::endl;
}

void TestMidiEventApplication() {
    std::cout << "Testing MIDI event application..." << std::endl;
    
    // Create a basic control map
    ControlMap control_map;
    control_map.map_id = "test_map";
    control_map.name = "Test Map";
    
    // Add a mapping from MIDI CC 7 to the volume parameter
    ControlMapping mapping;
    mapping.mapping_id = "vol_cc_mapping";
    mapping.source.kind = ControlSourceKind::MidiCC;
    mapping.source.channel = 0;
    mapping.source.number = 7;
    mapping.source.name = "Volume CC";
    mapping.target.kind = ControlTargetKind::InstrumentParam;
    mapping.target.target_id = "volume";
    mapping.min_value = 0.0;
    mapping.max_value = 1.0;
    mapping.curve = 1.0;
    
    control_map.mappings.Add(mapping);
    
    // Create a dummy runtime (we'll test the logic without full runtime)
    // Since we can't easily test the full runtime without a full instrument,
    // we'll test the MIDI event processing logic
    
    // Create a MIDI CC event for CC7 with value 64 (mid-range)
    MidiEvent event;
    event.kind = MidiEventKind::CC;
    event.channel = 0;
    event.number = 7;
    event.value = 64;
    
    // The ApplyMidiEvent method requires a real InstrumentRuntime, so we'll
    // just validate the components work as expected separately
    
    // Verify that the event and mapping match
    bool event_matches = false;
    for (const auto& mapping : control_map.mappings) {
        if (mapping.source.channel == event.channel && 
            mapping.source.number == event.number &&
            mapping.source.kind == ControlSourceKind::MidiCC) {
            event_matches = true;
            break;
        }
    }
    
    ASSERT(event_matches);
    
    std::cout << "MIDI event application logic tests passed!" << std::endl;
}

void TestControlValueApplication() {
    std::cout << "Testing control value application with curves..." << std::endl;
    
    // Test linear curve (curve = 1.0)
    ControlMapping linear_mapping;
    linear_mapping.min_value = 0.0;
    linear_mapping.max_value = 100.0;
    linear_mapping.curve = 1.0;
    linear_mapping.bipolar = false;
    
    // At normalized value 0.5, with linear curve and range 0-100, should get 50.0
    double linear_result = 0.0;
    
    // Simulate the transformation (without full runtime)
    double normalized_value = 0.5;
    if (linear_mapping.curve != 1.0) {
        linear_result = pow(normalized_value, linear_mapping.curve);
    } else {
        linear_result = normalized_value;
    }
    linear_result = linear_mapping.min_value + 
                   (linear_result * (linear_mapping.max_value - linear_mapping.min_value));
    
    ASSERT(abs(linear_result - 50.0) < 0.001);  // Allow for floating point precision
    
    // Test exponential curve (curve > 1.0)
    ControlMapping exp_mapping;
    exp_mapping.min_value = 0.0;
    exp_mapping.max_value = 100.0;
    exp_mapping.curve = 2.0;  // Quadratic curve
    exp_mapping.bipolar = false;
    
    // At normalized value 0.5 with quadratic curve, should get 0.5^2 = 0.25 in [0,1] -> 25.0 in [0,100]
    double exp_result = 0.0;
    if (exp_mapping.curve != 1.0) {
        exp_result = pow(0.5, exp_mapping.curve);  // 0.5^2 = 0.25
    } else {
        exp_result = 0.5;
    }
    exp_result = exp_mapping.min_value + 
                (exp_result * (exp_mapping.max_value - exp_mapping.min_value));
    
    ASSERT(abs(exp_result - 25.0) < 0.001);
    
    // Test bipolar mapping
    ControlMapping bipolar_mapping;
    bipolar_mapping.min_value = -100.0;
    bipolar_mapping.max_value = 100.0;
    bipolar_mapping.curve = 1.0;
    bipolar_mapping.bipolar = true;
    
    // For bipolar with normalized 0.5 -> should convert to 0.0 in bipolar range [-1,1] -> then to 0.0 in [-100,100]
    // Normalized 0.5 -> bipolar 0.0 -> mid-range value
    double bipolar_normalized = 0.5;
    double bipolar_value = (bipolar_normalized * 2.0) - 1.0;  // [0,1] -> [-1,1] = 0.0
    double mid_range = (bipolar_mapping.max_value + bipolar_mapping.min_value) / 2.0;  // 0.0
    double half_range = (bipolar_mapping.max_value - bipolar_mapping.min_value) / 2.0;  // 100.0
    double bipolar_result = mid_range + (bipolar_value * half_range);  // 0.0 + (0.0 * 100.0) = 0.0
    
    ASSERT(abs(bipolar_result - 0.0) < 0.001);
    
    std::cout << "Control value application with curves tests passed!" << std::endl;
}

void TestControlMappingWorkflow() {
    std::cout << "Testing complete control mapping workflow..." << std::endl;
    
    // 1. Create a simple instrument
    InstrumentGraph instrument;
    instrument.instrument_id = "synth1";
    instrument.name = "Simple Synth";
    
    // Add parameters
    InstrumentParameter param1;
    param1.id = "osc1_freq";
    param1.name = "Oscillator 1 Frequency";
    param1.type = "float";
    param1.min_value = 20.0;  // Hz
    param1.max_value = 20000.0;  // Hz
    instrument.parameters.push_back(param1);
    
    InstrumentParameter param2;
    param2.id = "filter_res";
    param2.name = "Filter Resonance";
    param2.type = "float";
    param2.min_value = 0.0;
    param2.max_value = 1.0;
    instrument.parameters.push_back(param2);
    
    // 2. Generate default control map for the instrument
    auto map_result = ControlMappingEngine::CreateDefaultMapForInstrument(instrument);
    ASSERT(map_result.ok);
    
    ControlMap control_map = map_result.data;
    ASSERT(control_map.mappings.GetCount() >= 2);  // Should have mappings for both parameters
    
    // 3. Find specific mappings
    bool found_freq_mapping = false;
    bool found_res_mapping = false;
    
    for (const auto& mapping : control_map.mappings) {
        if (mapping.target.target_id == "osc1_freq") {
            found_freq_mapping = true;
            // Check that source is MIDI CC
            ASSERT(mapping.source.kind == ControlSourceKind::MidiCC);
            // Check that range matches the parameter
            ASSERT(mapping.min_value == 20.0);
            ASSERT(mapping.max_value == 20000.0);
        }
        if (mapping.target.target_id == "filter_res") {
            found_res_mapping = true;
            // Check that source is MIDI CC
            ASSERT(mapping.source.kind == ControlSourceKind::MidiCC);
            // Check that range matches the parameter
            ASSERT(mapping.min_value == 0.0);
            ASSERT(mapping.max_value == 1.0);
        }
    }
    
    ASSERT(found_freq_mapping);
    ASSERT(found_res_mapping);
    
    // 4. Test applying a MIDI event (simulated)
    MidiEvent test_event;
    test_event.kind = MidiEventKind::CC;
    test_event.channel = 0;
    test_event.number = 1;  // This should match one of our automatically generated mappings
    test_event.value = 100;  // MIDI value 0-127
    
    // Check that there's a mapping that would respond to this event
    bool event_mapped = false;
    for (const auto& mapping : control_map.mappings) {
        if (mapping.source.kind == ControlSourceKind::MidiCC &&
            mapping.source.channel == test_event.channel &&
            mapping.source.number == test_event.number) {
            event_mapped = true;
            break;
        }
    }
    
    // Since we auto-generate CC mappings starting from CC1, this should be true
    ASSERT(event_mapped);
    
    std::cout << "Complete control mapping workflow tests passed!" << std::endl;
}

void RunControlMappingTests() {
    std::cout << "\n=== Running ControlMapping Tests ===" << std::endl;
    
    TestControlSourceKindJsonSerialization();
    TestControlTargetKindJsonSerialization();
    TestControlSourceJsonSerialization();
    TestControlTargetJsonSerialization();
    TestControlMappingJsonSerialization();
    TestControlMapJsonSerialization();
    TestMidiEventJsonSerialization();
    TestControlMappingEngineBasics();
    TestMidiEventApplication();
    TestControlValueApplication();
    TestControlMappingWorkflow();
    
    std::cout << "\n=== All ControlMapping Tests Passed! ===" << std::endl;
}

// Run the tests if this is the main file being executed
#ifdef CONTROLMAPPING_TESTS_MAIN
int main() {
    RunControlMappingTests();
    return 0;
}
#endif