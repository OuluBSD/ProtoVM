#ifndef _ProtoVM_TransmissionLine_h_
#define _ProtoVM_TransmissionLine_h_

#include "AnalogCommon.h"

// Transmission Line component with characteristic impedance and propagation delay
class TransmissionLine : public AnalogNodeBase {
public:
    typedef TransmissionLine CLASSNAME;

    // Constructor: Z0 is characteristic impedance in Ohms, delay_time is propagation delay in seconds
    TransmissionLine(double characteristic_impedance = 50.0, double delay_time = 1e-9);  // Default 50Ω, 1ns delay
    virtual ~TransmissionLine() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TransmissionLine"; }

    void SetCharacteristicImpedance(double z0);
    double GetCharacteristicImpedance() const { return characteristic_impedance; }
    
    void SetDelayTime(double delay_time);
    double GetDelayTime() const { return delay_time; }

private:
    double characteristic_impedance;  // Characteristic impedance in Ohms
    double delay_time;                // Propagation delay in seconds
    double length;                    // Physical length (optional, derived from delay and velocity factor)
    
    // For simulation, we'll model the delay as a simple delay line with memory
    static const int MAX_DELAY_SAMPLES = 100;  // Maximum number of samples to store for delay
    std::vector<double> voltage_delay_buffer_a;  // Delay buffer for terminal A
    std::vector<double> voltage_delay_buffer_b;  // Delay buffer for terminal B
    int delay_samples;                           // Number of samples corresponding to delay_time
    int current_sample_index;                    // Current position in the delay buffer

    static constexpr double MIN_IMPEDANCE = 0.1;        // 0.1Ω minimum
    static constexpr double MIN_DELAY = 1e-12;          // 1ps minimum delay
    static constexpr double SPEED_OF_LIGHT = 299792458.0; // m/s
    static constexpr double DEFAULT_VELOCITY_FACTOR = 0.66; // Velocity factor (fraction of c)
};

#endif