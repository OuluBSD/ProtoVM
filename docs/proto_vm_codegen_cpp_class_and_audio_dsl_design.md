# ProtoVM C++ Class & Audio DSL Code Generation Design

## Overview

This document describes the C++ Class & Audio DSL code generation feature in ProtoVM. This feature allows users to generate high-level C++ wrapper classes around generated logic blocks and emit a small Audio DSL/configuration stub that describes how to wire generated blocks into an audio rendering loop.

## Architecture Components

### 1. CppClassOptions

The `CppClassOptions` struct defines configuration options for C++ class generation:

```cpp
struct CppClassOptions {
    std::string class_name;           // e.g. "OscBlock"
    std::string state_class_name;     // e.g. "OscState"
    std::string namespace_name;       // optional namespace, empty if none

    bool generate_render_method = false;  // if true, emit render() that loops step()
    std::string step_method_name = "Step";     // e.g. "Step"
    std::string render_method_name = "Render"; // e.g. "Render";

    double default_sample_rate = 48000.0; // used for demo code & audio DSL
};
```

### 2. Audio DSL Structures

The Audio DSL consists of three main structures that define audio parameters:

#### AudioDslOscillator
```cpp
struct AudioDslOscillator {
    std::string id;              // e.g. "osc1"
    double frequency_hz;         // e.g. ~440.0 for A4 note
};
```

#### AudioDslPanLfo
```cpp
struct AudioDslPanLfo {
    std::string id;              // e.g. "pan_lfo1"
    double rate_hz;              // e.g. 0.25 (one full L->R->L cycle in 4s)
};
```

#### AudioDslOutputConfig
```cpp
struct AudioDslOutputConfig {
    double sample_rate_hz;       // e.g. 48000.0
    double duration_sec;         // e.g. 3.0
};
```

#### AudioDslGraph
```cpp
struct AudioDslGraph {
    std::string block_id;        // underlying block this maps to (oscillator-like)
    AudioDslOscillator osc;
    AudioDslPanLfo pan_lfo;
    AudioDslOutputConfig output;
};
```

## Code Emitter Enhancements

### EmitCppClassForModule

The `CodeEmitter::EmitCppClassForModule` method now generates C++ class wrappers around `CodegenModule` instances. The generated class includes:

- A state struct matching the underlying module's state variables
- A `Step()` method that executes one time-step of the module logic
- An optional `Render()` method that loops the `Step()` method for multi-sample processing

### EmitAudioDemoForOscillator

The `CodeEmitter::EmitAudioDemoForOscillator` method generates a complete, self-contained C++ demo that:

1. Includes necessary headers (`<cmath>`, `<vector>`, etc.)
2. Emits the generated C++ oscillator class
3. Implements a `main()` function that:
   - Sets up a sample rate (e.g., 48kHz) 
   - Generates 3 seconds of audio samples
   - Implements a 440Hz sine wave oscillator
   - Applies a pan LFO at 0.25 Hz to move signal from left to right and back
   - Outputs stereo samples to buffers
   - Optionally writes samples to a text file for inspection

## CircuitFacade Extensions

The `CircuitFacade` class has been extended with new methods:

### C++ Class Emission Methods

- `EmitCppClassForBlockInBranch` - Generates C++ class wrapper for a block in a given branch
- `BuildAudioDslForOscillatorBlockInBranch` - Builds Audio DSL configuration for an oscillator block
- `EmitAudioDemoForOscillatorBlockInBranch` - Generates complete audio demo for an oscillator block

## CLI Commands

### Standard CLI Commands

#### `codegen-block-cpp-class`

Generates a C++ class wrapper for a given block:

```
protovm --command codegen-block-cpp-class \
        --workspace /path/to/workspace \
        --session-id 123 \
        --branch main \
        --block-id OSC1 \
        --class-name Oscillator \
        --state-class-name OscState \
        --namespace Audio \
        --render-method
```

##### Response Format:
```json
{
  "ok": true,
  "command": "codegen-block-cpp-class",
  "data": {
    "session_id": 123,
    "branch": "main",
    "block_id": "OSC1",
    "language": "Cpp",
    "code": "/* Generated C++ class code */"
  }
}
```

#### `codegen-block-audio-demo`

Generates a complete audio demo for an oscillator block:

```
protovm --command codegen-block-audio-demo \
        --workspace /path/to/workspace \
        --session-id 123 \
        --branch main \
        --block-id OSC1 \
        --freq-hz 440.0 \
        --pan-lfo-hz 0.25 \
        --sample-rate 48000.0 \
        --duration-sec 3.0
```

##### Response Format:
```json
{
  "ok": true,
  "command": "codegen-block-audio-demo",
  "data": {
    "session_id": 123,
    "branch": "main",
    "block_id": "OSC1",
    "language": "Cpp",
    "audio_dsl": {
      "block_id": "OSC1",
      "osc": { "id": "osc1", "frequency_hz": 440.0 },
      "pan_lfo": { "id": "pan_lfo1", "rate_hz": 0.25 },
      "output": { "sample_rate_hz": 48000.0, "duration_sec": 3.0 }
    },
    "code": "/* Complete C++ demo code */"
  }
}
```

## CoDesigner Commands

### Designer Commands

#### `designer-codegen-block-cpp-class`

CoDesigner version of C++ class generation:

```json
{
  "command": "designer-codegen-block-cpp-class",
  "payload": {
    "designer_session_id": "cd-1234",
    "block_id": "OSC1",
    "class_name": "OscBlock",
    "state_class_name": "OscState",
    "namespace": "Audio",
    "render_method": true
  }
}
```

#### `designer-codegen-block-audio-demo`

CoDesigner version of audio demo generation:

```json
{
  "command": "designer-codegen-block-audio-demo",
  "payload": {
    "designer_session_id": "cd-1234",
    "block_id": "OSC1",
    "freq_hz": 440.0,
    "pan_lfo_hz": 0.25,
    "sample_rate": 48000.0,
    "duration_sec": 3.0
  }
}
```

## Example Output

Here's an example of what the generated C++ class might look like:

```cpp
// Auto-generated C++ class from ProtoVM codegen
#include <stdint.h>
#include <stdbool.h>
#include <cmath>

struct OscState {
    float phase;
    float freq;
    // plus any other state CodegenModule requires
};

class OscBlock {
public:
    OscBlock() { /* optional ctor/init */ }

    void Step(OscState& s, float* outL, float* outR, double sample_rate);
    // or: void Step(OscState& s, float& outL, float& outR, double sample_rate);

    // Optional render method
    void Render(OscState& s,
                float* outL, float* outR,
                int num_samples,
                double sample_rate);

private:
    // local temporaries can be kept inside Step.
};

void OscBlock::Step(OscState& s, float* outL, float* outR, double sample_rate) {
    // Declarations of local variables
    // Process combinational assignments
    // Process state updates
    // Set default output values if needed
}
```

## Audio Demo Example

The audio demo generated would implement:
- A 440 Hz sine wave oscillator (A4 note)
- A pan LFO that moves the signal from left to right and back in 4 seconds (0.25 Hz)
- At least 3 seconds of audio rendered at the specified sample rate
- Output to left and right channels with the panning effect

The generated demo is fully self-contained C++ code that can be compiled separately from the ProtoVM system.

## Implementation Notes

1. The generated code is designed to work with oscillator-like blocks that have been identified as such during behavioral analysis.
2. The C++ classes are designed to be easily integrated into audio processing pipelines.
3. The Audio DSL provides a declarative way to specify audio parameters without dealing with implementation details.
4. All generated code is pure C++ without dependencies on ProtoVM headers for standalone compilation.

## Future Enhancements

1. More sophisticated audio effects integration (filters, envelopes, etc.)
2. Integration with real audio APIs (JUCE, PortAudio, etc.)
3. Additional oscillator types beyond sine waves
4. More complex audio routing configurations