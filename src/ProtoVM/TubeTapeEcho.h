#ifndef _ProtoVM_TubeTapeEcho_h_
#define _ProtoVM_TubeTapeEcho_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include "LFO.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based tape echo circuits
class TubeTapeEcho : public ElectricNodeBase {
public:
    typedef TubeTapeEcho CLASSNAME;

    // Different types of tape echo configurations
    enum TapeEchoType {
        ECHORECORDER_TAPE_ECHO,    // Classic analog tape echo (like Echoplex)
        SPACE_ECHO,                // Spring reverb-like echo
        MAGNETIC_TAPE_DELAY,       // Magnetic tape delay
        VINTAGE_ANALOG_DELAY      // Vintage-style analog delay with tape characteristics
    };

    TubeTapeEcho(TapeEchoType type = ECHORECORDER_TAPE_ECHO);
    virtual ~TubeTapeEcho();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeTapeEcho"; }

    // Configuration methods
    void SetDelayTime(double time);
    void SetFeedback(double feedback);
    void SetTapeSaturation(double saturation);
    void SetWowAndFlutter(double wow_flutter);
    void SetHeadDistance(double distance);  // Distance between record and playback heads
    void SetTapeSpeed(double speed);        // Tape speed in inches per second
    void SetLowPassFilterFreq(double freq); // LPF frequency to simulate tape characteristics
    void SetHighPassFilterFreq(double freq); // HPF frequency to roll off low frequencies
    void SetStereoWidth(double width);      // Stereo widening
    
    // Getter methods
    double GetDelayTime() const { return delay_time; }
    double GetFeedback() const { return feedback; }
    double GetTapeSaturation() const { return tape_saturation; }
    double GetWowAndFlutter() const { return wow_flutter; }
    double GetHeadDistance() const { return head_distance; }
    double GetTapeSpeed() const { return tape_speed; }
    double GetLowPassFilterFreq() const { return low_pass_freq; }
    double GetHighPassFilterFreq() const { return high_pass_freq; }
    double GetStereoWidth() const { return stereo_width; }

private:
    TapeEchoType echo_type;
    
    // Parameters
    double delay_time;           // Base delay time in seconds
    double feedback;             // Feedback amount (0.0 to 0.99)
    double tape_saturation;      // Amount of tape saturation (0.0 to 1.0)
    double wow_flutter;          // Wow and flutter amount (0.0 to 0.1)
    double head_distance;        // Distance between heads in cm
    double tape_speed;           // Tape speed in ips (inches per second)
    double low_pass_freq;        // Low pass filter frequency to emulate tape characteristics
    double high_pass_freq;       // High pass filter frequency
    double stereo_width;         // Stereo widening effect
    
    // Internal parameters
    std::vector<double> delay_line_left;
    std::vector<double> delay_line_right;
    size_t write_pos;
    size_t read_pos_left;
    size_t read_pos_right;
    size_t delay_line_size;
    
    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tube_characteristics;
    
    // Processing state
    double input_signal_left;
    double input_signal_right;
    double output_signal_left;
    double output_signal_right;
    double modulation_phase;     // For wow and flutter modulation
    
    // Constants
    static constexpr size_t MAX_DELAY_SIZE = 88200; // 2 seconds at 44.1kHz
    static constexpr double MIN_DELAY_TIME = 0.01;   // 10ms minimum delay
    static constexpr double MAX_DELAY_TIME = 2.0;    // 2 seconds maximum delay
    static constexpr double MIN_FEEDBACK = 0.0;      // No feedback
    static constexpr double MAX_FEEDBACK = 0.99;     // Max feedback (before oscillation)
    static constexpr double MIN_TAPE_SATURATION = 0.0; // No saturation
    static constexpr double MAX_TAPE_SATURATION = 1.0; // Max saturation
    static constexpr double MIN_WOW_FLUTTER = 0.0;   // No wow/flutter
    static constexpr double MAX_WOW_FLUTTER = 0.1;   // Max wow/flutter
    static constexpr double MIN_HEAD_DISTANCE = 0.1; // 0.1 cm
    static constexpr double MAX_HEAD_DISTANCE = 5.0; // 5.0 cm
    static constexpr double MIN_TAPE_SPEED = 1.0;    // 1 ips
    static constexpr double MAX_TAPE_SPEED = 15.0;   // 15 ips
    static constexpr double MIN_LOWPASS_FREQ = 100.0; // 100Hz
    static constexpr double MAX_LOWPASS_FREQ = 10000.0; // 10kHz
    static constexpr double MIN_HIGHPASS_FREQ = 10.0;  // 10Hz
    static constexpr double MAX_HIGHPASS_FREQ = 1000.0; // 1kHz
    static constexpr double MIN_STEREO_WIDTH = 0.0;    // Mono
    static constexpr double MAX_STEREO_WIDTH = 2.0;    // Wide stereo
    
    void InitializeEcho(TapeEchoType type);
    void ProcessSignal();
    void ApplyTapeCharacteristics();
    void ApplyWowAndFlutter();
    void ApplyFilters();
};

#endif