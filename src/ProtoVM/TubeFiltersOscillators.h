#ifndef TUBE_FILTERS_OSCILLATORS_H
#define TUBE_FILTERS_OSCILLATORS_H

#include "Common.h"
#include "TubeComponents.h"
#include <vector>
#include <memory>

// Class for tube-based filter circuits
class TubeFilter : public ElectricNodeBase {
public:
    enum FilterType {
        LOWPASS,
        HIGHPASS,
        BANDPASS,
        NOTCH,
        ALLPASS
    };
    
    enum CircuitTopology {
        RC_LPF,           // RC low pass filter
        RC_HPF,           // RC high pass filter
        LC_BANDPASS,      // LC bandpass
        PI_NETWORK,       // Pi-section filter
        TUBE_RC_LPF       // RC filter with tube buffering
    };
    
    TubeFilter(FilterType type = LOWPASS, CircuitTopology topology = TUBE_RC_LPF);
    virtual ~TubeFilter() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Configure filter parameters
    void setCutoffFrequency(double freq);
    void setResonance(double res);  // For bandpass/notch filters
    void setGain(double gain) { filterGain = gain; }
    void setFilterType(FilterType type) { filterType = type; }
    void setTopology(CircuitTopology topology) { circuitTopology = topology; }
    
    // Get parameters
    double getCutoffFrequency() const { return cutoffFreq; }
    double getResonance() const { return resonance; }
    double getGain() const { return filterGain; }
    
private:
    FilterType filterType;
    CircuitTopology circuitTopology;
    
    // Filter parameters
    double cutoffFreq = 1000.0;  // 1kHz default
    double resonance = 0.707;    // Butterworth response
    double filterGain = 1.0;
    
    // Component values (for simulation)
    double resistance = 100000.0;  // 100kΩ default
    double capacitance = 0.000001; // 1μF default
    double inductance = 0.1;       // 100mH default
    
    // State variables for IIR filter implementation
    double sampleRate = 44100.0;
    std::vector<double> inputHistory;
    std::vector<double> outputHistory;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;  // For voltage-controlled filters
    
    // Operating parameters
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    
    // Tube buffer if used
    std::unique_ptr<TubeComponent> tubeBuffer;
    
    // Calculate filter response based on configuration
    void calculateResponse();
    
    // Process signal through filter
    void processSignal();
};

// Class for tube-based oscillator circuits
class TubeOscillator : public ElectricNodeBase {
public:
    enum OscillatorType {
        HARTLEY,
        COLPITTS,
        PIERCE,
        WIEN_BRIDGE,
        PHASE_SHIFT,
        RELAXATION
    };
    
    enum Waveform {
        SINE,
        TRIANGLE,
        SAWTOOTH,
        SQUARE
    };
    
    TubeOscillator(OscillatorType type = WIEN_BRIDGE);
    virtual ~TubeOscillator() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Configure oscillator parameters
    void setFrequency(double freq);
    void setWaveform(Waveform wf) { waveform = wf; }
    void setAmplitude(double amp) { amplitude = amp; }
    void enableSync(bool sync) { syncEnabled = sync; }
    void setSyncSignal(double sync) { syncSignal = sync; }
    void enableModulation(bool mod) { modulationEnabled = mod; }
    void setModulationSignal(double mod) { modulationSignal = mod; }
    
    // Get parameters
    double getFrequency() const { return frequency; }
    Waveform getWaveform() const { return waveform; }
    double getAmplitude() const { return amplitude; }
    
    // Start/stop oscillation
    void start();
    void stop();
    bool isRunning() const { return oscillating; }
    
private:
    OscillatorType oscillatorType;
    Waveform waveform;
    
    // Oscillation parameters
    double frequency = 440.0;      // Default to A4
    double amplitude = 1.0;
    bool syncEnabled = false;
    double syncSignal = 0.0;
    bool modulationEnabled = false;
    double modulationSignal = 0.0;
    
    // For Wien bridge and RC oscillators
    double resistance = 10000.0;   // 10kΩ
    double capacitance = 0.0000001; // 0.1μF
    
    // State variables for oscillation
    double phase = 0.0;
    double phaseIncrement = 0.0;
    bool oscillating = false;
    double sampleRate = 44100.0;
    
    // Output signal
    double currentOutput = 0.0;
    
    // Pin connections
    int outputPin = 0;
    int frequencyControlPin = 1;   // For VCO functionality
    int syncPin = 2;               // For hard sync
    int modulationPin = 3;         // For FM/AM modulation
    
    // Tube oscillator components
    std::unique_ptr<TubeComponent> oscillatorTube;
    std::unique_ptr<TubeComponent> bufferTube;
    
    // Calculate the next sample of the oscillator
    double generateNextSample();
    
    // Initialize oscillator based on type
    void initOscillator();
    
    // Update phase increment based on frequency
    void updatePhaseIncrement();
};

// Class for voltage-controlled tube oscillator
class TubeVCO : public TubeOscillator {
public:
    TubeVCO();
    virtual ~TubeVCO() = default;
    
    // Set control voltage to frequency mapping
    void setControlRange(double minFreq, double maxFreq);
    void setLinearControl(bool linear) { linearControl = linear; }
    
private:
    double minFrequency = 20.0;    // 20Hz
    double maxFrequency = 20000.0; // 20kHz
    bool linearControl = false;    // Exponential by default
    
    // Process control voltage to frequency
    double controlVoltageToFrequency(double controlVoltage);
};

#endif // TUBE_FILTERS_OSCILLATORS_H