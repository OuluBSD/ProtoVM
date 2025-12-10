# Global Pipelining Design

## 1. Motivation and Role

The Global Pipelining Engine is a core enhancement to ProtoVM that enables pipeline analysis and optimization across multiple blocks within a subsystem. While previous phases implemented block-level pipeline analysis and retiming capabilities, this phase introduces the ability to understand and optimize entire datapaths that span multiple blocks.

The Global Pipelining Engine:

- Creates a unified view of pipeline stages across multiple blocks
- Identifies end-to-end paths and critical depths spanning block boundaries
- Proposes coordinated retiming strategies across blocks to achieve system-level objectives
- Preserves local block functionality while optimizing global performance

## 2. Key Concepts

### 2.1 Global Pipeline Model
The `GlobalPipelineMap` represents a unified pipeline view of a subsystem containing multiple blocks. It includes:

- **Global stages**: Pipeline stages that span multiple blocks
- **End-to-end paths**: Paths from subsystem inputs to outputs
- **Cross-block registers**: Registers that conceptually belong to the same stage across blocks
- **Global metrics**: System-level depth and latency measurements

### 2.2 Global Pipelining Strategies
The engine supports several optimization strategies:

- **BalanceStages**: Equalize combinational depth across global pipeline stages
- **ReduceCriticalPath**: Minimize the maximum end-to-end depth while respecting latency constraints

### 2.3 Global Pipelining Plans
A global pipelining plan contains:

- Local retiming plans for each block in the subsystem
- Verification that CDC constraints are preserved across blocks
- Estimated global performance improvements

## 3. Data Structures

### 3.1 Global Pipeline Path (`GlobalPipelinePath`)
Represents an end-to-end path within a subsystem:
- `path_id`: Unique identifier for the path
- `reg_ids`: Sequence of registers along the path
- `block_ids`: Blocks involved along the path
- `total_stages`: Number of register-to-register hops
- `total_comb_depth_estimate`: Total estimated depth of the path

### 3.2 Global Pipeline Stage (`GlobalPipelineStage`)
Represents a global "stage band" across multiple blocks:
- `stage_index`: 0..N-1 index along reference paths
- `domain_id`: Clock domain of the stage
- `reg_ids`: Registers belonging to this stage across blocks
- `block_ids`: Blocks contributing logic to this stage
- `max_comb_depth_estimate`: Worst-case depth in this stage
- `avg_comb_depth_estimate`: Average depth in this stage

### 3.3 Global Pipeline Map (`GlobalPipelineMap`)
Represents the global pipeline model for a subsystem:
- `subsystem_id`: Identifier of the subsystem
- `block_ids`: Blocks in the subsystem
- `clock_domains`: Clock domains in the subsystem
- `stages`: Global pipeline stages
- `paths`: End-to-end paths in the subsystem
- `max_total_depth`: Maximum depth across all stages
- `max_stages`: Maximum number of stages

### 3.4 Global Pipelining Objective (`GlobalPipeliningObjective`)
Defines the optimization objective:
- `kind`: Strategy (BalanceStages or ReduceCriticalPath)
- `target_stage_count`: Desired number of stages
- `target_max_depth`: Desired maximum depth per stage
- `max_extra_registers`: Budget for added registers
- `max_total_moves`: Budget for retiming moves

### 3.5 Global Pipelining Plan (`GlobalPipeliningPlan`)
Contains a complete global optimization strategy:
- `id`: Plan identifier
- `subsystem_id`: Target subsystem
- `block_ids`: Blocks involved
- `objective`: Optimization objective
- `steps`: Per-block retiming steps to apply
- `estimated_global_depth_before/after`: Performance estimates
- `respects_cdc_fences`: Whether CDC constraints are preserved

## 4. Interaction with Other Components

### 4.1 Interaction with PipelineMap
- `GlobalPipelineMap` is built from multiple per-block `PipelineMap`s
- Maintains consistency with local pipeline analyses
- Aggregates timing and stage information from individual blocks

### 4.2 Interaction with RetimingOpt
- Uses local retiming optimization results to build global strategies
- Combines results from `RetimingOptimizer` across blocks
- Maintains safety guarantees provided by retiming infrastructure

### 4.3 Interaction with RetimingTransform
- Delegates actual transformations to `RetimingTransform`
- Applies global plans as sequences of local transformations
- Preserves behavioral equivalence through verified local transforms

### 4.4 CDC Constraints
- Respects CDC boundaries within and between blocks
- Verifies that global transformations don't introduce new hazards
- Maintains CDC fence compliance across the subsystem

## 5. CLI & Daemon Usage

### 5.1 CLI Commands

#### `global-pipeline-subsystem`
Analyze global pipeline structure of a subsystem.

Parameters:
- `--workspace`: Workspace path
- `--session-id`: Session identifier
- `--branch`: Branch name (optional, defaults to main)
- `--subsystem-id`: Subsystem identifier
- `--block-ids`: Comma-separated list of block IDs

Example:
```bash
./proto-vm-cli global-pipeline-subsystem --workspace ./workspace --session-id 1 --subsystem-id ALU_PIPE --block-ids ALU_STAGE1,ALU_STAGE2,ALU_FLAGS
```

#### `global-pipeline-opt-subsystem`
Propose global pipelining optimization plans for a subsystem.

Parameters:
- `--workspace`: Workspace path
- `--session-id`: Session identifier
- `--branch`: Branch name
- `--subsystem-id`: Subsystem identifier
- `--block-ids`: Comma-separated list of block IDs
- `--strategy`: Strategy (BalanceStages or ReduceCriticalPath)
- `--target-stage-count`: Target number of stages (optional)
- `--target-max-depth`: Target maximum depth (optional)
- `--max-extra-registers`: Maximum extra registers (optional)
- `--max-total-moves`: Maximum total moves (optional)

Example:
```bash
./proto-vm-cli global-pipeline-opt-subsystem --workspace ./workspace --session-id 1 --subsystem-id ALU_PIPE --block-ids ALU_STAGE1,ALU_STAGE2,ALU_FLAGS --strategy BalanceStages --target-max-depth 15
```

#### `global-pipeline-apply`
Apply a chosen global pipelining plan.

Parameters:
- `--workspace`: Workspace path
- `--session-id`: Session identifier
- `--branch`: Branch name
- `--plan-id`: Plan identifier to apply
- `--apply-only-safe`: Only apply safe moves (default true)
- `--allow-suspicious`: Allow suspicious moves (default false)
- `--max-moves`: Maximum moves to apply (optional)

Example:
```bash
./proto-vm-cli global-pipeline-apply --workspace ./workspace --session-id 1 --plan-id GPP_ALU_PIPE_BALANCE_1 --apply-only-safe true
```

### 5.2 Designer API Endpoints

#### `designer-global-pipeline`
Analyze global pipeline characteristics of a subsystem.

Payload:
```json
{
  "designer_session_id": "cd-1234",
  "target": "subsystem",
  "subsystem_id": "ALU_PIPE",
  "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"],
  "analyze_only": true
}
```

#### `designer-global-pipeline-opt`
Propose global pipelining optimization plans.

Payload:
```json
{
  "designer_session_id": "cd-1234",
  "target": "subsystem",
  "subsystem_id": "ALU_PIPE",
  "block_ids": ["ALU_STAGE1", "ALU_STAGE2", "ALU_FLAGS"],
  "strategy": "BalanceStages",
  "target_max_depth": 15,
  "apply": false
}
```

#### `designer-global-pipeline-apply`
Apply a chosen global pipelining plan.

Payload:
```json
{
  "designer_session_id": "cd-1234",
  "plan_id": "GPP_ALU_PIPE_BALANCE_1",
  "apply_only_safe": true,
  "allow_suspicious": false
}
```

## 6. Limitations and Future Work

### 6.1 Current Limitations
- Single-clock-domain per plan (no cross-clock-domain pipelining)
- Conservative approach to CDC safety (may be overly restrictive)
- Heuristic timing estimates may not be precise
- No automatic multi-plan scheduling (each plan is one-shot)

### 6.2 Future Enhancements
- Multi-domain global pipelining coordination
- More precise timing analysis using STA
- Adaptive plan selection based on circuit context
- Integration with high-level synthesis for pipeline generation
- Formal verification of global behavioral equivalence