# ProtoVM Musical Intent System Design

## Overview

The Musical Intent System is a Phase 30 component of ProtoVM that translates high-level musical language into verifiable parameter adjustments for digital signal processing and instrument synthesis. This system bridges the gap between human-friendly audio descriptors and the low-level parameters of digital audio systems.

## Intent Vocabulary

The system defines a deterministic vocabulary of musical intents that can be mapped to specific audio quality metrics and parameter adjustments.

### Available Intents

1. **Warmer**: Reduce high frequencies and add gentle saturation to create a "warmer" sound character
   - Mapping: Decreases HarmonicEnergy in high bands, reduces PeakLevel spikes
   - Actions: Suggests high-frequency roll-off filters or gentle saturation

2. **Brighter**: Increase high frequencies and reduce low-mid content
   - Mapping: Increases HarmonicEnergy in high frequency bands
   - Actions: Suggests high-frequency boost filters

3. **Wider**: Increase stereo image width
   - Mapping: Increases StereoWidth metric while maintaining StereoBalance
   - Actions: Adjusts stereo width parameters to 0.7-1.0 range

4. **Narrower**: Reduce stereo image width
   - Mapping: Decreases StereoWidth metric
   - Actions: Adjusts stereo width parameters to 0.1-0.7 range

5. **MoreMovement**: Increase modulation depth or rate for dynamic movement
   - Mapping: Increases time-variance of StereoBalance and width within limits
   - Actions: Increases pan LFO rate (0.5-2.5Hz) and depth (0.3-1.0)

6. **LessMovement**: Decrease modulation depth or rate
   - Mapping: Decreases time-variance of StereoBalance and width
   - Actions: Decreases pan LFO rate (0.5-0.1Hz) and depth (0.5-0.1)

7. **Softer**: Reduce harsh transients and peakiness
   - Mapping: Reduces PeakLevel metric
   - Actions: Adds soft clipping, reduces output gain up to 20%

8. **MorePunch**: Increase transient response and attack characteristics
   - Mapping: Increases PeakLevel metric within safe bounds
   - Actions: Adds compressor with fast attack/release, enhances attack

9. **Cleaner**: Reduce harmonic distortion, DC offset, and noise
   - Mapping: Reduces DCOffset and HarmonicEnergy metrics
   - Actions: Applies low-distortion parameters, DC offset correction

10. **Dirtier**: Add mild saturation or noise for character
    - Mapping: Increases HarmonicEnergy within bounded amounts
    - Actions: Adds mild saturation (up to 0.5) and subtle analog noise (up to 0.3)

## System Architecture

### Core Components

1. **MusicalIntentEngine**: Core engine responsible for mapping intents to actions and verifying changes
2. **IntentRequest**: Contains the musical intents, strength parameter, and safety policies
3. **IntentAction**: Concrete parameter adjustments that implement the intents
4. **IntentPlan**: Complete plan with before/after verification including QA metrics

### Intent Request Structure

```cpp
struct IntentRequest {
    Vector<IntentKind> intents;       // List of intents to apply
    double strength = 0.5;            // 0..1 strength of the change
    double duration_sec = 3.0;        // Duration for audio analysis
    int sample_rate = 48000;          // Sample rate for processing
    int block_size = 512;             // Block size for processing

    // Safety policies
    bool allow_regression = false;    // Allow quality regressions
    String qa_profile_name;           // Optional QA threshold profile
};
```

### Intent Action Kinds

1. **SetInstrumentParam**: Modify existing instrument parameters
2. **AddModulator**: Add LFO or modulation routing
3. **AdjustMix**: Change gain, pan, or width parameters
4. **SuggestRefactorPlaybook**: Recommend applying specific refactor playbooks

## Safety and Verification

### Quality Assurance Integration

The system integrates with Phase 29's Audio QA system to ensure all changes meet quality standards:

1. **Before/After Analysis**: Renders audio before and after applying changes
2. **Metric Comparison**: Compares relevant audio quality metrics
3. **Threshold Validation**: Validates against QA threshold profiles
4. **Regression Detection**: Identifies any quality regressions

### Default Threshold Profile

```
RMSLevel: -30.0 to -5.0 dB
PeakLevel: -12.0 to -0.1 dB (avoiding clipping)
DCOffset: -0.01 to 0.01 (very low offset)
StereoBalance: -0.9 to 0.9 (not too imbalanced)
FundamentalFrequency: 100.0 to 5000.0 Hz (valid range)
```

## Command Interface

### CLI Commands

1. **intent-propose**: Generate a proposal without applying changes
   - `--instrument-path`: Path to the instrument file
   - `--intent`: Comma-separated list of intents (e.g., "warmer,wider")
   - `--strength`: 0-1 strength of the effect
   - `--duration-sec`: Duration for analysis
   - `--allow-regression`: Whether to allow quality regressions
   - `--qa-profile`: Named QA threshold profile

2. **intent-verify**: Generate and verify a proposal with full audio analysis
   - Same parameters as intent-propose
   - Performs complete before/after analysis and verification

### Daemon Endpoints

1. **designer-intent-propose**: Propose intent changes in designer context
2. **designer-intent-verify**: Verify intent changes in designer context

## Usage Examples

### CLI Example

```bash
# Propose making an instrument warmer and wider
proto_vm_cli intent-propose \
  --instrument-path my_synth.json \
  --intent warmer,wider \
  --strength 0.7 \
  --duration-sec 5.0

# Verify and apply changes with QA validation
proto_vm_cli intent-verify \
  --session-id 12345 \
  --branch-name main \
  --instrument-path my_synth.json \
  --intent warmer,wider,more-movement \
  --strength 0.6 \
  --allow-regression false
```

### Daemon Example

```json
{
  "id": "req-123",
  "command": "designer-intent-verify",
  "payload": {
    "designer_session_id": "cd-abc123",
    "target": "block",
    "block_id": "oscillator_01",
    "intent": "warmer,softer",
    "strength": 0.5,
    "duration_sec": 3.0,
    "allow_regression": false,
    "qa_profile": "default"
  }
}
```

## Deterministic Behavior

The system ensures deterministic behavior by:

1. Using fixed algorithmic mappings from intents to metrics
2. Applying consistent strength scaling across all intents
3. Following the same verification process for all plans
4. Maintaining consistent QA threshold profiles

## Extensibility

The system is designed for future expansion:

1. New intents can be added by extending the IntentKind enum
2. Additional QA metrics can be incorporated into intent mappings
3. New action types can be added to support more complex transformations
4. QA profiles can be customized for different audio applications

## Limitations

1. Current implementation suggests refactoring when advanced DSP features aren't available
2. No machine learning model training (planned for Phase 31+)
3. No automatic "best of N search" (planned for Phase 31+)
4. Limited to instrument-level controls (no plugin-binary rewriting)

## Future Enhancements

1. **Phase 31+**: Machine learning-based intent interpretation
2. **Phase 31+**: Automatic optimization of intent parameters
3. **Phase 31+**: Psychoacoustic model integration
4. **Phase 31+**: Multi-instrument orchestration intents