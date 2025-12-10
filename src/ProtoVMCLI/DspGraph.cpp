#include "DspGraph.h"
#include <cmath>  // For standard math functions

namespace ProtoVMCLI {

// This file contains the implementation of the DspGraph data structures.
// Currently, the structures are simple data containers with no complex logic,
// so the implementation file is mostly a placeholder for future functionality
// or potential helper methods.

// Example utility function that could be used with the DSP graph
double SineWave(double frequency_hz, double time_sec) {
    return std::sin(2.0 * M_PI * frequency_hz * time_sec);
}

double CosineWave(double frequency_hz, double time_sec) {
    return std::cos(2.0 * M_PI * frequency_hz * time_sec);
}

} // namespace ProtoVMCLI