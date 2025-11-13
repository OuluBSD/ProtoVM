# 1970s Analog Synthesizer Emulation

This example demonstrates a 1970s-style analog synthesizer with VCOs, ADSR envelopes, portamento, and multiple waveforms.

## Features

- Multiple oscillator waveforms: sine, sawtooth, square, triangle
- ADSR envelope generator for shaping note dynamics
- Portamento/gliding effect between notes
- Random note sequencer at 120 BPM
- Real-time audio output via PortAudio
- WAV file export capability

## Building

To build the project, you'll need CMake and PortAudio installed:

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install portaudio19-dev cmake

# Or on macOS with Homebrew
brew install portaudio cmake

# Build the project
cd analog_synth
mkdir build
cd build
cmake ..
make

# Run the synthesizer in real-time
./analog_synth

# Or generate a WAV file
./analog_synth --wav output.wav
```

## Usage

- For real-time audio: `./analog_synth`
- For WAV file output: `./analog_synth --wav filename.wav`
- For help: `./analog_synth --help`

The synthesizer will play a sequence of random notes at 120 BPM using a sawtooth waveform with ADSR envelope and portamento effects.

## Components

- Oscillator: Generates waveforms (sine, sawtooth, square, triangle)
- ADSR Envelope: Controls amplitude over time (Attack, Decay, Sustain, Release)
- Portamento: Glides between notes smoothly
- Sequencer: Generates random notes at specified BPM
- PortAudio Wrapper: Handles real-time audio output
- WAV Writer: Outputs audio to WAV files