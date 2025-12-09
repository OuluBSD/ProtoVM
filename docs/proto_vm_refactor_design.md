# ProtoVM Refactor Engine Design

## Overview

The ProtoVM Refactor Engine provides behavior-preserving transformations for digital circuits using semantic analysis. It leverages the existing analysis layers to identify and safely apply structural transformations that maintain high-level circuit behavior.

## Architecture

The refactor engine builds upon the existing ProtoVM analysis infrastructure:

- **CircuitGraph**: Graph representation of the circuit for pattern matching
- **BlockAnalysis**: Identification of circuit blocks for higher-level transformations
- **BehavioralAnalysis**: Verification that transformations preserve behavior
- **FunctionalAnalysis**: Analysis of functional dependency relations
- **TimingAnalysis**: Optional timing constraints checking

## Transformation Model

### Key Types

The transformation engine defines several key types:

```cpp
enum class TransformationKind {
    Unknown,
    SimplifyDoubleInversion,
    SimplifyRedundantGate,
    ReplaceWithKnownBlock,
    RewireFanoutTree,
    MergeEquivalentBlocks,
};

enum class PreservationLevel {
    BehaviorKindPreserved,     // High-level behavior kind remains the same
    IOContractPreserved,       // Ports and roles preserved
    DependencyPatternPreserved // Functional dependency structure preserved
};

struct TransformationTarget {
    String subject_id;          // e.g. block ID, component ID, or region identifier
    String subject_kind;        // "Block", "Component", "Region"
};

struct TransformationStep {
    String description;         // human-readable
};

struct TransformationPlan {
    String id;                              // unique transformation ID (per proposal)
    TransformationKind kind;
    TransformationTarget target;
    Vector<PreservationLevel> guarantees;   // what we assert is preserved
    Vector<TransformationStep> steps;       // high-level steps
};
```

### Transformation Engine

The `TransformationEngine` provides the core functionality:

- `ProposeTransformationsForBranch()`: Find transformation opportunities for an entire branch
- `ProposeTransformationsForBlock()`: Find transformation opportunities for a specific block
- `MaterializePlan()`: Convert a plan into low-level edit operations
- `VerifyBehaviorPreserved()`: Check if a transformation preserves behavior

## Implemented Transformation Patterns

### 1. SimplifyDoubleInversion

**Pattern**: `A → NOT → X → NOT → B`

**Transformation**: Rewires A directly to B, removes intermediate inverters

**Guarantees**: BehaviorKindPreserved, IOContractPreserved

### 2. SimplifyRedundantGate

**Pattern**: Idempotent gates like `X AND X = X` or `X OR X = X`

**Transformation**: Reduces redundant gate usage in simple patterns

**Guarantees**: BehaviorKindPreserved, IOContractPreserved

### 3. ReplaceWithKnownBlock

**Pattern**: Generic combinational blocks that match known patterns (adders, muxes, etc.)

**Transformation**: Reclassifies/reshapes underlying block structure to a canonical form

**Guarantees**: BehaviorKindPreserved, IOContractPreserved, DependencyPatternPreserved

## Behavior Preservation Strategy

The system uses a heuristic approach to verify behavioral preservation:

1. **Local Behavioral Check**: Compare BehaviorDescriptors before/after transformation
2. **Port/Role Preservation**: Verify that port signatures remain compatible
3. **Dependency Analysis**: Check that functional dependencies are preserved
4. **Timing Sanity Check**: Ensure timing constraints aren't violated (optional)

The engine errs on the side of safety, rejecting transformations that cannot be verified.

## CLI Commands

### `refactor-suggest`

Purpose: Propose transformations for the entire branch

Usage:
```
proto-vm-cli refactor-suggest --workspace <path> --session-id <id> --max-plans <n>
```

### `refactor-suggest-block`

Purpose: Propose transformations for a specific block

Usage:
```
proto-vm-cli refactor-suggest-block --workspace <path> --session-id <id> --block-id <id> --max-plans <n>
```

### `refactor-apply`

Purpose: Apply a chosen transformation plan

Usage:
```
proto-vm-cli refactor-apply --workspace <path> --session-id <id> --plan-id <id>
```

## JSON API

The daemon exposes the same commands via JSON over WebSockets:

- Command: `refactor-suggest`
- Command: `refactor-suggest-block` 
- Command: `refactor-apply`

## Integration with Session/Revision System

All transformations properly integrate with the existing session/branch/revision system:

- Each applied transformation creates new circuit revision
- Branch-aware operation support
- Event logging with proper metadata
- Conflict resolution through existing mechanisms

## Limitations

- Heuristic-based approach (not formal verification)
- Pattern-based matching limited to well-understood transformations
- Timing analysis currently optional and limited
- No global optimization meta-heuristics

## Future Extensions

- Additional transformation patterns
- SAT/SMT-based verification for critical transformations
- Global optimization strategies
- More sophisticated timing analysis integration