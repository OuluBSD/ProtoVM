#include "ProtoVM.h"
#include "VCO.h"
#include "VCF.h"
#include "VCA.h"
#include "LFO.h"
#include "ADSR.h"
#include "TubeFilter.h"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "Testing Analog Synthesizer Components..." << std::endl;

    // Test VCO
    std::cout << "\nTesting VCO (Voltage Controlled Oscillator)..." << std::endl;
    VCO vco(VCOType::SAWTOOTH, 440.0);  // A4 note
    vco.SetControlVoltage(1.0);
    vco.SetAmplitude(0.5);
    
    for (int i = 0; i < 5; i++) {
        vco.Tick();
        std::cout << "VCO Output " << i << ": " << vco.GetOutput() << std::endl;
    }

    // Test LFO
    std::cout << "\nTesting LFO (Low Frequency Oscillator)..." << std::endl;
    LFO lfo(LFOType::SINE, 2.0);  // 2 Hz modulation
    
    for (int i = 0; i < 5; i++) {
        lfo.Tick();
        std::cout << "LFO Output " << i << ": " << lfo.GetOutput() << std::endl;
    }

    // Test ADSR Envelope
    std::cout << "\nTesting ADSR Envelope..." << std::endl;
    ADSR adsr(0.1, 0.2, 0.7, 0.3);  // Standard envelope times
    std::cout << "ADSR Initial State: " << (adsr.IsActive() ? "Active" : "Inactive") << std::endl;
    
    adsr.NoteOn();
    for (int i = 0; i < 3; i++) {
        adsr.Tick();
        std::cout << "ADSR Output (Note On) " << i << ": " << adsr.GetOutput() << std::endl;
    }
    
    adsr.NoteOff();
    for (int i = 0; i < 3; i++) {
        adsr.Tick();
        std::cout << "ADSR Output (Note Off) " << i << ": " << adsr.GetOutput() << std::endl;
    }

    // Test VCA
    std::cout << "\nTesting VCA (Voltage Controlled Amplifier)..." << std::endl;
    VCA vca(VCACharacteristic::EXPONENTIAL, 1.0);
    vca.SetInput(0.5);  // Input signal
    vca.SetControlVoltage(2.0);  // Control voltage
    
    for (int i = 0; i < 5; i++) {
        vca.Tick();
        std::cout << "VCA Output " << i << ": " << vca.GetOutput() << std::endl;
    }

    // Test VCF
    std::cout << "\nTesting VCF (Voltage Controlled Filter)..." << std::endl;
    VCF vcf(FilterType::LOWPASS, FilterImplementation::MOOG_LADDER, 1000.0, 0.5);
    vcf.SetInput(0.5);  // Input signal
    vcf.SetControlVoltage(1.0);  // Control voltage
    
    for (int i = 0; i < 5; i++) {
        vcf.Tick();
        std::cout << "VCF Output " << i << ": " << vcf.GetOutput() << std::endl;
    }

    // Test Tube Filter
    std::cout << "\nTesting Tube Filter..." << std::endl;
    TubeFilter tube_filter(TubeFilterType::LOWPASS, 500.0);
    tube_filter.SetInput(0.3);  // Input signal
    
    for (int i = 0; i < 5; i++) {
        tube_filter.Tick();
        std::cout << "Tube Filter Output " << i << ": " << tube_filter.GetOutput() << std::endl;
    }

    std::cout << "\nAll analog synthesizer components tested successfully!" << std::endl;
    return 0;
}