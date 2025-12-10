# ProtoVM DSP Graph and Runtime Design

## Overview

The DSP Graph and Runtime system provides an executable digital signal processing (DSP) engine for audio-oriented circuits in ProtoVM. This system enables offline audio rendering and future real-time extensions by creating an intermediate representation (DspGraph) of audio processing components and a runtime engine (DspRuntime) to execute the signal flow.

## DspGraph IR (Intermediate Representation)

The DspGraph IR provides a representation of audio processing nodes, their connections, and signal flow topology.

### Core Concepts

- **DspNode**: Represents an audio processing unit (oscillator, panner, etc.)
- **DspConnection**: Represents signal flow between nodes
- **DspGraph**: The complete graph representation with all nodes and connections

### Node Kinds

- **Oscillator**: Audio-rate oscillator producing time-varying signals (e.g., sine waves)
- **PanLfo**: Low-frequency oscillator for panning or modulation (0.1-10Hz range)
- **StereoPanner**: Maps mono signals to stereo L/R outputs with pan control
- **OutputSink**: Final output node writing samples to buffers

### Port Types

- **Audio**: Per-sample audio-rate signals (e.g., oscillator output)
- **Control**: Scalar control-rate signals (e.g., LFO for panning)

### Example: 440 Hz Oscillator with Pan LFO

The system can generate a graph with:
- Oscillator node (440 Hz sine wave)
- Pan LFO node (0.25 Hz control signal)
- Stereo panner node (mixes mono input with pan control to stereo output)
- Output sink (collects stereo output)

## DspRuntime Execution Engine

The DspRuntime provides a pull-style execution engine that renders audio offline. It processes the graph in topological order, ensuring all dependencies are satisfied before processing a node.

### Execution Model

- **Offline rendering**: Processes all samples in advance rather than real-time
- **Sample-by-sample**: Processes each sample timestamp sequentially
- **Topological ordering**: Ensures dependencies are resolved in correct order

### Runtime Flow

1. **Initialization**: Set up all node states and output buffers
2. **Per-sample processing**: For each sample index
   - Evaluate nodes in dependency order:
     - Oscillator: Compute next sample (e.g., sin(2π * freq * time))
     - PanLfo: Compute next control value (e.g., 0.5*(1+sin(2π * rate * time)))
     - Panner: Apply panning to audio input based on control value
     - Output: Write samples to output buffers
3. **Output**: Return filled stereo buffers

## AudioDslGraph to DspGraph Mapping

The system supports conversion from AudioDslGraph (high-level audio specification) to DspGraph (executable representation):

- **AudioDslOscillator** → **Oscillator node**
- **AudioDslPanLfo** → **PanLfo node** 
- Custom panner and output nodes added automatically

## API Integration

### CLI Commands

- `dsp-graph-inspect`: Inspect the constructed DSP graph for an oscillator block
- `dsp-render-osc`: Render 3 seconds of stereo audio for a 440Hz oscillator with 0.25Hz pan LFO

### CoDesigner Endpoints

- `designer-dsp-graph-inspect`: Inspect DSP graph via CoDesigner API
- `designer-dsp-render-osc`: Render audio via CoDesigner API

## Implementation Details

### DspGraphBuilder

Converts AudioDslGraph to DspGraph by creating appropriate nodes and connecting them according to the audio processing flow.

### DspRuntime Implementation

Uses a simple phase accumulation model for oscillators and LFOs to ensure frequency accuracy over long renderings.

### CircuitFacade Integration

Provides high-level APIs to build and render DSP graphs for specific blocks in branches, abstracting the underlying implementation details.

## Example Usage

For a 440Hz sine wave oscillator with 0.25Hz pan LFO at 48kHz sample rate for 3 seconds:

1. Create AudioDslGraph with oscillator frequency=440, LFO rate=0.25
2. Build DspGraph using DspGraphBuilder
3. Initialize DspRuntime with graph
4. Render 48000*3 = 144000 stereo samples
5. Output will show sine wave smoothly panning from left to right and back over 4-second cycles