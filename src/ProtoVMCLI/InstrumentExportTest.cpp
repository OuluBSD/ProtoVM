#include "InstrumentExport.h"
#include "InstrumentBuilder.h"
#include "InstrumentGraph.h"
#include <iostream>
#include <string>
#include <cassert>
#include <vector>

using namespace ProtoVM;

void TestInstrumentExportBasic() {
    std::cout << "Testing InstrumentExport::EmitStandaloneCppForInstrument (basic)..." << std::endl;

    // Create a simple instrument for testing
    InstrumentGraph instrument;
    instrument.instrument_id = "TEST_INSTRUMENT";
    instrument.sample_rate_hz = 48000.0;
    instrument.voice_count = 1;
    instrument.note.base_freq_hz = 440.0;  // A4
    instrument.note.duration_sec = 3.0;    // 3 seconds
    instrument.note.velocity = 1.0;

    // Create voice template
    instrument.voice_template.pan_lfo_hz = 0.25;  // 0.25 Hz pan LFO

    // Create a single voice
    VoiceConfig voice;
    voice.id = "voice0";
    voice.detune_cents = 0.0;  // No detune for single voice
    voice.use_analog_source = false;
    instrument.voices.push_back(voice);

    // Create export options
    InstrumentExportOptions options;
    options.program_name = "test_hybrid_instrument";
    options.namespace_name = "";
    options.include_wav_writer = true;
    options.output_wav_filename = "test_output.wav";
    options.emit_comment_banner = true;

    // Test the export function
    auto result = InstrumentExport::EmitStandaloneCppForInstrument(instrument, options);

    // Check that the operation was successful
    assert(result.ok);
    
    // Check that we got some C++ code back
    const std::string& cpp_code = result.data;
    assert(!cpp_code.empty());

    // Check that the generated code contains expected elements
    assert(cpp_code.find("#include <cmath>") != std::string::npos);
    assert(cpp_code.find("#include <vector>") != std::string::npos);
    assert(cpp_code.find("const int SAMPLE_RATE = 48000") != std::string::npos);
    assert(cpp_code.find("const double DURATION_SEC = 3.00") != std::string::npos);
    assert(cpp_code.find("const int VOICE_COUNT = 1") != std::string::npos);
    assert(cpp_code.find("const double BASE_FREQ = 440.00") != std::string::npos);
    assert(cpp_code.find("const double PAN_LFO_HZ = 0.25") != std::string::npos);
    assert(cpp_code.find("int main()") != std::string::npos);
    assert(cpp_code.find("Render(") != std::string::npos);
    assert(cpp_code.find("WriteWav16") != std::string::npos);  // Should be present since include_wav_writer = true

    std::cout << "InstrumentExport basic test passed!" << std::endl;
}

void TestInstrumentExportNoWavWriter() {
    std::cout << "Testing InstrumentExport without WAV writer..." << std::endl;

    // Create a simple instrument for testing
    InstrumentGraph instrument;
    instrument.instrument_id = "TEST_INSTRUMENT_NOWAV";
    instrument.sample_rate_hz = 44100.0;
    instrument.voice_count = 2;  // Test multi-voice
    instrument.note.base_freq_hz = 220.0;  // A3
    instrument.note.duration_sec = 1.0;    // 1 second
    instrument.note.velocity = 0.8;

    // Create voice template
    instrument.voice_template.pan_lfo_hz = 1.0;  // 1 Hz pan LFO

    // Create voices with different detunes
    VoiceConfig voice1;
    voice1.id = "voice0";
    voice1.detune_cents = -5.0;  // -5 cents
    voice1.use_analog_source = true;
    instrument.voices.push_back(voice1);

    VoiceConfig voice2;
    voice2.id = "voice1";
    voice2.detune_cents = 5.0;  // +5 cents
    voice2.use_analog_source = true;
    instrument.voices.push_back(voice2);

    // Create export options without WAV writer
    InstrumentExportOptions options;
    options.program_name = "test_hybrid_no_wav";
    options.namespace_name = "TestNamespace";
    options.include_wav_writer = false;  // No WAV writer
    options.output_wav_filename = "should_not_appear.wav";
    options.emit_comment_banner = true;

    // Test the export function
    auto result = InstrumentExport::EmitStandaloneCppForInstrument(instrument, options);

    // Check that the operation was successful
    assert(result.ok);
    
    // Check that we got some C++ code back
    const std::string& cpp_code = result.data;
    assert(!cpp_code.empty());

    // Check that the generated code contains expected elements
    assert(cpp_code.find("namespace TestNamespace") != std::string::npos);
    assert(cpp_code.find("#include <cmath>") != std::string::npos);
    assert(cpp_code.find("const int SAMPLE_RATE = 44100") != std::string::npos);
    assert(cpp_code.find("const double DURATION_SEC = 1.00") != std::string::npos);
    assert(cpp_code.find("const int VOICE_COUNT = 2") != std::string::npos);
    assert(cpp_code.find("const double BASE_FREQ = 220.00") != std::string::npos);
    assert(cpp_code.find("const double PAN_LFO_HZ = 1.00") != std::string::npos);
    assert(cpp_code.find("int main()") != std::string::npos);
    assert(cpp_code.find("Render(") != std::string::npos);
    assert(cpp_code.find("WriteWav16") == std::string::npos);  // Should NOT be present
    assert(cpp_code.find("std::cout") != std::string::npos);  // Should print samples instead

    std::cout << "InstrumentExport without WAV writer test passed!" << std::endl;
}

void TestInstrumentExportWithNamespace() {
    std::cout << "Testing InstrumentExport with namespace..." << std::endl;

    // Use the InstrumentBuilder to create a proper instrument
    InstrumentVoiceTemplate voice_template;
    voice_template.id = Upp::String("test_voice_ns");
    voice_template.analog_block_id = Upp::String("");
    voice_template.digital_block_id = Upp::String("DIGITAL_TEST_BLOCK");
    voice_template.pan_lfo_hz = 0.5;

    NoteDesc note;
    note.base_freq_hz = 880.0;  // A5
    note.velocity = 0.7;
    note.duration_sec = 2.0;

    auto build_result = ProtoVMCLI::InstrumentBuilder::BuildHybridInstrument(
        Upp::String("TEST_INSTRUMENT_NS"),
        voice_template,
        96000.0,  // High sample rate
        4,        // 4 voices
        note,
        15.0     // 15 cents detune spread
    );

    assert(build_result.ok);

    // Create export options with namespace
    InstrumentExportOptions options;
    options.program_name = "test_with_namespace";
    options.namespace_name = "AudioSynth";  // Test namespace
    options.include_wav_writer = true;
    options.output_wav_filename = "output.wav";
    options.emit_comment_banner = true;

    // Test the export function
    auto result = InstrumentExport::EmitStandaloneCppForInstrument(build_result.data, options);

    // Check that the operation was successful
    assert(result.ok);
    
    // Check that we got some C++ code back
    const std::string& cpp_code = result.data;
    assert(!cpp_code.empty());

    // Check that the generated code contains namespace
    assert(cpp_code.find("namespace AudioSynth") != std::string::npos);
    assert(cpp_code.find("} // namespace AudioSynth") != std::string::npos);
    
    // Check other expected elements
    assert(cpp_code.find("#include <cmath>") != std::string::npos);
    assert(cpp_code.find("const int SAMPLE_RATE = 96000") != std::string::npos);
    assert(cpp_code.find("const int VOICE_COUNT = 4") != std::string::npos);
    assert(cpp_code.find("const double BASE_FREQ = 880.00") != std::string::npos);
    assert(cpp_code.find("const double PAN_LFO_HZ = 0.50") != std::string::npos);
    assert(cpp_code.find("WriteWav16") != std::string::npos);

    std::cout << "InstrumentExport with namespace test passed!" << std::endl;
}

int main() {
    std::cout << "Starting InstrumentExport tests..." << std::endl;

    TestInstrumentExportBasic();
    TestInstrumentExportNoWavWriter();
    TestInstrumentExportWithNamespace();

    std::cout << "All InstrumentExport tests completed successfully!" << std::endl;
    return 0;
}