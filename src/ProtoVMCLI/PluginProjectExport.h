#ifndef PLUGIN_PROJECT_EXPORT_H
#define PLUGIN_PROJECT_EXPORT_H

#include "String.h"
#include "Result.h"
#include "InstrumentGraph.h"
#include "PluginSkeletonExport.h"  // For PluginTargetKind

struct PluginProjectExportOptions {
    PluginTargetKind target;       // Vst3, Lv2, Clap, Ladspa

    String plugin_name;            // Human-readable name
    String plugin_id;              // Unique ID / URI (format-dependent)
    String vendor;                 // Vendor / author name
    String version;                // e.g. "1.0.0"

    String output_dir;             // Path where project scaffold is written

    // Audio / instrument settings:
    int num_inputs = 0;            // For now: 0 (instrument)
    int num_outputs = 2;           // Stereo
    int default_sample_rate = 48000;
    int default_block_size = 512;
    int default_voice_count = 4;

    bool emit_readme = true;
    bool emit_build_files = true;
};

class PluginProjectExport {
public:
    // Generate a full plugin project scaffold on disk.
    static Result<void> ExportPluginProject(
        const ProtoVMCLI::InstrumentGraph& instrument,
        const PluginProjectExportOptions& opts
    );

private:
    static Result<void> ExportVst3Project(
        const ProtoVMCLI::InstrumentGraph& instrument,
        const PluginProjectExportOptions& opts
    );

    static Result<void> ExportLv2Project(
        const ProtoVMCLI::InstrumentGraph& instrument,
        const PluginProjectExportOptions& opts
    );

    static Result<void> ExportClapProject(
        const ProtoVMCLI::InstrumentGraph& instrument,
        const PluginProjectExportOptions& opts
    );

    static Result<void> ExportLadspaProject(
        const ProtoVMCLI::InstrumentGraph& instrument,
        const PluginProjectExportOptions& opts
    );

    static Result<void> CreateDirectories(const String& output_dir);
    static Result<void> WriteWrapperSource(
        const String& wrapper_path,
        const PluginProjectExportOptions& opts
    );
    static Result<void> WriteBuildFiles(
        const String& output_dir,
        const PluginProjectExportOptions& opts
    );
    static Result<void> WriteMetadataFiles(
        const String& output_dir,
        const PluginProjectExportOptions& opts
    );
    static Result<void> WriteReadme(
        const String& readme_path,
        const PluginProjectExportOptions& opts
    );
};

#endif // PLUGIN_PROJECT_EXPORT_H