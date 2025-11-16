#ifndef _ProtoVM_AudioProcessingModes_h_
#define _ProtoVM_AudioProcessingModes_h_

#include "AnalogCommon.h"
#include "AudioSignalPath.h"
#include <vector>

// Enum for audio processing modes
enum class AudioMode {
    MONO,
    STEREO,
    LEFT_ONLY,
    RIGHT_ONLY,
    M_S_ENCODE,    // Mid-Side encoding
    M_S_DECODE,    // Mid-Side decoding
    DUAL_MONO      // Two independent mono channels
};

// Structure for audio channel configuration
struct AudioChannelConfig {
    AudioMode mode;
    int channel_count;
    bool is_interleaved;     // For buffer organization
    double sample_rate;
    
    AudioChannelConfig(AudioMode m = AudioMode::STEREO, int ch = 2, double sr = 44100.0)
        : mode(m), channel_count(ch), is_interleaved(true), sample_rate(sr) {}
};

// Base class for audio processors that handle different channel configurations
class AudioProcessor : public AnalogNodeBase {
public:
    typedef AudioProcessor CLASSNAME;
    
    AudioProcessor(const std::string& name = "AudioProcessor");
    virtual ~AudioProcessor();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AudioProcessor"; }

    // Set/get audio processing mode
    void SetMode(AudioMode mode);
    AudioMode GetMode() const { return config.mode; }

    // Set/get channel count
    void SetChannelCount(int count);
    int GetChannelCount() const { return config.channel_count; }

    // Set/get sample rate
    void SetSampleRate(double rate);
    double GetSampleRate() const { return config.sample_rate; }

    // Configure the processor
    void SetConfig(const AudioChannelConfig& new_config);
    const AudioChannelConfig& GetConfig() const { return config; }

    // Process audio buffer with specified configuration
    virtual bool ProcessBuffer(const std::vector<std::vector<double>>& input, 
                              std::vector<std::vector<double>>& output);

    // Get/set bypass state
    void SetBypass(bool bypass) { bypassed = bypass; }
    bool IsBypassed() const { return bypassed; }

    // Get input/output channel counts
    int GetInputChannelCount() const { return input_channels.size(); }
    int GetOutputChannelCount() const { return output_channels.size(); }

    // Set input channels
    void SetInputChannels(const std::vector<double>& inputs);
    const std::vector<double>& GetInputChannels() const { return input_channels; }

    // Get output channels
    const std::vector<double>& GetOutputChannels() const { return output_channels; }

protected:
    // Internal processing that depends on mode
    virtual bool InternalProcess() = 0;

private:
    AudioChannelConfig config;
    std::vector<double> input_channels;
    std::vector<double> output_channels;
    std::string processor_name;
    bool bypassed;
};

// Mono processor - always processes as mono regardless of input
class MonoProcessor : public AudioProcessor {
public:
    typedef MonoProcessor CLASSNAME;
    
    MonoProcessor(const std::string& name = "MonoProcessor");
    virtual ~MonoProcessor() {}

    virtual String GetClassName() const override { return "MonoProcessor"; }

protected:
    virtual bool InternalProcess() override;

private:
    // Mono-specific processing parameters
    double mono_output;
};

// Stereo processor - processes left and right channels separately or together
class StereoProcessor : public AudioProcessor {
public:
    typedef StereoProcessor CLASSNAME;
    
    StereoProcessor(const std::string& name = "StereoProcessor");
    virtual ~StereoProcessor() {}

    virtual String GetClassName() const override { return "StereoProcessor"; }

    // Stereo-specific functions
    void SetStereoWidth(double width) { stereo_width = width; }
    double GetStereoWidth() const { return stereo_width; }

    void SetBalance(double balance) { // -1.0 = full left, 1.0 = full right
        channel_balance = std::max(-1.0, std::min(1.0, balance)); 
    }
    double GetBalance() const { return channel_balance; }

protected:
    virtual bool InternalProcess() override;

private:
    double stereo_width;      // 0.0 = mono, 1.0 = full stereo
    double channel_balance;   // -1.0 = full left, 1.0 = full right
    std::vector<double> left_channel;
    std::vector<double> right_channel;
};

// Mid-Side processor - converts between stereo and mid-side formats
class MidSideProcessor : public AudioProcessor {
public:
    typedef MidSideProcessor CLASSNAME;
    
    MidSideProcessor(const std::string& name = "MidSideProcessor");
    virtual ~MidSideProcessor() {}

    virtual String GetClassName() const override { return "MidSideProcessor"; }

    // Set processing direction
    void SetEncodeMode(bool encode) { is_encoder = encode; }
    bool IsEncodeMode() const { return is_encoder; }

protected:
    virtual bool InternalProcess() override;

private:
    bool is_encoder;  // True for L,R -> M,S, false for M,S -> L,R
    double mid_signal;
    double side_signal;
};

// Dual Mono processor - processes two independent mono channels
class DualMonoProcessor : public AudioProcessor {
public:
    typedef DualMonoProcessor CLASSNAME;
    
    DualMonoProcessor(const std::string& name = "DualMonoProcessor");
    virtual ~DualMonoProcessor() {}

    virtual String GetClassName() const override { return "DualMonoProcessor"; }

    // Set different parameters for each channel
    void SetChannelParams(int channel, double param1, double param2) {
        if (channel == 0) {
            ch1_param1 = param1;
            ch1_param2 = param2;
        } else if (channel == 1) {
            ch2_param1 = param1;
            ch2_param2 = param2;
        }
    }

protected:
    virtual bool InternalProcess() override;

private:
    double ch1_param1, ch1_param2;  // Parameters for channel 1
    double ch2_param1, ch2_param2;  // Parameters for channel 2
};

// Utility functions for audio mode conversion
namespace AudioModeUtils {
    // Convert stereo to mono by averaging
    double StereoToMono(const std::vector<double>& stereo_input);
    
    // Convert mono to stereo by duplicating
    std::vector<double> MonoToStereo(double mono_input);
    
    // Convert stereo L,R to mid-side M,S
    void StereoToMidSide(double left, double right, double& mid, double& side);
    
    // Convert mid-side M,S to stereo L,R
    void MidSideToStereo(double mid, double side, double& left, double& right);
    
    // Apply balance to stereo signal (-1.0 to 1.0)
    void ApplyBalance(double& left, double& right, double balance);
    
    // Apply stereo width adjustment (0.0 to 1.0)
    void ApplyStereoWidth(double& left, double& right, double width);
}

#endif