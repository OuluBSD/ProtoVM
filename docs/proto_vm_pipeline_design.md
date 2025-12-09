# ProtoVM Pipeline Analysis Design

## Overview

This document describes the design of the clock-domain and pipeline reasoning layer in ProtoVM. This functionality enables ProtoVM to identify and analyze:

- Clock domains in circuits
- Register classification and organization
- Pipeline stage identification
- Register-to-register paths and their characteristics
- Cross-domain boundaries and potential synchronization issues

## Key Concepts

### Clock Domains
A clock domain consists of a set of registers that are synchronized by the same clock signal. The system identifies clock domains by analyzing clock signals in the circuit and mapping registers to these signals.

### Pipeline Stages
A pipeline stage consists of combinational logic between register stages. Registers are grouped into stages based on their logical position in the data path.

### Register-to-Register Paths
These represent the combinational paths between registers, with characteristics including:
- Depth of logic elements
- Clock domain (same or different)
- Stage span (number of pipeline stages between source and destination)

## Data Structures

### ClockSignalInfo
```cpp
struct ClockSignalInfo {
    std::string signal_name;    // e.g. "CLK", "CPU_CLK"
    int    domain_id;      // numeric domain identifier
};
```

Represents a unique clock signal in the circuit that defines a clock domain.

### RegisterInfo
```cpp
struct RegisterInfo {
    std::string reg_id;         // stable ID (e.g. component:pin or block-local ID)
    std::string name;           // human-friendly name if available
    std::string clock_signal;   // which signal clocks this register
    int    domain_id;      // resolved clock domain id
    std::string reset_signal;   // optional
};
```

Describes a register and its association with a clock domain.

### PipelineStageInfo
```cpp
struct PipelineStageInfo {
    int   stage_index;            // e.g. 0..N-1
    int   domain_id;              // associated clock domain
    std::vector<std::string> registers_in;  // reg_ids that feed into this stage
    std::vector<std::string> registers_out; // reg_ids driven by this stage
    int   comb_depth_estimate;    // approximate logic depth
};
```

Represents a logical pipeline stage with input and output registers.

### RegToRegPathInfo
```cpp
struct RegToRegPathInfo {
    std::string src_reg_id;
    std::string dst_reg_id;
    int    domain_id;             // domain if same; -1 if cross-domain
    int    comb_depth_estimate;   // approximate depth between them
    int    stage_span;            // how many pipeline stages between src and dst (0,1,2,...)
    bool   crosses_clock_domain;  // true if src and dst in different domains
};
```

Describes a path between two registers with timing and domain information.

### PipelineMap
```cpp
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
```

The top-level structure containing all pipeline analysis information for a block or subsystem.

## Analysis Engine

### PipelineAnalysis Class

The `PipelineAnalysis` class provides static methods for building pipeline maps:

```cpp
class PipelineAnalysis {
public:
    // Build a pipeline map for a single block.
    static Result<PipelineMap> BuildPipelineMapForBlock(
        const CircuitGraph& graph,
        const TimingAnalysis* timing,         // optional
        const ScheduledModule* scheduled_ir,  // optional
        const std::string& block_id
    );

    // Optional: Build a pipeline map for a subsystem (multi-block).
    static Result<PipelineMap> BuildPipelineMapForSubsystem(
        const CircuitGraph& graph,
        const TimingAnalysis* timing,
        const ScheduledModule* scheduled_ir,  // may be omitted or approximate
        const std::string& subsystem_id,
        const std::vector<std::string>& block_ids
    );
};
```

The analysis uses multiple input sources for the most accurate results:

- **CircuitGraph**: Provides the structural representation of the circuit
- **TimingAnalysis**: Offers timing path information for depth estimation
- **ScheduledModule**: Provides pipeline stage information from scheduling results
- **BehavioralAnalysis**: Helps identify register-like components

## Integration with CircuitFacade

The `CircuitFacade` class integrates the pipeline analysis with the branch-aware architecture:

```cpp
Result<PipelineMap> BuildPipelineMapForBlockInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id
);

Result<PipelineMap> BuildPipelineMapForSubsystemInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& subsystem_id,
    const std::vector<std::string>& block_ids
);
```

## CLI Commands

### pipeline-block

The `pipeline-block` command analyzes a single block and returns its pipeline structure:

```
proto-vm-cli pipeline-block --workspace <path> --session-id <id> --block-id <id> --branch <name>
```

The daemon version accepts a JSON payload:
```json
{
  "command": "pipeline-block",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "block_id": "B_PIPE_STAGE1",
    "branch": "main"
  }
}
```

### pipeline-subsystem

The `pipeline-subsystem` command analyzes multiple blocks as a combined subsystem:

```
proto-vm-cli pipeline-subsystem --workspace <path> --session-id <id> --subsystem-id <id> --block-ids <id1,id2,id3> --branch <name>
```

The daemon version accepts a JSON payload:
```json
{
  "command": "pipeline-subsystem",
  "workspace": "/path/to/workspace",
  "session_id": 1,
  "payload": {
    "subsystem_id": "ALU_PIPE",
    "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"],
    "branch": "main"
  }
}
```

## JSON Serialization

The pipeline data structures are fully serializable to JSON through the `JsonIO` class:

- `ClockSignalInfoToValueMap()`
- `RegisterInfoToValueMap()`
- `PipelineStageInfoToValueMap()`
- `RegToRegPathInfoToValueMap()`
- `PipelineMapToValueMap()`
- And their corresponding array serialization methods

## Design Considerations

### Heuristic Analysis

The pipeline analysis uses heuristics to identify clock signals and registers:
- Clock signals are identified by name patterns (e.g., containing "CLK", "clk", or "CLOCK")
- Register components are identified by type patterns (e.g., containing "FF", "REG", or "LATCH")

In the future, more sophisticated analysis techniques could improve accuracy.

### Timing Estimation

When scheduled IR is available, the analysis uses stage information from the scheduling results. Otherwise, it estimates pipeline stages using circuit structure and timing analysis.

### Cross-Domain Detection

The system identifies potential clock domain crossing paths and marks them for further analysis, though full CDC verification is outside the scope of this implementation.

## Limitations

- The analysis is read-only and does not modify the circuit
- Clock detection is heuristic-based and may not identify all clock signals
- Register identification is also heuristic-based and may miss some register-like structures
- No formal CDC verification is performed; detection is heuristic
- Depth estimates are approximations and may not reflect actual gate delays