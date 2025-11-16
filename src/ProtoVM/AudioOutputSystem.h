#ifndef _ProtoVM_AudioOutputSystem_h_
#define _ProtoVM_AudioOutputSystem_h_

#include "AnalogCommon.h"
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sndfile.h>  // For audio file I/O

// Audio format configuration
struct AudioFormat {
    int sample_rate;      // Samples per second (e.g., 44100, 48000, 96000)
    int bit_depth;        // Bits per sample (e.g., 16, 24, 32)
    int channels;         // Number of audio channels (1=mono, 2=stereo, etc.)
    double max_amplitude; // Maximum amplitude before clipping (typically 1.0)
    
    AudioFormat(int sr = 44100, int bd = 16, int ch = 2) 
        : sample_rate(sr), bit_depth(bd), channels(ch) {
        // Calculate max amplitude based on bit depth
        switch (bit_depth) {
            case 8:
                max_amplitude = 127.0;
                break;
            case 16:
                max_amplitude = 32767.0;
                break;
            case 24:
                max_amplitude = 8388607.0;
                break;
            case 32:
                max_amplitude = 2147483647.0;
                break;
            default:
                max_amplitude = 32767.0; // Default to 16-bit
                break;
        }
    }
};

// Audio output system
class AudioOutputSystem : public AnalogNodeBase {
public:
    typedef AudioOutputSystem CLASSNAME;

    AudioOutputSystem(const AudioFormat& format = AudioFormat());
    virtual ~AudioOutputSystem();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AudioOutputSystem"; }

    // Configure audio format
    void SetFormat(const AudioFormat& format);
    const AudioFormat& GetFormat() const { return audio_format; }

    // Add audio samples to output buffer
    bool AddSample(const std::vector<double>& sample);  // For multi-channel
    bool AddSample(double mono_sample);                 // For mono

    // Write buffer to file
    bool WriteToFile(const std::string& filepath, int sf_format = SF_FORMAT_WAV | SF_FORMAT_PCM_16);
    
    // Real-time audio output (placeholder - would need platform-specific implementation)
    bool StartRealTimeOutput();
    bool StopRealTimeOutput();
    bool IsRealTimeOutputActive() const { return real_time_active; }

    // Buffer management
    int GetBufferSampleCount() const { return buffer.size() / audio_format.channels; }
    void ClearBuffer();
    void SetMaxBufferSize(int max_samples);
    
    // Get/set sample rate
    int GetSampleRate() const { return audio_format.sample_rate; }
    void SetSampleRate(int sr) { audio_format.sample_rate = sr; }

    // Get/set bit depth
    int GetBitDepth() const { return audio_format.bit_depth; }
    void SetBitDepth(int bd) { audio_format.bit_depth = bd; }

    // Get/set number of channels
    int GetChannelCount() const { return audio_format.channels; }
    void SetChannelCount(int ch) { audio_format.channels = ch; }

    // Apply sample format conversion and clipping if needed
    double ConvertSample(double input_sample) const;

private:
    AudioFormat audio_format;
    std::vector<double> buffer;  // Audio sample buffer
    int max_buffer_size;         // Maximum buffer size in samples
    std::unique_ptr<SNDFILE, decltype(&sf_close)> output_file_handle;
    bool real_time_active;       // Whether real-time output is active
    bool file_output_active;     // Whether file output is active

    // Initialize the audio system
    bool Initialize();
    
    // Process the buffer for output
    void ProcessOutput();
    
    // Convert internal samples to the target format
    std::vector<short> ConvertBufferToShort() const;
};

#endif