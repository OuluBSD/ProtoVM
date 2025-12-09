#pragma once

#include "CdcModel.h"
#include "PipelineModel.h"
#include "CircuitGraph.h"
#include "TimingAnalysis.h"

using namespace Upp;

class CdcAnalysis {
public:
    // Build a CDC report for a single block using its pipeline map and graph.
    static Result<CdcReport> BuildCdcReportForBlock(
        const PipelineMap& pipeline,
        const CircuitGraph& graph,
        const TimingAnalysis* timing = nullptr
    );

    // Optional: Build a CDC report for a subsystem (multi-block).
    static Result<CdcReport> BuildCdcReportForSubsystem(
        const PipelineMap& pipeline,
        const CircuitGraph& graph,
        const TimingAnalysis* timing = nullptr
    );

private:
    static CdcCrossingKind ClassifyCrossing(const CdcCrossing& crossing, 
                                           const CircuitGraph& graph);
    static CdcSeverity DetermineSeverity(CdcCrossingKind kind);
    static String GenerateSummary(CdcCrossingKind kind, 
                                 const CdcCrossingEndpoint& src, 
                                 const CdcCrossingEndpoint& dst,
                                 int bit_width);
    static String GenerateDetail(CdcCrossingKind kind, 
                                const CdcCrossingEndpoint& src, 
                                const CdcCrossingEndpoint& dst,
                                int bit_width);
};