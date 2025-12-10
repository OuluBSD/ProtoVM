# ProtoVM High-Level Code Generation Layer

## Overview

The ProtoVM High-Level Code Generation Layer provides a bridge from ProtoVM's circuit analysis and transformation capabilities to executable C/C++ code. This layer sits on top of the BehavioralAnalysis, HLS IR, Scheduled IR, and Structural Synthesis layers to generate functionally equivalent C or C++ code that represents the behavior of circuit blocks.

## Architecture and Components

### Codegen IR Layer

The Codegen IR layer provides a function-like view of circuit blocks:

- **Inputs** → Function parameters
- **Outputs** → Return values or out-parameters  
- **Registers** → State fields or struct members
- **Combinational logic** → Sequential C/C++ expressions
- **Sequential updates** → Register state updates per tick

### Core Components

#### 1. CodegenTargetLanguage
```cpp
enum class CodegenTargetLanguage {
    C,    // Generate C code
    Cpp   // Generate C++ code (future extension)
};
```

#### 2. Codegen IR Data Structures

##### CodegenValue
Represents a typed value in the code generation IR:
```cpp
struct CodegenValue {
    std::string name;             // Variable name
    std::string c_type;           // C/C++ type (e.g., "int", "float", "uint32_t")
    int bit_width;                // Bit width (-1 if unknown)
    CodegenStorageKind storage;   // Storage classification
    bool is_array = false;        // Whether it's an array
    int array_length = -1;        // Array length if applicable
};
```

##### CodegenExpr
Represents a C/C++ expression:
```cpp
struct CodegenExpr {
    CodegenExprKind kind;         // Value, UnaryOp, BinaryOp, TernaryOp, Call
    std::string op;               // Operator string ("+", "-", "==", etc.)
    std::vector<CodegenValue> args; // Expression arguments
    std::string literal;          // Literal value if applicable
};
```

##### CodegenAssignment
Represents a variable assignment:
```cpp
struct CodegenAssignment {
    CodegenValue target;          // Assignment target
    CodegenExpr  expr;            // Assignment expression
};
```

##### CodegenModule
Represents a complete function-like module:
```cpp
struct CodegenModule {
    std::string id;                               // Module ID
    std::string block_id;                         // Source block ID
    
    std::vector<CodegenValue> inputs;             // Function inputs
    std::vector<CodegenValue> outputs;            // Function outputs
    std::vector<CodegenValue> locals;             // Local variables
    std::vector<CodegenValue> state;              // Persistent state
    
    std::vector<CodegenAssignment> comb_assigns;  // Combinational computations
    std::vector<CodegenAssignment> state_updates; // Sequential updates
    
    bool is_oscillator_like = false;              // Oscillator detection flag
    std::string behavior_summary;                 // Behavioral description
};
```

## Codegen IR Inference

The `CodegenIrInference` class converts ProtoVM IR and behavioral analysis results into the Codegen IR format.

### Key Methods

- `BuildCodegenModuleForBlockInBranch()` - Build CodegenModule from a circuit block
- `BuildCodegenModuleForNodeRegionInBranch()` - Build CodegenModule from a node region

The inference engine uses BehavioralAnalysis and HlsIr to create a function-like view of the circuit block, with special handling for oscillator-like circuits.

## Code Emission Engine

The `CodeEmitter` class converts CodegenModule instances into actual C/C++ code.

### Key Methods

- `EmitCodeForModule()` - Generate C/C++ function from CodegenModule
- `EmitOscillatorDemo()` - Generate oscillator demo code (for oscillator-like blocks)

### C Code Generation Pattern

```c
typedef struct {
    // State variables
    float phase;
    float freq;
    // ... other registers
} BlockState;

void BlockStep(BlockState* s, 
               float in1, float in2,           // inputs
               float* out1, float* out2)       // outputs
{
    // Local temporary variables
    float tmp0;
    
    // Combinational assignments
    tmp0 = in1 + in2;
    
    // Output assignments
    *out1 = tmp0;
    
    // State updates
    s->phase = s->phase + s->freq;
}
```

## CLI Commands

### 1. `codegen-block-ir`
Inspect the CodegenModule structure for a circuit block.

**Parameters:**
- `--workspace` - Path to workspace
- `--session-id` - Session ID
- `--branch` - Branch name (optional)
- `--block-id` - Block ID

**Output:** JSON representation of the CodegenModule

### 2. `codegen-block-c`
Generate C/C++ code for a circuit block.

**Parameters:**
- `--workspace` - Path to workspace
- `--session-id` - Session ID
- `--branch` - Branch name (optional)
- `--block-id` - Block ID
- `--lang` - Language (`C` or `Cpp`, defaults to `C`)
- `--state-struct-name` - Name for state struct (defaults to `BlockState`)
- `--function-name` - Name for step function (defaults to `BlockStep`)
- `--emit-state-struct` - Whether to emit state struct (defaults to `true`)

**Output:** Generated C/C++ code

### 3. `codegen-block-osc-demo` (Optional)
Generate oscillator demo code for oscillator-like blocks.

**Parameters:**
- `--workspace` - Path to workspace
- `--session-id` - Session ID
- `--branch` - Branch name (optional)
- `--block-id` - Block ID
- `--lang` - Language (`C` or `Cpp`, defaults to `C`)
- `--state-struct-name` - Name for oscillator state struct
- `--step-function-name` - Name for oscillator step function
- `--render-function-name` - Name for oscillator render function

**Output:** Oscillator demo code with sample rendering and stereo panning

## CoDesigner Endpoints

### 1. `designer-codegen-block-c`
Generate C/C++ code for a block in the designer context.

**Request:**
```json
{
  "command": "designer-codegen-block-c",
  "designer_session_id": "cd-1234",
  "block_id": "OSC1",
  "language": "C",
  "emit_state_struct": true,
  "state_struct_name": "OscState",
  "function_name": "OscStep"
}
```

**Response:**
```json
{
  "ok": true,
  "command": "designer-codegen-block-c",
  "data": {
    "designer_session": { ... },
    "result": {
      "block_id": "OSC1",
      "language": "C",
      "code": "/* generated code */",
      "state_struct_name": "OscState",
      "function_name": "OscStep"
    }
  }
}
```

### 2. `designer-codegen-osc-demo`
Generate oscillator demo code in the designer context.

**Request:**
```json
{
  "command": "designer-codegen-osc-demo",
  "designer_session_id": "cd-1234",
  "block_id": "OSC1",
  "language": "C"
}
```

**Response:**
```json
{
  "ok": true,
  "command": "designer-codegen-osc-demo",
  "data": {
    "designer_session": { ... },
    "result": {
      "block_id": "OSC1",
      "language": "C",
      "osc_code": "/* generated oscillator demo */"
    }
  }
}
```

## Relationship to Other Components

The Code Generation Layer depends on:
- **BehavioralAnalysis** - For behavioral classification and description
- **HlsIr** - For the high-level synthesis IR
- **ScheduledIr** - For pipeline-aware code generation
- **StructuralSynthesis** - For oscillator detection and pattern recognition

## Oscillator / DSP Support

The system has special support for oscillator-like circuits:

### Detection
- Behavioral analysis identifies phase accumulators and frequency controls
- Pattern matching identifies oscillator-like structures
- Sets `is_oscillator_like = true` in CodegenModule

### Demo Generation
- Generates sample rendering functions
- Includes stereo panning using sine LFO
- Provides a complete audio processing pipeline

### Example Output
```c
void OscStep(OscState* s, float* sample_out) {
    // Oscillator logic
    *sample_out = sinf(s->phase);
    s->phase += s->frequency;
}

void OscRender(OscState* s, float* outL, float* outR, int n) {
    for (int i = 0; i < n; ++i) {
        float sample;
        OscStep(s, &sample);
        
        float pan = sinf(s->phase * 0.1f);  // Simple LFO
        float pan_normalized = (pan + 1.0f) * 0.5f;
        
        outL[i] = sample * (0.5f * (1.0f - pan_normalized));
        outR[i] = sample * (0.5f * (1.0f + pan_normalized));
    }
}
```

## API Integration

The code generation layer is fully integrated with:
- CircuitFacade for high-level operations
- CommandDispatcher for CLI commands
- SessionServer for daemon commands
- CoDesigner for AI-assisted design workflows

## Design Goals

- **Clarity over Performance** - Generated code prioritizes readability and structural correctness
- **Function-like View** - Convert circuit blocks to familiar function paradigm
- **State Management** - Proper handling of persistent registers/state
- **DSP Readiness** - Specialized support for audio/oscillator applications
- **Extensibility** - Easy to add new target languages or output formats

## Future Extensions

- Generation of SIMD/AVX-optimized code
- More sophisticated DSP patterns (filters, envelopes, etc.)
- Support for vectorized processing
- Integration with real-time audio frameworks
- C++ class generation with methods