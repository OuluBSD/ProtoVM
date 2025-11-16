#ifndef _ProtoVM_TubeCircuits_h_
#define _ProtoVM_TubeCircuits_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include <vector>

// Enum for filter types
enum class TubeFilterType {
    LOW_PASS,      // Low-pass filter
    HIGH_PASS,     // High-pass filter
    BAND_PASS,     // Band-pass filter
    BAND_STOP,     // Band-stop (notch) filter
    ALL_PASS       // All-pass filter (phase shifter)
};

// Enum for oscillator types
enum class TubeOscillatorType {
    HARTLEY,       // Hartley oscillator
    COLPITTS,      // Colpitts oscillator
    PIERCE,        // Pierce oscillator
    WIEN_BRIDGE,   // Wien bridge oscillator
    PHASE_SHIFT    // Phase shift oscillator
};

// Base class for tube-based filters
class TubeFilter : public AnalogNodeBase {
public:
    typedef TubeFilter CLASSNAME;

    TubeFilter(TubeFilterType type = TubeFilterType::LOW_PASS);
    virtual ~TubeFilter();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeFilter"; }

    // Set/get input signal
    void SetInputSignal(double signal) { input_signal = signal; }
    double GetInputSignal() const { return input_signal; }
    
    // Get output signal
    double GetOutputSignal() const { return output_signal; }
    
    // Set/get cutoff frequency (for low-pass/high-pass) or center frequency (for band-pass)
    void SetCutoffFrequency(double freq) { cutoff_frequency = std::max(20.0, std::min(20000.0, freq)); }
    double GetCutoffFrequency() const { return cutoff_frequency; }
    
    // Set/get Q factor (resonance)
    void SetQFactor(double q) { q_factor = std::max(0.1, std::min(100.0, q)); }
    double GetQFactor() const { return q_factor; }
    
    // Set/get gain
    void SetGain(double gain) { filter_gain = std::max(0.1, std::min(100.0, gain)); }
    double GetGain() const { return filter_gain; }
    
    // Set filter type
    void SetFilterType(TubeFilterType type) { filter_type = type; }
    TubeFilterType GetFilterType() const { return filter_type; }
    
    // Get filter response (for visualization)
    double GetResponseAtFrequency(double freq) const;
    
    // Enable/disable the filter
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }

protected:
    TubeFilterType filter_type;
    double input_signal;
    double output_signal;
    double cutoff_frequency;  // Cutoff or center frequency in Hz
    double q_factor;          // Quality factor (resonance)
    double filter_gain;       // Filter gain
    bool is_enabled;
    
    // Vector of tubes used in the filter
    std::vector<std::unique_ptr<Tube>> filter_tubes;
    
    // Internal processing function
    virtual void ProcessSignal() = 0;
    
    static constexpr double MIN_CUTOFF_FREQ = 20.0;   // 20 Hz
    static constexpr double MAX_CUTOFF_FREQ = 20000.0; // 20 kHz
    static constexpr double MIN_Q_FACTOR = 0.1;       // Minimum Q
    static constexpr double MAX_Q_FACTOR = 100.0;     // Maximum Q
};

// Low-pass filter using tube circuits
class TubeLowPassFilter : public TubeFilter {
public:
    typedef TubeLowPassFilter CLASSNAME;

    TubeLowPassFilter();
    virtual ~TubeLowPassFilter() {}

    virtual String GetClassName() const override { return "TubeLowPassFilter"; }

protected:
    virtual void ProcessSignal() override;
    
    // Implementation based on tube RC or resonant circuits
    double CalculateLowPassResponse(double input, double cutoff, double q);
};

// High-pass filter using tube circuits
class TubeHighPassFilter : public TubeFilter {
public:
    typedef TubeHighPassFilter CLASSNAME;

    TubeHighPassFilter();
    virtual ~TubeHighPassFilter() {}

    virtual String GetClassName() const override { return "TubeHighPassFilter"; }

protected:
    virtual void ProcessSignal() override;
    
    // Implementation based on tube RC circuits
    double CalculateHighPassResponse(double input, double cutoff, double q);
};

// Band-pass filter using tube circuits
class TubeBandPassFilter : public TubeFilter {
public:
    typedef TubeBandPassFilter CLASSNAME;

    TubeBandPassFilter();
    virtual ~TubeBandPassFilter() {}

    virtual String GetClassName() const override { return "TubeBandPassFilter"; }

protected:
    virtual void ProcessSignal() override;
    
    // Implementation based on tube resonant circuits
    double CalculateBandPassResponse(double input, double center_freq, double q);
};

// Base class for tube-based oscillators
class TubeOscillator : public AnalogNodeBase {
public:
    typedef TubeOscillator CLASSNAME;

    TubeOscillator(TubeOscillatorType type = TubeOscillatorType::HARTLEY);
    virtual ~TubeOscillator();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeOscillator"; }

    // Get the output signal
    double GetOutputSignal() const { return output_signal; }
    
    // Set/get frequency
    void SetFrequency(double freq) { frequency = std::max(20.0, std::min(100000.0, freq)); }
    double GetFrequency() const { return frequency; }
    
    // Set/get amplitude
    void SetAmplitude(double amp) { amplitude = std::max(0.1, std::min(10.0, amp)); }
    double GetAmplitude() const { return amplitude; }
    
    // Set/get waveform type (for Wien Bridge which can produce multiple waveforms)
    void SetWaveformType(VCOType type) { waveform_type = type; }
    VCOType GetWaveformType() const { return waveform_type; }
    
    // Set oscillator type
    void SetOscillatorType(TubeOscillatorType type) { osc_type = type; }
    TubeOscillatorType GetOscillatorType() const { return osc_type; }
    
    // Enable/disable the oscillator
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }
    
    // Set/get feedback amount
    void SetFeedback(double fb) { feedback = std::max(0.0, std::min(2.0, fb)); }
    double GetFeedback() const { return feedback; }

protected:
    TubeOscillatorType osc_type;
    double output_signal;
    double frequency;          // Oscillation frequency in Hz
    double amplitude;          // Output amplitude
    VCOType waveform_type;     // For Wien bridge that can produce multiple waveforms
    bool is_enabled;
    double feedback;           // Feedback factor
    double phase;              // Current phase of oscillation
    
    // Vector of tubes used in the oscillator
    std::vector<std::unique_ptr<Tube>> osc_tubes;
    
    // Internal processing function
    virtual void ProcessSignal() = 0;
    
    static constexpr double MIN_FREQUENCY = 20.0;     // 20 Hz
    static constexpr double MAX_FREQUENCY = 100000.0; // 100 kHz
    static constexpr double MIN_AMPLITUDE = 0.1;      // Minimum amplitude
    static constexpr double MAX_AMPLITUDE = 10.0;     // Maximum amplitude
    static constexpr double TWO_PI = 2.0 * M_PI;
};

// Hartley Oscillator using tube
class TubeHartleyOscillator : public TubeOscillator {
public:
    typedef TubeHartleyOscillator CLASSNAME;

    TubeHartleyOscillator();
    virtual ~TubeHartleyOscillator() {}

    virtual String GetClassName() const override { return "TubeHartleyOscillator"; }

protected:
    virtual void ProcessSignal() override;
};

// Colpitts Oscillator using tube
class TubeColpittsOscillator : public TubeOscillator {
public:
    typedef TubeColpittsOscillator CLASSNAME;

    TubeColpittsOscillator();
    virtual ~TubeColpittsOscillator() {}

    virtual String GetClassName() const override { return "TubeColpittsOscillator"; }

protected:
    virtual void ProcessSignal() override;
};

// Wien Bridge Oscillator using tube
class TubeWienBridgeOscillator : public TubeOscillator {
public:
    typedef TubeWienBridgeOscillator CLASSNAME;

    TubeWienBridgeOscillator();
    virtual ~TubeWienBridgeOscillator() {}

    virtual String GetClassName() const override { return "TubeWienBridgeOscillator"; }

protected:
    virtual void ProcessSignal() override;
};

// Phase Shift Oscillator using tube
class TubePhaseShiftOscillator : public TubeOscillator {
public:
    typedef TubePhaseShiftOscillator CLASSNAME;

    TubePhaseShiftOscillator();
    virtual ~TubePhaseShiftOscillator() {}

    virtual String GetClassName() const override { return "TubePhaseShiftOscillator"; }

protected:
    virtual void ProcessSignal() override;
};

#endif