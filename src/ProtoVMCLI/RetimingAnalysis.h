#ifndef _ProtoVM_RetimingAnalysis_h_
#define _ProtoVM_RetimingAnalysis_h_

#include "RetimingModel.h"
#include "PipelineModel.h"
#include "CdcModel.h"
#include "TimingAnalysis.h"
#include "ScheduledIr.h"
#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>

namespace ProtoVMCLI {

class RetimingAnalysis {
public:
    // Analyze a single block for intra-domain retiming opportunities.
    static Result<Vector<RetimingPlan>> AnalyzeRetimingForBlock(
        const PipelineMap& pipeline,
        const CdcReport& cdc_report,
        const TimingAnalysis* timing = nullptr,          // optional
        const ScheduledModule* scheduled_ir = nullptr    // optional
    );

    // Optionally: analyze a subsystem (multi-block pipeline).
    static Result<Vector<RetimingPlan>> AnalyzeRetimingForSubsystem(
        const PipelineMap& pipeline,
        const CdcReport& cdc_report,
        const TimingAnalysis* timing = nullptr,
        const ScheduledModule* scheduled_ir = nullptr   // may be limited; subsystem-level scheduling is approximate
    );

private:
    // Helper methods
    static Result<Vector<RetimingPlan>> IdentifyCandidatePaths(
        const PipelineMap& pipeline,
        const CdcReport& cdc_report,
        const TimingAnalysis* timing,
        const ScheduledModule* scheduled_ir,
        const String& target_id
    );

    static Result<Vector<RetimingMove>> GenerateMovesForPath(
        const RegToRegPathInfo& path,
        const PipelineMap& pipeline,
        const CdcReport& cdc_report,
        const ScheduledModule* scheduled_ir
    );

    static RetimingMoveSafety DetermineSafety(
        const RegToRegPathInfo& path,
        const CdcReport& cdc_report,
        const Vector<String>& cdc_anchored_regs
    );

    static int EstimateDepthAfterMove(
        int before_depth,
        RetimingMoveDirection direction
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_RetimingAnalysis_h_