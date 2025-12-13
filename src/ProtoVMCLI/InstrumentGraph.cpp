#include "InstrumentGraph.h"
#include <cmath>  // For standard math functions

namespace ProtoVMCLI {

// This file contains the implementation of the InstrumentGraph data structures.
// Currently, the structures are simple data containers with no complex logic,
// so the implementation file is mostly a placeholder for future functionality
// or potential helper methods.

// Helper function to convert cents to frequency multiplier
double CentsToFreqMultiplier(double cents) {
    return std::pow(2.0, cents / 1200.0);
}

// Helper function to calculate frequency with detune in cents
double ApplyDetune(double base_freq_hz, double detune_cents) {
    return base_freq_hz * CentsToFreqMultiplier(detune_cents);
}

} // namespace ProtoVMCLI