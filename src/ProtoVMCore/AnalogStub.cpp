// Minimal AnalogSimulation implementation for the headless digital core.
//
// The full analog engine lives in the ProtoVM application package, which the
// headless core deliberately does not depend on (it drags in the CLI, AudioQa,
// FFTW and jsoncpp). Machine only calls these three members, so a no-op analog
// backend keeps the digital simulator self-contained and linkable.
#include "../ProtoVM/AnalogSimulation.h"

AnalogSimulation::AnalogSimulation()
	: time_step(1e-6), max_iterations(100), tolerance(1e-6) {}

void AnalogSimulation::RegisterAnalogComponent(AnalogNodeBase* component) {}

bool AnalogSimulation::Tick() { return true; }
