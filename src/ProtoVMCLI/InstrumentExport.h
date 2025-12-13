#pragma once

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
        static Result<std::string> EmitStandaloneCppForInstrument(
            const InstrumentGraph& instrument,
            const InstrumentExportOptions& options
        );
    };

}  // namespace ProtoVM