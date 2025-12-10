# ProtoVM Structural Synthesis & Canonicalization Design

## 1. Motivation & Goals

The structural synthesis layer provides a robust framework for analyzing and refactoring digital circuits at the gate-level structure while preserving behavioral semantics. This layer enables ProtoVM to:

- **Detect redundant logic** and simplify it (e.g., `X & X → X`, shared subexpressions)
- **Normalize structures** into canonical patterns (e.g., standard mux/add/sub/compare forms)
- **Factor common logic** across cones where safe
- **Propose structural refactor plans** that can be applied using the existing transformation engine

The layer is designed to be:
- **Local and conservative** (operate on cones where behavior is easy to preserve)
- **Guided by IR and functional analysis**, not purely syntactic
- **Safe w.r.t. CDC, retiming topology, and block boundaries**

## 2. Data Structures

### 2.1 Structural Pattern Kinds

The `StructuralPatternKind` enum class defines the types of structural patterns that can be detected:

- `RedundantLogic`: Duplicated cones, idempotent operations like `X & X`, `X | X`, etc.
- `CommonSubexpression`: Shared logic reused across multiple outputs
- `CanonicalMux`: Mux trees that can be normalized to standard forms
- `CanonicalAdder`: Adder-like structures that can be normalized to standard forms
- `CanonicalComparator`: Comparator-like structures
- `ConstantPropagation`: Cones dominated by constants that can be propagated
- `DeadLogic`: Logic that does not influence observable outputs

### 2.2 Structural Pattern

Represents a detected structural pattern in the circuit:

```cpp
struct StructuralPattern {
    String pattern_id;              // Unique identifier for this pattern
    StructuralPatternKind kind;     // Type of pattern detected
    Vector<String> node_ids;        // Nodes/components involved in the pattern
    String description;             // Explanatory description of the pattern
};
```

### 2.3 Refactor Safety Classification

The `StructuralRefactorSafety` enum class classifies the safety of proposed refactoring moves:

- `Safe`: Expected behavior preserved under current heuristics
- `Suspicious`: Might be safe, but needs human/AI review
- `Forbidden`: Detected, but should not be auto-applied

### 2.4 Structural Refactor Move

Describes a proposed structural transformation:

```cpp
struct StructuralRefactorMove {
    String move_id;                             // Unique identifier for this move
    String target_block_id;                     // Block in which refactor occurs
    StructuralPatternKind kind;                 // Type of refactor
    Vector<String> affected_node_ids;          // Nodes affected by this move
    StructuralRefactorSafety safety;           // Safety classification
    String safety_reason;                      // Explanation for safety assessment
    String transform_hint;                     // High-level description of intended transformation
};
```

### 2.5 Structural Refactor Plan

Bundles multiple refactor moves into a comprehensive plan:

```cpp
struct StructuralRefactorPlan {
    String id;                                  // Plan identifier
    String target_block_id;                     // Target block for the refactor
    Vector<StructuralPattern> patterns;         // Detected patterns
    Vector<StructuralRefactorMove> moves;       // Proposed refactor moves
    int gate_count_before = -1;                // Gate count before (estimated)
    int gate_count_after_estimate = -1;        // Gate count after (estimated)
    int depth_before = -1;                     // Combinational depth before (estimated)
    int depth_after_estimate = -1;             // Combinational depth after (estimated)
    bool respects_cdc_fences = true;           // Whether CDC constraints are respected
};
```

## 3. Analysis Engine

### 3.1 Structural Analysis

The `StructuralAnalysis` class provides static methods for pattern detection:

```cpp
class StructuralAnalysis {
public:
    // Analyze a block for structural simplification patterns
    static Result<StructuralRefactorPlan> AnalyzeBlockStructure(
        const String& block_id,
        const CircuitGraph& graph,
        const FunctionalAnalysis* functional = nullptr,  // optional
        const HlsIrModule* ir_module = nullptr,          // optional: IR view of the block
        const CdcReport* cdc_report = nullptr            // optional: do not cross CDC fences
    );
};
```

### 3.2 Pattern Detection Implementation

The analysis uses multiple input sources:
1. **CircuitGraph** to identify combinational cones between registers and nodes with identical truth-functions
2. **FunctionalAnalysis** and **HlsIr** to detect algebraic redundancies and recognize canonical patterns
3. **CdcReport** to ensure no CDC constraints are violated during refactoring

## 4. Transformation Bridge

### 4.1 Structural Transform

The `StructuralTransform` class bridges structural refactor plans to the transformation system:

```cpp
class StructuralTransform {
public:
    // Build a TransformationPlan from a structural refactor plan (without applying)
    static Result<TransformationPlan> BuildTransformationPlanForStructuralRefactor(
        const StructuralRefactorPlan& plan,
        bool apply_only_safe_moves = true
    );

    // Apply a StructuralRefactorPlan directly in a branch (build + apply)
    static Result<RetimingApplicationResult> ApplyStructuralRefactorInBranch(
        const StructuralRefactorPlan& plan,
        bool apply_only_safe_moves,
        SessionStore& session_store,
        const SessionMetadata& session,
        const String& session_dir,
        const String& branch_name
    );
};
```

## 5. CircuitFacade Integration

### 5.1 Structural Synthesis API Methods

The `CircuitFacade` provides branch-aware methods for structural synthesis:

```cpp
// Analyze a block for structural refactor opportunities in a specific branch
Result<StructuralRefactorPlan> AnalyzeBlockStructureInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const std::string& block_id
);

// Apply a structural refactor plan in a specific branch
Result<RetimingApplicationResult> ApplyStructuralRefactorPlanInBranch(
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name,
    const StructuralRefactorPlan& plan,
    bool apply_only_safe_moves
);
```

## 6. CLI Commands

### 6.1 struct-analyze-block

Analyzes a block for structural refactor opportunities:

```
proto-vm-cli struct-analyze-block --workspace PATH --session-id ID --block-id BLOCK_ID [--branch BRANCH_NAME]
```

**Output:**
```json
{
  "ok": true,
  "command": "struct-analyze-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "ALU_CORE",
    "structural_refactor_plan": { ... }
  }
}
```

### 6.2 struct-apply-block

Applies a structural refactor plan to a block:

```
proto-vm-cli struct-apply-block --workspace PATH --session-id ID --plan-id PLAN_ID --block-id BLOCK_ID [--branch BRANCH_NAME] [--safe-only true]
```

**Output:**
```json
{
  "ok": true,
  "command": "struct-apply-block",
  "error_code": null,
  "error": null,
  "data": {
    "session_id": 1,
    "branch": "main",
    "block_id": "ALU_CORE",
    "application_result": { ... }
  }
}
```

## 7. CoDesigner Integration

### 7.1 designer-struct-analyze

Analyzes structural refactor opportunities for a focused block:

**Request:**
```json
{
  "command": "designer-struct-analyze",
  "payload": {
    "designer_session_id": "cd-1234",
    "target": "block",
    "block_id": "ALU_CORE"
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "designer-struct-analyze",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "structural_refactor_plan": { ... }
  }
}
```

### 7.2 designer-struct-apply

Applies a structural refactor plan in the designer session context:

**Request:**
```json
{
  "command": "designer-struct-apply",
  "payload": {
    "designer_session_id": "cd-1234",
    "plan_id": "SRP_ALU_CORE_1",
    "apply_only_safe": true
  }
}
```

**Response:**
```json
{
  "ok": true,
  "command": "designer-struct-apply",
  "error_code": null,
  "error": null,
  "data": {
    "designer_session": { ... },
    "application_result": { ... }
  }
}
```

## 8. JSON Serialization

All new structural synthesis data structures are fully serializable to JSON via the `JsonIO` class methods:

- `StructuralPatternKindToJson()` - Converts enum to string
- `StructuralPatternToValueMap()` - Serializes pattern objects
- `StructuralPatternsToValueArray()` - Serializes pattern arrays
- `StructuralRefactorSafetyToJson()` - Converts safety enum to string
- `StructuralRefactorMoveToValueMap()` - Serializes refactor move objects
- `StructuralRefactorMovesToValueArray()` - Serializes refactor move arrays
- `StructuralRefactorPlanToValueMap()` - Serializes plan objects

## 9. Safety & Verification

The structural synthesis layer implements several safety measures:
1. **CDC Compliance**: Ensures refactoring moves do not cross clock domain boundaries
2. **Behavior Preservation**: Uses heuristics to verify that transformations preserve expected behavior
3. **Safety Classification**: All refactor moves are classified with safety levels
4. **Option to Apply Safe Moves Only**: Applications can choose to apply only safe moves

## 10. Relationship to Future Code Generation

The structural synthesis layer is designed to prepare the ground for future code generation phases:
1. **Normalized structures** correspond cleanly to HLS-IR expressions (adders, muxes, comparators, small ALUs)
2. **Canonical forms** make it easy to map to C/C++/assembly snippets
3. **Structured refactoring** creates well-formed blocks suitable for audio/DSP mappings

## 11. Limitations

1. **Heuristic-based**: No full formal equivalence checking; relies on existing behavioral and IR layers
2. **Local optimizations**: Focuses on cone-level optimizations rather than global optimization
3. **No complex transformations**: Limited to well-understood refactoring patterns for safety

## 12. Performance Considerations

1. **Cache Results**: Structural analysis results can be cached per block and revision
2. **Incremental Analysis**: Only re-analyze blocks that have changed
3. **Bounded Complexity**: Pattern detection algorithms are designed with bounded complexity for practical use