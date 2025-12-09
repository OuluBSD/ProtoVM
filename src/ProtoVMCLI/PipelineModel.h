#ifndef _ProtoVM_PipelineModel_h_
#define _ProtoVM_PipelineModel_h_

#include "SessionTypes.h"  // For Result<T>
#include "ProtoVM.h"       // For Upp types
#include <string>
#include <vector>

namespace ProtoVMCLI {

struct ClockSignalInfo {
    std::string signal_name;    // e.g. "CLK", "CPU_CLK"
    int    domain_id;      // numeric domain identifier
};

struct RegisterInfo {
    std::string reg_id;         // stable ID (e.g. component:pin or block-local ID)
    std::string name;           // human-friendly name if available
    std::string clock_signal;   // which signal clocks this register
    int    domain_id;      // resolved clock domain id
    std::string reset_signal;   // optional
};

struct PipelineStageInfo {
    int   stage_index;            // e.g. 0..N-1
    int   domain_id;              // associated clock domain
    std::vector<std::string> registers_in;  // reg_ids that feed into this stage
    std::vector<std::string> registers_out; // reg_ids driven by this stage
    int   comb_depth_estimate;    // approximate logic depth
};

struct RegToRegPathInfo {
    std::string src_reg_id;
    std::string dst_reg_id;
    int    domain_id;             // domain if same; -1 if cross-domain
    int    comb_depth_estimate;   // approximate depth between them
    int    stage_span;            // how many pipeline stages between src and dst (0,1,2,...)
    bool   crosses_clock_domain;  // true if src and dst in different domains
};

struct PipelineMap {
    std::string id;  // block id or subsystem id

    // Clock domains involved
    std::vector<ClockSignalInfo> clock_domains;

    // Registers and their domains
    std::vector<RegisterInfo> registers;

    // Pipeline stages
    std::vector<PipelineStageInfo> stages;

    // Register-to-register paths of interest
    std::vector<RegToRegPathInfo> reg_paths;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_PipelineModel_h_