#ifndef _ProtoVM_TubeAudioIO_h_
#define _ProtoVM_TubeAudioIO_h_

#include "AnalogCommon.h"
#include "AnalogComponents.h"
#include "TubeModels.h"
#include "AudioSignalPath.h"
#include <sndfile.h>
#include <memory>

// Audio input interface for tube circuits
class TubeAudioInput : public AnalogNodeBase {
public:
    typedef TubeAudioInput CLASSNAME;

    // Audio input source type
    enum class InputType {
        MICROPHONE,
        LINE_LEVEL,
        INSTRUMENT,
        FILE_INPUT,
        REALTIME_INPUT  // For future real-time audio input
    };

    TubeAudioInput(InputType source_type = InputType::LINE_LEVEL, 
                   double input_impedance = 1e6,  // High impedance input
                   double max_input_level = 10.0); // Max input voltage before clipping
    virtual ~TubeAudioInput();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeAudioInput"; }

    // Get/set input type
    void SetInputType(InputType type) { input_type = type; }
    InputType GetInputType() const { return input_type; }

    // Get/set input impedance (affects how it loads the previous stage)
    void SetInputImpedance(double impedance) { input_impedance = impedance; }
    double GetInputImpedance() const { return input_impedance; }

    // Get/set max input level
    void SetMaxInputLevel(double level) { max_input_level = level; }
    double GetMaxInputLevel() const { return max_input_level; }

    // Set audio data for file-based input
    void SetAudioData(const std::vector<std::vector<double>>& data); // [channel][sample]
    void SetSampleRate(int rate) { sample_rate = rate; }

    // Get current input signal
    double GetOutput() const { return output_signal; }

    // Connection points
    int GetOutputPin() const { return 0; }  // Output pin for connecting to tube circuit

private:
    InputType input_type;
    double input_impedance;      // Input impedance in ohms
    double max_input_level;      // Maximum input voltage before clipping
    std::vector<std::vector<double>> audio_data;  // Multi-channel audio data [channel][sample]
    size_t current_sample_idx;   // Current sample position in audio data
    int sample_rate;             // Sample rate for audio data
    double output_signal;        // Current output signal value
    double last_processed_signal; // Last processed signal (for filtering)
    bool has_audio_data;         // Whether we're using pre-loaded data

    // Simulate input characteristics based on type
    double ProcessInputSignal(double input_signal);
};

// Audio output interface for tube circuits
class TubeAudioOutput : public AnalogNodeBase {
public:
    typedef TubeAudioOutput CLASSNAME;

    // Audio output destination type
    enum class OutputType {
        HEADPHONES,
        LINE_OUT,
        SPEAKER,
        FILE_OUTPUT,
        REALTIME_OUTPUT  // For future real-time audio output
    };

    TubeAudioOutput(OutputType dest_type = OutputType::LINE_OUT,
                    double output_impedance = 600.0,  // Standard line output impedance
                    double load_impedance = 10000.0); // Expected load impedance
    virtual ~TubeAudioOutput();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeAudioOutput"; }

    // Get/set output type
    void SetOutputType(OutputType type) { output_type = type; }
    OutputType GetOutputType() const { return output_type; }

    // Get/set output impedance
    void SetOutputImpedance(double impedance) { output_impedance = impedance; }
    double GetOutputImpedance() const { return output_impedance; }

    // Get/set load impedance
    void SetLoadImpedance(double impedance) { load_impedance = impedance; }
    double GetLoadImpedance() const { return load_impedance; }

    // Set input signal to the output stage
    void SetInputSignal(double signal) { input_signal = signal; }
    double GetInputSignal() const { return input_signal; }

    // Connection points
    int GetInputPin() const { return 0; }   // Input pin for connecting from tube circuit

    // File output management
    bool StartFileOutput(const std::string& filepath, int channels = 2);
    bool StopFileOutput();
    bool IsFileOutputActive() const { return file_output_active; }

    // Get/set sample rate for file output
    void SetSampleRate(int rate) { sample_rate = rate; }
    int GetSampleRate() const { return sample_rate; }

    // Buffer management
    void SetMaxBufferSize(int samples) { max_buffer_size = samples; }
    int GetCurrentBufferSize() const { return static_cast<int>(output_buffer.size()); }
    bool ClearBuffer();

private:
    OutputType output_type;
    double output_impedance;     // Output impedance in ohms
    double load_impedance;       // Expected load impedance in ohms
    double input_signal;         // Input signal from tube circuit
    double output_signal;        // Processed output signal
    double last_processed_signal; // Last processed signal (for filtering)
    int sample_rate;             // Sample rate for processing
    int max_buffer_size;         // Maximum buffer size in samples

    // File output management
    std::unique_ptr<SNDFILE, decltype(&sf_close)> output_file_handle;
    bool file_output_active;
    std::string current_file_path;
    std::vector<std::vector<double>> output_buffer;  // [channel][sample] - circular buffer
    size_t buffer_write_pos;      // Current write position in buffer

    // Process output signal based on output type and characteristics
    double ProcessOutputSignal(double input_signal);

    // Write samples to file output
    bool WriteBufferToFile();
};

// Stereo pair of tube audio interfaces
class TubeAudioStereoInterface {
public:
    TubeAudioStereoInterface();
    virtual ~TubeAudioStereoInterface();

    TubeAudioInput& GetLeftInput() { return left_input; }
    TubeAudioInput& GetRightInput() { return right_input; }
    TubeAudioOutput& GetLeftOutput() { return left_output; }
    TubeAudioOutput& GetRightOutput() { return right_output; }

    void SetSampleRate(int rate);
    int GetSampleRate() const { return sample_rate; }

    // Start file output for both channels
    bool StartFileOutput(const std::string& filepath);
    bool StopFileOutput();

    // Process both channels
    bool Process();

private:
    TubeAudioInput left_input;
    TubeAudioInput right_input;
    TubeAudioOutput left_output;
    TubeAudioOutput right_output;
    int sample_rate;
};

// Class for managing parameter automation
class ParameterAutomation {
public:
    ParameterAutomation();
    virtual ~ParameterAutomation();

    struct AutomationPoint {
        double time;         // Time in seconds
        double value;        // Parameter value at that time
        bool active;         // Whether this point is active

        AutomationPoint(double t = 0.0, double v = 0.0) : time(t), value(v), active(true) {}
    };

    // Add an automation point
    void AddAutomationPoint(int param_id, const AutomationPoint& point);
    
    // Get parameter value at a specific time
    double GetParameterValue(int param_id, double current_time);
    
    // Set parameter value at current simulation time
    void SetParameterAtTime(int param_id, double value);

    // Set interpolation mode for parameter changes
    enum class InterpolationMode {
        LINEAR,
        SMOOTH,
        STEP
    };
    
    void SetInterpolationMode(int param_id, InterpolationMode mode);
    InterpolationMode GetInterpolationMode(int param_id) const;

    // Clear automation for a parameter
    void ClearAutomation(int param_id);

    // Get automation points for a parameter
    const std::vector<AutomationPoint>& GetAutomationPoints(int param_id) const;

private:
    struct ParameterData {
        std::vector<AutomationPoint> points;
        InterpolationMode interp_mode;
        double current_value;

        ParameterData() : interp_mode(InterpolationMode::LINEAR), current_value(0.0) {}
    };

    std::map<int, ParameterData> param_map;
};

#endif