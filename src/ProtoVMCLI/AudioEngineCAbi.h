#ifndef AUDIO_ENGINE_C_ABI_H
#define AUDIO_ENGINE_C_ABI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Basic opaque handle
typedef struct ProtoVM_AudioEngine ProtoVM_AudioEngine;

// Simple parameter identifiers (index-based for now).
typedef enum ProtoVM_ParamId {
    PROTOVM_PARAM_MAIN_FREQ = 0,
    PROTOVM_PARAM_MAIN_GAIN = 1,
    PROTOVM_PARAM_PAN_DEPTH = 2,
    // reserve space for more
    PROTOVM_PARAM_COUNT
} ProtoVM_ParamId;

typedef struct ProtoVM_AudioEngineConfig {
    int sample_rate;    // e.g. 48000
    int max_block_size; // e.g. 1024
    int num_channels;   // For now: 2 (stereo)
    int voice_count;    // e.g. 4
} ProtoVM_AudioEngineConfig;

typedef struct ProtoVM_AudioEngineParams {
    float values[PROTOVM_PARAM_COUNT];
} ProtoVM_AudioEngineParams;

// Create/destroy
ProtoVM_AudioEngine* ProtoVM_AudioEngine_Create(const ProtoVM_AudioEngineConfig* cfg);
void ProtoVM_AudioEngine_Destroy(ProtoVM_AudioEngine* engine);

// Reset / flush state
void ProtoVM_AudioEngine_Reset(ProtoVM_AudioEngine* engine);

// Set parameters (RT-safe: just store values, no allocation)
void ProtoVM_AudioEngine_SetParams(ProtoVM_AudioEngine* engine,
                                   const ProtoVM_AudioEngineParams* params);

// Audio processing (non-interleaved stereo)
void ProtoVM_AudioEngine_Process(
    ProtoVM_AudioEngine* engine,
    const float* inL,
    const float* inR,
    float* outL,
    float* outR,
    int num_frames
);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_ENGINE_C_ABI_H