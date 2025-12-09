#ifndef _ProtoVM_Scheduling_h_
#define _ProtoVM_Scheduling_h_

#include "ScheduledIr.h"
#include "TimingAnalysis.h"
#include "CircuitGraph.h"
#include "HlsIr.h"
#include "SessionTypes.h"
#include <vector>

namespace ProtoVMCLI {

enum class SchedulingStrategy {
    SingleStage,           // all comb ops in stage 0
    DepthBalancedStages,   // split by depth into N stages
    FixedStageCount        // user-specified N
};

struct SchedulingConfig {
    SchedulingStrategy strategy;
    int requested_stages;       // used for FixedStageCount, else ignored or advisory
    
    SchedulingConfig() : strategy(SchedulingStrategy::SingleStage), requested_stages(1) {}
    SchedulingConfig(SchedulingStrategy strat, int stages) : strategy(strat), requested_stages(stages) {}
};

class SchedulingEngine {
public:
    // Build a ScheduledModule from an IrModule and optional timing/graph info.
    static Result<ScheduledModule> BuildSchedule(
        const IrModule& ir,
        const TimingAnalysis* timing,          // optional pointer
        const CircuitGraph* graph,            // optional pointer
        const SchedulingConfig& config
    );
    
    // Helper method to compute timing depth for each expression in the module
    static Result<std::vector<int>> ComputeTimingDepths(
        const IrModule& ir,
        const TimingAnalysis* timing,          // optional pointer
        const CircuitGraph* graph             // optional pointer
    );
    
    // Helper method to assign stages to expressions based on their depths
    static Result<std::vector<StageIndex>> AssignStages(
        const std::vector<int>& depths,
        int num_stages,
        const SchedulingConfig& config
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_Scheduling_h_