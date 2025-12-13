# ProtoVM Instrument Export C++ Design Document

## 1. Overview

The Instrument Export C++ feature enables users to export a hybrid analog/digital instrument as a standalone C++ program. This program contains all necessary code to render audio from the instrument without requiring the ProtoVM runtime, allowing users to compile and run instrument configurations independently.

## 2. Purpose

The instrument export feature addresses the need for:
- Standalone C++ programs from instrument configurations
- Portable instrument implementations that don't require ProtoVM runtime
- Integration with external build systems and audio frameworks
- Educational and demonstration purposes
- Offline rendering without ProtoVM dependencies

## 3. Architecture

### 3.1 Components

The feature consists of:

1. **InstrumentExport** - Core export engine that generates standalone C++ code from `InstrumentGraph`
2. **InstrumentExportOptions** - Configuration for export behavior (namespace, WAV writer, etc.)
3. **CircuitFacade Integration** - Integration with existing ProtoVM infrastructure
4. **CLI Command** - `instrument-export-cpp` command for command-line usage
5. **CoDesigner Endpoint** - `designer-instrument-export-cpp` endpoint for AI clients

### 3.2 Data Flow

```
InstrumentGraph -> InstrumentExport -> Standalone C++ Source Code
```

## 4. Data Structures

### 4.1 InstrumentExportOptions

Configuration struct for export behavior:

```cpp
struct InstrumentExportOptions {
    std::string program_name;          // e.g. "hybrid_instrument_demo"
    std::string namespace_name;        // optional, default empty
    bool include_wav_writer = true;
    std::string output_wav_filename;   // e.g. "output.wav"
    bool emit_comment_banner = true;
};
```

### 4.2 Export Configuration

- `program_name`: Name for the generated C++ program
- `namespace_name`: Optional namespace for generated code
- `include_wav_writer`: Whether to include WAV file writer functionality
- `output_wav_filename`: Name of output WAV file (if writer is included)
- `emit_comment_banner`: Whether to include header comments in generated code

## 5. Core Functionality

### 5.1 InstrumentExport Engine

The `InstrumentExport::EmitStandaloneCppForInstrument` method takes an `InstrumentGraph` and `InstrumentExportOptions`, generating a complete C++ program that:

1. Includes necessary headers (`<cmath>`, `<vector>`, etc.)
2. Defines constants from the instrument configuration
3. Creates voice state structures with frequency detuning
4. Implements oscillator functions (sine wave approximation)
5. Implements pan LFO calculation
6. Implements render function that fills stereo buffers
7. Optionally includes a WAV writer function
8. Provides a complete `main()` function that:
   - Sets up instrument parameters
   - Initializes output buffers
   - Renders audio
   - Writes WAV file or prints samples

### 5.2 Generated Program Structure

The generated C++ program follows this structure:

```cpp
// Header comments (optional)
#include <cmath>
#include <vector>
// ... other includes

// Namespace (optional)
namespace my_namespace {

// Constants
const int SAMPLE_RATE = 48000;
const double DURATION_SEC = 3.0;
// ... other constants

// Voice state structure
struct VoiceState { /* ... */ };

// Audio generation functions
float GenerateVoiceSample(VoiceState& voice) { /* ... */ }
float CalculatePan(double time_sec) { /* ... */ }
void Render(std::vector<float>& left, std::vector<float>& right) { /* ... */ }

// WAV writer function (optional)
void WriteWav16(/* ... */) { /* ... */ }

} // namespace

int main() {
    // Initialize and render audio
    return 0;
}
```

## 6. Integration Points

### 6.1 CircuitFacade Integration

The `ExportInstrumentAsStandaloneCppInBranch` method in `CircuitFacade` provides the main integration point:

```cpp
Result<std::string> ExportInstrumentAsStandaloneCppInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const InstrumentGraph& instrument,
    const InstrumentExportOptions& options
);
```

### 6.2 CLI Integration

The `instrument-export-cpp` command allows command-line export:

```
proto-vm-cli instrument-export-cpp \
  --workspace /path/to/workspace \
  --session-id 123 \
  --branch main \
  --instrument-id "HYBRID_OSC_1" \
  --analog-block-id "ANALOG_OSC_BLOCK" \
  --digital-block-id "DIGITAL_OSC_BLOCK" \
  --voice-count 4 \
  --sample-rate 48000 \
  --duration-sec 3.0 \
  --base-freq-hz 440.0 \
  --detune-spread-cents 10.0 \
  --pan-lfo-hz 0.25 \
  --program-name "my_hybrid_instrument" \
  --namespace "MyAudio" \
  --wav-filename "output.wav" \
  --no-wav-writer
```

### 6.3 CoDesigner Integration

The `designer-instrument-export-cpp` endpoint allows AI clients to export instruments:

```json
{
  "command": "designer-instrument-export-cpp",
  "designer_session_id": "cd-1234",
  "instrument_id": "HYBRID_OSC_1",
  "analog_block_id": "ANALOG_OSC_BLOCK",
  "digital_block_id": "DIGITAL_OSC_BLOCK",
  "voice_count": 4,
  "sample_rate_hz": 48000.0,
  "duration_sec": 3.0,
  "base_freq_hz": 440.0,
  "detune_spread_cents": 10.0,
  "pan_lfo_hz": 0.25,
  "program_name": "my_hybrid_instrument",
  "namespace_name": "MyAudio",
  "wav_filename": "output.wav",
  "include_wav_writer": true
}
```

## 7. Generated C++ Code Features

### 7.1 Voice Model

Each voice uses a simple sine wave oscillator with phase accumulation:

```cpp
struct VoiceState {
    double phase;
    double freq;
    double detune_factor;
    VoiceState(double base_freq, double detune_cents);
};

float GenerateVoiceSample(VoiceState& voice) {
    float sample = static_cast<float>(std::sin(voice.phase));
    voice.phase += 2.0 * M_PI * voice.freq / SAMPLE_RATE;
    return sample;
}
```

### 7.2 Pan LFO

Stereo panning using a low-frequency oscillator:

```cpp
float CalculatePan(double time_sec) {
    double pan_phase = 2.0 * M_PI * PAN_LFO_HZ * time_sec;
    double pan = 0.5 * (1.0 + std::sin(pan_phase));  // 0..1
    return static_cast<float>(pan);
}
```

### 7.3 WAV Writer (Optional)

If enabled, includes a minimal WAV writer without external dependencies:

```cpp
void WriteWav16(const std::string& filename, 
                const std::vector<float>& left,
                const std::vector<float>& right, 
                int sample_rate);
```

## 8. Design Considerations

### 8.1 No ProtoVM Dependencies

The generated C++ code has no dependencies on ProtoVM headers or libraries, making it truly standalone.

### 8.2 Simplified Oscillator Model

The export uses a simple sine wave approximation rather than full analog/digital modeling to maintain code simplicity while preserving the essential audio characteristics.

### 8.3 Deterministic Output

The generated code produces consistent results given the same instrument configuration.

## 9. Limitations

- Generated code is approximated (simplified oscillator model) rather than full circuit simulation
- No real-time audio API integration
- No dynamic parameter changes during playback
- Limited to fixed instrument configurations at generation time

## 10. Use Cases

- Educational demonstrations of audio synthesis
- Standalone instrument prototypes
- Integration with external audio applications
- Offline rendering without ProtoVM dependencies
- AI client workflows requiring compilable instrument code