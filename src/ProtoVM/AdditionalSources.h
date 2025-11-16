#ifndef _ProtoVM_AdditionalSources_h_
#define _ProtoVM_AdditionalSources_h_

#include "AnalogCommon.h"
#include "Common.h"
#include <cmath>
#include <random>

// Antenna component - receives and generates signals based on electromagnetic environment
class Antenna : public AnalogNodeBase {
public:
    typedef Antenna CLASSNAME;

    Antenna(double sensitivity = 1.0, double frequency = 100.0e6); // 100 MHz default
    virtual ~Antenna() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "Antenna"; }

    void SetSensitivity(double sens);
    double GetSensitivity() const { return sensitivity; }
    
    void SetFrequency(double freq);
    double GetFrequency() const { return frequency; }

private:
    double sensitivity;    // Sensitivity factor for signal reception
    double frequency;      // Center frequency in Hz
    std::random_device rd;
    std::mt19937 gen;
    std::normal_distribution<double> noise_dist;  // For realistic signal noise
    
    static constexpr double PI = 3.14159265358979323846;
};

// AM Source - Amplitude Modulated signal generator
class AmSource : public AnalogNodeBase {
public:
    typedef AmSource CLASSNAME;

    AmSource(double carrier_freq = 1000.0, double modulation_freq = 10.0, 
             double modulation_index = 0.5, double amplitude = 1.0);
    virtual ~AmSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AmSource"; }

    void SetCarrierFrequency(double freq);
    double GetCarrierFrequency() const { return carrier_freq; }
    
    void SetModulationFrequency(double freq);
    double GetModulationFrequency() const { return modulation_freq; }
    
    void SetModulationIndex(double index);
    double GetModulationIndex() const { return modulation_index; }
    
    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }

private:
    double carrier_freq;      // Carrier frequency in Hz
    double modulation_freq;   // Modulation frequency in Hz
    double modulation_index;  // Modulation index (0.0 to 1.0)
    double amplitude;         // Peak amplitude in volts
    
    static constexpr double PI = 3.14159265358979323846;
};

// FM Source - Frequency Modulated signal generator
class FmSource : public AnalogNodeBase {
public:
    typedef FmSource CLASSNAME;

    FmSource(double carrier_freq = 1000.0, double modulation_freq = 10.0, 
             double modulation_index = 1.0, double amplitude = 1.0);
    virtual ~FmSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "FmSource"; }

    void SetCarrierFrequency(double freq);
    double GetCarrierFrequency() const { return carrier_freq; }
    
    void SetModulationFrequency(double freq);
    double GetModulationFrequency() const { return modulation_freq; }
    
    void SetModulationIndex(double index);
    double GetModulationIndex() const { return modulation_index; }
    
    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }

private:
    double carrier_freq;      // Carrier frequency in Hz
    double modulation_freq;   // Modulation frequency in Hz
    double modulation_index;  // Modulation index
    double amplitude;         // Peak amplitude in volts
    
    static constexpr double PI = 3.14159265358979323846;
};

// Current Source - Provides constant current regardless of voltage
class CurrentSource : public AnalogNodeBase {
public:
    typedef CurrentSource CLASSNAME;

    CurrentSource(double current = 0.001); // 1mA default
    virtual ~CurrentSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "CurrentSource"; }

    void SetCurrent(double current);
    double GetCurrent() const { return current_val; }

private:
    double current_val;  // Output current in amps
};

// Noise Generator - Generates various types of electronic noise
class NoiseGenerator : public AnalogNodeBase {
public:
    typedef NoiseGenerator CLASSNAME;

    enum class NoiseType {
        WHITE,
        PINK,
        BROWN
    };

    NoiseGenerator(NoiseType type = NoiseType::WHITE, double amplitude = 0.1);
    virtual ~NoiseGenerator() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "NoiseGenerator"; }

    void SetNoiseType(NoiseType type);
    NoiseType GetNoiseType() const { return noise_type; }
    
    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }

private:
    NoiseType noise_type;
    double amplitude;          // Noise amplitude in volts
    std::random_device rd;
    std::mt19937 gen;
    std::normal_distribution<double> white_noise_dist;
    double pink_buffer[3];     // For pink noise generation
    double brown_value;        // For brown noise generation
    
    static constexpr double PI = 3.14159265358979323846;
};

// Audio Input - Simulates audio input from microphone, file, etc.
class AudioInput : public AnalogNodeBase {
public:
    typedef AudioInput CLASSNAME;

    AudioInput(double amplitude = 1.0, double frequency = 440.0);
    virtual ~AudioInput() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AudioInput"; }

    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }
    
    void SetFrequency(double freq);
    double GetFrequency() const { return frequency; }
    
    // In a real implementation, this would connect to audio input device
    void SetAudioData(const std::vector<double>& data);  // For pre-loaded audio data

private:
    double amplitude;          // Peak amplitude in volts
    double frequency;          // Base frequency in Hz
    std::vector<double> audio_data;  // Pre-loaded audio samples
    size_t current_sample_idx;       // Current position in audio data
    bool has_audio_data;             // Whether we're using pre-loaded data
    
    static constexpr double PI = 3.14159265358979323846;
};

// Data Input - Digital data input (parallel or serial)
class DataInput : public AnalogNodeBase {
public:
    typedef DataInput CLASSNAME;

    enum class InputType {
        PARALLEL,
        SERIAL
    };

    DataInput(InputType type = InputType::PARALLEL, int bits = 8);
    virtual ~DataInput() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "DataInput"; }

    void SetInputType(InputType type);
    InputType GetInputType() const { return input_type; }
    
    void SetBitCount(int bits);
    int GetBitCount() const { return bit_count; }
    
    void SetDataValue(uint32_t value);  // For parallel input
    uint32_t GetDataValue() const { return data_value; }
    
    void SetSerialData(const std::vector<bool>& data);  // For serial input
    void SetClockFrequency(double freq);
    double GetClockFrequency() const { return clock_frequency; }

private:
    InputType input_type;
    int bit_count;              // Number of data bits (for parallel)
    uint32_t data_value;        // Current parallel data value
    std::vector<bool> serial_data;  // Serial input data
    size_t serial_bit_idx;      // Current bit position in serial data
    double clock_frequency;     // Clock frequency for serial data
    bool clock_phase;           // Current clock phase for serial data
    double time_per_bit;        // Time per bit based on clock frequency
    
    static constexpr double PI = 3.14159265358979323846;
};

// External Voltage - Stub for scripting, allows voltage to be set externally
class ExternalVoltage : public AnalogNodeBase {
public:
    typedef ExternalVoltage CLASSNAME;

    ExternalVoltage(double initial_voltage = 0.0);
    virtual ~ExternalVoltage() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "ExternalVoltage"; }

    void SetVoltage(double v);
    double GetVoltage() const { return voltage; }
    
    // For scripting interface - allows external code to set voltage
    void SetExternalVoltage(double v);
    void SetWaveformData(const std::vector<double>& data);
    void LoadWaveFile(const String& filename);

private:
    double voltage;                    // Current output voltage
    std::vector<double> wave_data;     // Waveform data from file
    size_t current_sample_idx;         // Current sample index when playing back wave data
    bool use_wave_data;                // Whether to use waveform data or fixed voltage
    double sample_rate;                // Sample rate for wave data playback
    
    static constexpr double PI = 3.14159265358979323846;
};

#endif