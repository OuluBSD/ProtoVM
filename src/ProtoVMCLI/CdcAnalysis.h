#pragma once

#include "SessionTypes.h"  // For Result<T>
#include "CdcModel.h"
#include "PipelineModel.h"
#include "CircuitGraph.h"
#include "TimingAnalysis.h"
#include <vector>

class CdcAnalysis {
public:
    // Build a CDC report for a single block using its pipeline map and graph.
    static ProtoVMCLI::Result<CdcReport> BuildCdcReportForBlock(
        const ProtoVMCLI::PipelineMap& pipeline,
        const ProtoVMCLI::CircuitGraph& graph,
        const ProtoVMCLI::TimingAnalysis* timing = nullptr
    );

    // Optional: Build a CDC report for a subsystem (multi-block).
    static ProtoVMCLI::Result<CdcReport> BuildCdcReportForSubsystem(
        const ProtoVMCLI::PipelineMap& pipeline,
        const ProtoVMCLI::CircuitGraph& graph,
        const ProtoVMCLI::TimingAnalysis* timing = nullptr
    );

private:
    static CdcCrossingKind ClassifyCrossing(const CdcCrossing& crossing,
                                           const ProtoVMCLI::CircuitGraph& graph);
    static CdcSeverity DetermineSeverity(CdcCrossingKind kind);
    static std::string GenerateSummary(CdcCrossingKind kind,
                                 const CdcCrossingEndpoint& src,
                                 const CdcCrossingEndpoint& dst,
                                 int bit_width);
    static std::string GenerateDetail(CdcCrossingKind kind,
                                const CdcCrossingEndpoint& src,
                                const CdcCrossingEndpoint& dst,
                                int bit_width);
};