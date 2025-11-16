#ifndef _ProtoVM_Oversampling_h_
#define _ProtoVM_Oversampling_h_

#include "AnalogCommon.h"
#include "AudioSignalPath.h"
#include <vector>
#include <memory>

// Enum for different oversampling factors
enum class OversamplingFactor {
    NONE = 1,
    X2 = 2,
    X4 = 4,
    X8 = 8,
    X16 = 16
};

// Enum for different anti-aliasing filter types
enum class FilterType {
    NEAREST,
    LINEAR,
    CUBIC,
    BUTTERWORTH,
    CHEBYSHEV,
    CUSTOM
};

// Base class for oversampling processors
class OversamplingProcessor {
public:
    OversamplingProcessor(OversamplingFactor factor = OversamplingFactor::X4, 
                         FilterType filter_type = FilterType::BUTTERWORTH,
                         double input_sr = 44100.0);
    virtual ~OversamplingProcessor();

    // Process a single sample
    virtual double ProcessSample(double input) = 0;

    // Process a buffer of samples
    virtual std::vector<double> ProcessBuffer(const std::vector<double>& input) = 0;

    // Get/set oversampling factor
    void SetFactor(OversamplingFactor factor);
    OversamplingFactor GetFactor() const { return factor; }
    int GetFactorValue() const { return static_cast<int>(factor); }

    // Get/set filter type
    void SetFilterType(FilterType type);
    FilterType GetFilterType() const { return filter_type; }

    // Get/set sample rates
    void SetInputSampleRate(double rate) { input_sample_rate = rate; }
    double GetInputSampleRate() const { return input_sample_rate; }
    
    void SetOutputSampleRate(double rate) { output_sample_rate = rate; }
    double GetOutputSampleRate() const { return output_sample_rate; }

    // Reset internal state
    virtual void Reset();

    // Get latency introduced by oversampling (in samples at input rate)
    virtual int GetLatency() const { return latency; }

protected:
    OversamplingFactor factor;
    FilterType filter_type;
    double input_sample_rate;
    double output_sample_rate;
    int latency;
    
    // Internal processing buffers
    std::vector<double> upsampled_buffer;
    std::vector<double> filtered_buffer;
};

// Upsampling processor with anti-aliasing filter
class Upsampler : public OversamplingProcessor {
public:
    Upsampler(OversamplingFactor factor = OversamplingFactor::X4, 
             FilterType filter_type = FilterType::BUTTERWORTH,
             double input_sr = 44100.0);
    virtual ~Upsampler();

    virtual double ProcessSample(double input) override;
    virtual std::vector<double> ProcessBuffer(const std::vector<double>& input) override;

    // Upsample a buffer by the specified factor
    std::vector<double> UpsampleBuffer(const std::vector<double>& input);

    // Apply anti-aliasing filter
    void ApplyAntiAliasingFilter(std::vector<double>& signal);

private:
    // FIR filter coefficients for anti-aliasing
    std::vector<double> filter_coeffs;
    
    // Filter state (for IIR filters)
    std::vector<double> filter_state;
    
    // Input buffer for processing
    std::vector<double> input_delay_line;
    
    // Initialize filter coefficients based on filter type
    void InitializeFilter();
};

// Downsampling processor with decimation filter
class Downsampler : public OversamplingProcessor {
public:
    Downsampler(OversamplingFactor factor = OversamplingFactor::X4, 
               FilterType filter_type = FilterType::BUTTERWORTH,
               double output_sr = 44100.0);
    virtual ~Downsampler();

    virtual double ProcessSample(double input) override;
    virtual std::vector<double> ProcessBuffer(const std::vector<double>& input) override;

    // Downsample a buffer by the specified factor
    std::vector<double> DownsampleBuffer(const std::vector<double>& input);

    // Apply decimation filter
    void ApplyDecimationFilter(std::vector<double>& signal);

private:
    // FIR filter coefficients for decimation
    std::vector<double> filter_coeffs;
    
    // Filter state (for IIR filters)
    std::vector<double> filter_state;
    
    // Input buffer for processing
    std::vector<double> input_delay_line;
    
    // Initialize filter coefficients based on filter type
    void InitializeFilter();
};

// Full oversampling processor with both upsampling and downsampling
class FullOversamplingProcessor : public OversamplingProcessor {
public:
    FullOversamplingProcessor(OversamplingFactor factor = OversamplingFactor::X4, 
                            FilterType filter_type = FilterType::BUTTERWORTH,
                            double input_sr = 44100.0);
    virtual ~FullOversamplingProcessor();

    virtual double ProcessSample(double input) override;
    virtual std::vector<double> ProcessBuffer(const std::vector<double>& input) override;

    // Process with oversampling - upsample, process, downsample
    double ProcessWithOversampling(double input, 
                                  std::function<double(double)> process_callback);
    
    std::vector<double> ProcessBufferWithOversampling(const std::vector<double>& input,
                                                     std::function<std::vector<double>(const std::vector<double>&)> process_callback);

private:
    std::unique_ptr<Upsampler> upsampler;
    std::unique_ptr<Downsampler> downsampler;
    
    // Internal processing buffers
    std::vector<double> temp_buffer;
};

// Utility functions for oversampling
namespace OversamplingUtils {
    // Generate FIR filter coefficients for anti-aliasing
    std::vector<double> GenerateFIRFilterCoeffs(FilterType type, int order, 
                                               double cutoff_freq, double sample_rate);
    
    // Apply FIR filter to a signal
    void ApplyFIRFilter(std::vector<double>& signal, const std::vector<double>& coeffs);
    
    // Apply IIR filter to a signal
    void ApplyIIRFilter(std::vector<double>& signal, const std::vector<double>& b_coeffs,
                       const std::vector<double>& a_coeffs);
    
    // Generate test tones for oversampling quality testing
    std::vector<double> GenerateTestTone(double frequency, double sample_rate, 
                                        int duration_samples, double amplitude = 1.0);
    
    // Check for aliasing in a signal
    double MeasureAliasing(const std::vector<double>& signal, double sample_rate);
}

// Base class for audio effects that use oversampling
class OversampledEffect : public TimeVaryingEffect {
public:
    typedef OversampledEffect CLASSNAME;
    
    OversampledEffect(const std::string& name = "OversampledEffect",
                     OversamplingFactor factor = OversamplingFactor::X4);
    virtual ~OversampledEffect();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "OversampledEffect"; }

    // Set/get oversampling factor
    void SetOversamplingFactor(OversamplingFactor factor);
    OversamplingFactor GetOversamplingFactor() const { return oversampling_processor->GetFactor(); }

    // Set/get filter type
    void SetFilterType(FilterType type);
    FilterType GetFilterType() const { return oversampling_processor->GetFilterType(); }

    // Process the effect with oversampling - to be implemented by derived classes
    virtual double ProcessSampleWithOversampling(double input) = 0;

    // Get latency introduced by oversampling
    int GetOversamplingLatency() const { return oversampling_processor->GetLatency(); }

protected:
    std::unique_ptr<FullOversamplingProcessor> oversampling_processor;

private:
    OversamplingFactor oversampling_factor;
    FilterType filter_type;
};

#endif