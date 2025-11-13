#ifndef _ProtoVM_AnalogCommon_h_
#define _ProtoVM_AnalogCommon_h_

#include <vector>
#include <cmath>

// AnalogNodeBase extends ElectricNodeBase to handle continuous analog values
class AnalogNodeBase : public ElectricNodeBase {
public:
    typedef AnalogNodeBase CLASSNAME;
    
    AnalogNodeBase();
    virtual ~AnalogNodeBase() {}

    // Analog value at each connector
    std::vector<double> analog_values;
    
    // Override Tick to handle analog behavior
    virtual bool Tick() override;
    
    // Process analog signal
    virtual bool ProcessAnalog(double input_voltage, int pin_id);
    
    // Set the analog voltage at a specific pin
    void SetAnalogValue(int pin_id, double voltage);
    
    // Get the analog voltage at a specific pin
    double GetAnalogValue(int pin_id) const;
    
    // Update analog voltage at a specific pin by reference for efficiency
    void UpdateAnalogValue(int pin_id, double voltage);
    
    // Calculate time constant for RC circuits
    static double CalculateRCConstant(double resistance, double capacitance);
    
    // Calculate voltage across a capacitor at time t
    static double RCResponse(double initial_voltage, double target_voltage, 
                            double time_constant, double time_elapsed);

protected:
    // Simulation time step (in seconds)
    static constexpr double SIMULATION_TIMESTEP = 1.0 / 44100.0;  // Match audio sample rate

    // Time in seconds since start of simulation
    double simulation_time = 0.0;
    
    // Function to update internal state based on analog inputs
    virtual void UpdateState();
    
    // Function to compute outputs based on internal state
    virtual void ComputeOutputs();
};

#endif