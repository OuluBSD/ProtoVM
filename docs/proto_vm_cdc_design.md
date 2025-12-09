# ProtoVM CDC (Clock-Domain Crossing) Analysis Design

## Overview

The CDC (Clock-Domain Crossing) safety analysis layer in ProtoVM provides a robust framework for identifying and classifying potential hazards when signals cross between different clock domains in a digital circuit. This analysis is crucial for preventing metastability and ensuring reliable operation of circuits with multiple clock domains.

## Core Concepts

### CDC Challenges

Clock-domain crossing presents several challenges in digital design:
- **Metastability**: When signals cross clock domains without proper synchronization, flip-flops can enter an unstable state
- **Data incoherence**: Multi-bit signals may experience different arrival times across clock domains
- **Timing violations**: Setup and hold time requirements may be violated across different clock domains

### CDC Analysis Goals

- Identify all register-to-register paths that cross clock domains
- Classify crossings according to safety patterns
- Provide actionable warnings and guidance for unsafe crossings
- Prepare for future automatic synchronizer insertion

## Data Structures

### CdcCrossingKind

The type of clock-domain crossing pattern:

- `SingleBitSyncCandidate`: A 1-bit signal that appears suitable for a 2-flop synchronizer
- `MultiBitBundle`: A multi-bit bus or group crossing requiring special handling (Gray code, handshake, etc.)
- `HandshakeLike`: Patterns resembling ready/valid or request/ack style signals
- `UnknownPattern`: Crossings that do not match known safe patterns

### CdcSeverity

The severity level of a CDC issue:

- `Info`: General information about a crossing
- `Warning`: Potential issue requiring designer review
- `Error`: High-confidence hazard requiring immediate attention

### CdcCrossingEndpoint

Represents a source or destination endpoint of a CDC crossing:

- `reg_id`: Register identifier from PipelineMap::RegisterInfo::reg_id
- `clock_signal`: Clock signal name associated with the endpoint
- `domain_id`: Clock domain identifier from PipelineMap

### CdcCrossing

Represents an individual clock-domain crossing:

- `id`: Unique identifier for the crossing (e.g., "CDCC_0001")
- `src`: Source endpoint with clock domain information
- `dst`: Destination endpoint with clock domain information
- `kind`: The classification of the crossing pattern
- `is_single_bit`: Boolean indicating if this is a single-bit signal
- `bit_width`: Number of bits in the crossing (-1 if unknown)
- `crosses_reset_boundary`: Boolean indicating if crossing also crosses reset domains

### CdcIssue

Represents a specific CDC-related issue:

- `id`: Unique identifier for the issue
- `severity`: The severity level (Info, Warning, Error)
- `summary`: Brief human-readable description
- `detail`: Longer explanation and remediation hints
- `crossing_id`: Reference to the associated crossing

### CdcReport

The top-level report containing all CDC analysis results:

- `id`: Block or subsystem identifier
- `clock_domains`: Reference list of clock domains from PipelineMap
- `crossings`: All identified crossings between registers
- `issues`: All issues identified during analysis

## Analysis Implementation

### Crossing Discovery

The CDC analysis uses the existing `PipelineMap` and `CircuitGraph` to identify register-to-register paths that cross clock domains:

1. For every `RegToRegPathInfo` in `pipeline.reg_paths`:
   - If `crosses_clock_domain` is true, treat as a CDC candidate
   - Construct `CdcCrossing` entries with source and destination endpoints

2. The analysis focuses on register-to-register paths initially, but could be extended to include primary inputs and other crossing points.

### Classification Heuristics

The system uses several heuristics to classify crossings:

#### SingleBitSyncCandidate
- Appears to be a 1-bit control signal (bit_width == 1)
- No obvious bundling with related signals
- Generally safe with proper 2-flop synchronizer

#### MultiBitBundle
- Appears as a bus or multiple related registers crossing together
- Requires special handling like Gray encoding or handshake protocols
- Risky without proper synchronization

#### HandshakeLike
- Detects pairs like `*_valid` and `*_ready`, or `*_req` and `*_ack`
- Crossing in opposite directions between domains
- Can be safe if protocol is correctly designed for CDC

#### UnknownPattern
- Fallback category for anything that doesn't match the above
- Requires careful manual review

### Severity Mapping

The system maps crossing classifications to severity levels:

- `SingleBitSyncCandidate` → `Warning` (safe if proper synchronizer exists)
- `MultiBitBundle` → `Error` (multi-bit CDC without verification is risky)
- `HandshakeLike` → `Info` or `Warning` (depends on design confidence)
- `UnknownPattern` → `Warning` (requires designer review)

### Remediation Hints

The system provides general guidance for common crossing types:

- Multi-bit: "Consider using async FIFO or Gray code encoding"
- Handshake: "Verify handshake protocol is designed for clock domain crossing"
- General: "Review crossing for proper synchronizer implementation"

## Integration with Existing Systems

### PipelineMap Integration

The CDC analysis builds upon the existing `PipelineMap` structure from Phase 19, using:
- Clock domain information from `PipelineMap::clock_domains`
- Register path information from `PipelineMap::reg_paths`
- Cross-domain path flags (`crosses_clock_domain`)

### CircuitGraph Integration

The analysis uses the `CircuitGraph` to understand signal relationships and potentially infer bit widths for better classification.

### TimingAnalysis Integration

The optional `TimingAnalysis` parameter could be used in the future for more detailed timing-based CDC analysis.

## API and Commands

### CircuitFacade Integration

The `CircuitFacade` provides two new methods:
- `BuildCdcReportForBlockInBranch()`: Analyze CDC issues for a specific block
- `BuildCdcReportForSubsystemInBranch()`: Analyze CDC issues for a subsystem across multiple blocks

### CLI Commands

Two new commands are available:

#### `cdc-block`
Analyzes CDC issues for a single block:
```
proto-vm-cli cdc-block --workspace <path> --session-id <id> --block-id <block_id> --branch <name>
```

#### `cdc-subsystem`
Analyzes CDC issues across a subsystem of multiple blocks:
```
proto-vm-cli cdc-subsystem --workspace <path> --session-id <id> --subsystem-id <subsystem_id> --block-ids <id1,id2,id3> --branch <name>
```

## Limitations

- Analysis is heuristic and best-effort, not formal verification
- Does not verify the presence of actual synchronizers in the design
- No deep protocol verification for handshakes
- Focuses on register-to-register paths initially
- Classification is based on static analysis and naming patterns

## Future Enhancements

- Enhanced bit-width inference for more accurate multi-bit detection
- Protocol verification for handshake-like crossings
- Integration with timing analysis for setup/hold time checks
- Automatic synchronizer insertion in future phases
- Support for more complex CDC structures like async FIFOs

## Security and Safety

The CDC analysis layer is read-only and does not modify the circuit structure. It only reports potential issues to guide designers in fixing CDC hazards manually. This approach ensures that the analysis cannot introduce new hazards while providing valuable guidance for safe clock-domain crossing design.