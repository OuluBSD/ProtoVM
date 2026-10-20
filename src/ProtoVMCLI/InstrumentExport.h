#pragma once

#include <ProtoVM/ProtoVM.h>  // For Upp types (String, Vector)
#include <string>
#include <vector>

#include "JsonIO.h"
#include "InstrumentGraph.h"

namespace ProtoVM {

    struct InstrumentExportOptions {
        std::string program_name;          // e.g. "hybrid_instrument_demo"
        std::string namespace_name;        // optional, default empty
        bool include_wav_writer = true;
        std::string output_wav_filename;   // e.g. "output.wav"
        bool emit_comment_banner = true;
    };

    class InstrumentExport {
    public:
        // Export a standalone C++ program for a given instrument.
        static ProtoVMCLI::Result<std::string> EmitStandaloneCppForInstrument(
            const ProtoVMCLI::InstrumentGraph& instrument,
            const InstrumentExportOptions& options
        )
    {
        std::string code;
        code += "// Standalone C++ emission for instrument: " + instrument.instrument_id.ToStd() + "\n";
        code += "// namespace: " + options.namespace_name + "\n";
        code += "#include <cstdint>\n\n";
        code += "namespace " + (options.namespace_name.empty() ? std::string("instrument") : options.namespace_name) + " {\n\n";
        code += "// Generated from the ProtoVM instrument graph.\n";
        code += "constexpr double kSampleRateHz = " + std::to_string(instrument.sample_rate_hz) + ";\n";
        code += "constexpr int kVoiceCount = " + std::to_string(instrument.voice_count) + ";\n\n";
        code += "} // namespace\n";
        return ProtoVMCLI::Result<std::string>::MakeOk(code);
    }
    };

}  // namespace ProtoVM