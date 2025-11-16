#ifndef SYNTH_ARCH_ADAPTER_H
#define SYNTH_ARCH_ADAPTER_H

#include "SynthArchitectures.h"
#include <memory>

// A wrapper/adapter class that provides the same interface as Synthesizer for audio systems
class SynthArchAdapter {
public:
    SynthArchAdapter(std::unique_ptr<SynthArchitecture> arch) 
        : synthArch(std::move(arch)) {}
    
    double getNextSample() {
        if (synthArch) {
            return synthArch->getNextSample();
        }
        return 0.0;
    }
    
    void noteOn(double frequency, double velocity = 1.0) {
        if (synthArch) {
            synthArch->noteOn(frequency, velocity);
        }
    }
    
    void noteOff() {
        if (synthArch) {
            synthArch->noteOff();
        }
    }
    
    void setSampleRate(int rate) {
        if (synthArch) {
            synthArch->setSampleRate(rate);
        }
    }
    
    SynthArchitecture* getArchitecture() { return synthArch.get(); }

private:
    std::unique_ptr<SynthArchitecture> synthArch;
};

#endif // SYNTH_ARCH_ADAPTER_H