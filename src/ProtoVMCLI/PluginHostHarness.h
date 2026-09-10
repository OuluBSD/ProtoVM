#ifndef _PROTOPVM_PROTOVMCLI_PluginHostHarness_h
#define _PROTOPVM_PROTOVMCLI_PluginHostHarness_h

#include <Upp/Upp.h>
#include "../ProtoVMCommon/Result.h"
#include "AudioQa.h"

namespace Upp {

enum class HostPluginKind {
    Clap,
    Ladspa,
    Lv2,
    Vst3
};

struct HostLoadOptions {
    HostPluginKind kind;
    String plugin_path;      // shared library path or bundle root
    String plugin_id;        // CLAP id, LV2 URI, LADSPA index/name (optional)
    int sample_rate = 48000;
    int max_block_size = 512;
    int num_inputs = 0;
    int num_outputs = 2;
};

struct HostRunOptions {
    double duration_sec = 3.0;
    bool wallclock_paced = false;     // optional sleep to simulate real time

    // Parameter automation:
    bool enable_param_automation = false;
    String param_name;               // for CLAP: param id/name (best-effort)
    int param_index = -1;            // for LADSPA: control port index
    double param_start = 0.0;
    double param_end   = 1.0;

    // Stress loop:
    Vector<int> block_sizes;         // if empty: fixed max_block_size
    bool enable_denormal_flush = true;
};

struct HostRenderedSummary {
    int num_frames = 0;
    double duration_sec = 0.0;
    int sample_rate = 0;
    int max_block_size = 0;
};

struct HostArtifactPaths {
    String wav_path;           // optional, if saved
    String artifact_path;      // resolved plugin binary (if discoverable)
};

struct HostRunResult {
    HostRenderedSummary rendered;
    HostArtifactPaths artifacts;

    AudioQaReport qa_report;          // from Phase 29A
    Vector<String> host_warnings;     // loader/host notes
};

class PluginHostHarness {
public:
    static Result<HostRunResult> LoadRunAndAnalyze(
        const HostLoadOptions& load,
        const HostRunOptions& run,
        const Optional<String>& save_wav_path
    );
};

}

#endif