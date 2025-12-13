#ifndef PLUGIN_SKELETON_EXPORT_H
#define PLUGIN_SKELETON_EXPORT_H

#include "String.h"  // Assuming String is defined in the codebase
#include "Result.h"
#include "AudioEngineCAbi.h"

enum class PluginTargetKind {
    Vst3,
    Lv2,
    Clap,
    Ladspa
};

struct PluginSkeletonOptions {
    PluginTargetKind target;
    String plugin_name;           // e.g. "ProtoVMHybridOsc"
    String plugin_id;             // e.g. "protovm.hybrid.osc"
    String vendor;                // e.g. "ProtoVM"
    int num_inputs = 0;           // for now: 0 (instrument)
    int num_outputs = 2;          // stereo
    bool emit_comment_banner = true;
};

class PluginSkeletonExport {
public:
    // Emit a plugin-oriented source snippet that wraps the C ABI engine.
    static Result<String> EmitPluginSkeletonSource(
        const PluginSkeletonOptions& opts
    );
};

#endif // PLUGIN_SKELETON_EXPORT_H