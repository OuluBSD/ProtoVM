#ifndef TUBE_REVERB_CIRCUITS_H
#define TUBE_REVERB_CIRCUITS_H

#include "ElectricNodeBase.h"
#include <vector>
#include <memory>

// Class to model spring reverb circuits
class SpringReverb : public ElectricNodeBase {
public:
    enum SpringType {
        ACCUTRON_2A,      // Fender's vintage spring
        ACCUTRON_3A,      // Another Fender variant
        SPRAGALL_4AB2A,   // Gibson/Epiphone vintage
        MODERN_SPRING     // Modern reverb tank
    };
    
    SpringReverb(SpringType type = ACCUTRON_2A);
    virtual ~SpringReverb() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure reverb parameters
    void setReverbTime(double time);  // in seconds
    void setDamping(double damp);     // 0.0 to 1.0
    void setMix(double mix);          // 0.0 (dry) to 1.0 (wet)
    void setPreDelay(double delay);   // in seconds
    void setInputGain(double gain) { inputGain = gain; }
    void setOutputGain(double gain) { outputGain = gain; }
    
    // Get parameters
    double getReverbTime() const { return reverbTime; }
    double getDamping() const { return damping; }
    double getMix() const { return wetMix; }
    double getPreDelay() const { return preDelay; }
    
    // Enable/disable specific effects
    void enableHiFreqDamping(bool enable) { hiFreqDampingEnabled = enable; }
    void enableLowFreqDecay(bool enable) { lowFreqDecayEnabled = enable; }

private:
    SpringType springType;
    
    // Physical parameters
    double reverbTime = 2.0;         // Reverb decay time in seconds
    double damping = 0.3;            // High frequency damping
    double wetMix = 0.3;             // Mix ratio of wet to dry signal
    double preDelay = 0.01;          // Pre-delay in seconds
    double inputGain = 1.0;
    double outputGain = 0.8;
    
    // Spring tank characteristics
    double springLength = 0.5;       // Length in meters
    double springTension = 100.0;    // Tension factor
    double springMass = 0.01;        // Mass per unit length
    
    // Internal simulation parameters
    double sampleRate = 44100.0;
    int preDelayBufferSize;
    int reverbBufferSize;
    
    // Delay line buffers (for spring reverberation simulation)
    std::vector<double> preDelayBuffer;
    std::vector<double> reverbBuffer;
    int preDelayWriteIndex = 0;
    int preDelayReadIndex = 0;
    int reverbWriteIndex = 0;
    int reverbReadIndex = 0;
    
    // All-pass filter parameters for early reflections
    std::vector<double> allpassBuffers;
    std::vector<int> allpassDelays;
    std::vector<double> allpassFeedbacks;
    
    // Input/output
    int inputPin = 0;
    int outputPin = 1;
    int groundPin = 2;
    
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    
    // Effects enable flags
    bool hiFreqDampingEnabled = true;
    bool lowFreqDecayEnabled = true;
    
    // Initialize the reverb based on spring type
    void initializeSpring(SpringType type);
    
    // Process the signal through the spring reverb simulation
    void processSignal();
    
    // Apply all-pass filter for diffuse reflections
    double allpassFilter(int stage, double input);
    
    // Apply damping and decay to the reverb signal
    double applyDamping(double signal, int delayIndex);
};

// Class to model plate reverb circuits
class PlateReverb : public ElectricNodeBase {
public:
    PlateReverb();
    virtual ~PlateReverb() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure plate reverb parameters
    void setReverbTime(double time);   // in seconds
    void setDamping(double damp);      // 0.0 to 1.0
    void setSize(double size);         // relative size 0.1 to 2.0
    void setBrightness(double bright); // 0.0 to 1.0
    void setMix(double mix);           // 0.0 (dry) to 1.0 (wet)
    
    // Get parameters
    double getReverbTime() const { return reverbTime; }
    double getDamping() const { return damping; }
    double getSize() const { return plateSize; }
    double getBrightness() const { return brightness; }
    double getMix() const { return wetMix; }

private:
    // Physical parameters
    double reverbTime = 2.5;         // Reverb decay time in seconds
    double damping = 0.2;            // High frequency damping
    double plateSize = 1.0;          // Relative plate size
    double brightness = 0.5;         // HF content
    double wetMix = 0.3;             // Mix ratio
    double inputGain = 1.0;
    double outputGain = 0.8;
    
    // Plate reverb simulation parameters
    static const int NUM_COMBS = 8;
    static const int NUM_ALLPASSES = 4;
    
    // Comb filter buffers (to simulate standing waves in the plate)
    std::vector<std::vector<double>> combBuffers;
    std::vector<int> combBufferSizes;
    std::vector<double> combFeedbacks;
    std::vector<int> combIndices;
    
    // All-pass filter buffers (for diffusion)
    std::vector<std::vector<double>> allpassBuffers;
    std::vector<int> allpassBufferSizes;
    std::vector<double> allpassFeedbacks;
    std::vector<int> allpassIndices;
    
    // Input/output
    int inputPin = 0;
    int outputPin = 1;
    int groundPin = 2;
    
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double sampleRate = 44100.0;
    
    // Initialize comb and all-pass filter parameters
    void initializeFilters();
    
    // Process the signal through the plate reverb simulation
    void processSignal();
};

// Component for tube reverb driver circuits (e.g., reverb driver tube)
class TubeReverbDriver : public ElectricNodeBase {
public:
    TubeReverbDriver(const std::string& tubeType = "12AX7");
    virtual ~TubeReverbDriver() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure driver parameters
    void setDrive(double drive) { driverGain = drive; }
    void setBias(double bias) { operatingBias = bias; }
    void setOutputImpedance(double impedance) { outputZ = impedance; }
    void setTubeType(const std::string& type) { tubeType = type; }
    
    // Get parameters
    double getDrive() const { return driverGain; }
    double getBias() const { return operatingBias; }
    double getOutputImpedance() const { return outputZ; }
    std::string getTubeType() const { return tubeType; }

private:
    std::string tubeType = "12AX7";
    double driverGain = 20.0;
    double operatingBias = -1.5;
    double outputZ = 600.0;         // Output impedance in ohms
    
    // Input/output
    int inputPin = 0;
    int outputPin = 1;
    int bPlusPin = 2;
    int groundPin = 3;
    
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double bPlusVoltage = 250.0;
    
    // Process the signal through the tube driver
    void processSignal();
};

// Complete reverb unit simulation with tube driver and spring/plate reverb
class TubeReverbUnit : public ElectricNodeBase {
public:
    enum ReverbConfiguration {
        SPRING_REVERB,
        PLATE_REVERB,
        CHAMBER_REVERB  // Could be added later
    };
    
    TubeReverbUnit(ReverbConfiguration config = SPRING_REVERB);
    virtual ~TubeReverbUnit() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure reverb parameters
    void setReverbTime(double time);
    void setDamping(double damp);
    void setMix(double mix);
    void setPreDelay(double delay);
    
    // Access to the reverb components
    SpringReverb* getSpringReverb() { return springReverb.get(); }
    PlateReverb* getPlateReverb() { return plateReverb.get(); }
    TubeReverbDriver* getDriver() { return driver.get(); }
    
    // Set reverb type
    void setConfiguration(ReverbConfiguration config);

private:
    ReverbConfiguration config;
    
    // Components
    std::unique_ptr<TubeReverbDriver> driver;
    std::unique_ptr<SpringReverb> springReverb;
    std::unique_ptr<PlateReverb> plateReverb;
    
    // Input/output
    int inputPin = 0;
    int outputPin = 1;
    int bPlusPin = 2;
    
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double bPlusVoltage = 250.0;
    
    // Process the signal through the complete reverb unit
    void processSignal();
};

#endif // TUBE_REVERB_CIRCUITS_H