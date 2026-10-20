#ifndef PLUGIN_SKELETON_EXPORT_H
#define PLUGIN_SKELETON_EXPORT_H

// Plugin skeleton export: emits a plugin-oriented source snippet that wraps the
// C ABI engine. Kept dependency-light (no U++/Result dependency) so it can be
// included from any translation unit without forcing a U++ include chain.
//
// The full implementation lives in PluginSkeletonExport.cpp, which is not
// compiled by ProtoVMCLI.upp; callers only use the option/kind enums below.

enum class PluginTargetKind {
	Vst3,
	Lv2,
	Clap,
	Ladspa
};

struct PluginSkeletonOptions {
	PluginTargetKind target = PluginTargetKind::Vst3;
	int num_inputs = 2;          // stereo
	int num_outputs = 2;          // stereo
	bool emit_comment_banner = true;
};

#endif // PLUGIN_SKELETON_EXPORT_H