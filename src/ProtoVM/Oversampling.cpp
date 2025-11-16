#include "Oversampling.h"
#include <cmath>
#include <algorithm>
#include <functional>

// OversamplingProcessor implementation
OversamplingProcessor::OversamplingProcessor(OversamplingFactor factor, 
                                           FilterType filter_type, 
                                           double input_sr)
    : factor(factor), filter_type(filter_type), input_sample_rate(input_sr),
      output_sample_rate(input_sr * static_cast<int>(factor)), latency(0) {
}

OversamplingProcessor::~OversamplingProcessor() {
}

void OversamplingProcessor::SetFactor(OversamplingFactor f) {
    factor = f;
    output_sample_rate = input_sample_rate * static_cast<int>(factor);
}

void OversamplingProcessor::SetFilterType(FilterType type) {
    filter_type = type;
}

void OversamplingProcessor::Reset() {
    latency = 0;
    upsampled_buffer.clear();
    filtered_buffer.clear();
}

// Upsampler implementation
Upsampler::Upsampler(OversamplingFactor factor, FilterType filter_type, double input_sr)
    : OversamplingProcessor(factor, filter_type, input_sr) {
    InitializeFilter();
    
    // Initialize delay line
    int factor_val = GetFactorValue();
    input_delay_line.resize(factor_val, 0.0);
}

Upsampler::~Upsampler() {
}

double Upsampler::ProcessSample(double input) {
    // This is a simplified implementation
    // In a real implementation, we would handle the sample rate conversion properly
    return input; // Placeholder
}

std::vector<double> Upsampler::ProcessBuffer(const std::vector<double>& input) {
    std::vector<double> upsampled = UpsampleBuffer(input);
    ApplyAntiAliasingFilter(upsampled);
    return upsampled;
}

std::vector<double> Upsampler::UpsampleBuffer(const std::vector<double>& input) {
    int factor_val = GetFactorValue();
    std::vector<double> output(input.size() * factor_val, 0.0);
    
    // Zero-stuffing: insert zeros between samples
    for (size_t i = 0; i < input.size(); i++) {
        output[i * factor_val] = input[i];
    }
    
    return output;
}

void Upsampler::ApplyAntiAliasingFilter(std::vector<double>& signal) {
    // Apply the filter to remove imaging artifacts introduced by upsampling
    if (filter_coeffs.empty()) return;
    
    // Simple FIR filtering implementation
    std::vector<double> temp = signal;
    
    for (size_t i = 0; i < signal.size(); i++) {
        double sum = 0.0;
        for (size_t j = 0; j < filter_coeffs.size(); j++) {
            int src_idx = static_cast<int>(i) - static_cast<int>(j) + static_cast<int>(filter_coeffs.size()/2);
            if (src_idx >= 0 && src_idx < static_cast<int>(signal.size())) {
                sum += temp[src_idx] * filter_coeffs[j];
            }
        }
        signal[i] = sum;
    }
}

void Upsampler::InitializeFilter() {
    int factor_val = GetFactorValue();
    
    // Calculate cutoff frequency (conservative to avoid aliasing)
    double cutoff = (input_sample_rate / 2.0) * 0.9; // 90% of Nyquist to be safe
    
    filter_coeffs = OversamplingUtils::GenerateFIRFilterCoeffs(filter_type, 64, cutoff, output_sample_rate);
    
    // Initialize filter state
    filter_state.resize(filter_coeffs.size(), 0.0);
}

// Downsampler implementation
Downsampler::Downsampler(OversamplingFactor factor, FilterType filter_type, double output_sr)
    : OversamplingProcessor(factor, filter_type, output_sr / static_cast<int>(factor)) {
    output_sample_rate = output_sr;
    InitializeFilter();
    
    // Initialize delay line
    int factor_val = GetFactorValue();
    input_delay_line.resize(filter_coeffs.size(), 0.0);
}

Downsampler::~Downsampler() {
}

double Downsampler::ProcessSample(double input) {
    // This is a simplified implementation
    // In a real implementation, we would handle the sample rate conversion properly
    return input; // Placeholder
}

std::vector<double> Downsampler::ProcessBuffer(const std::vector<double>& input) {
    ApplyDecimationFilter(input);
    return DownsampleBuffer(input);
}

std::vector<double> Downsampler::DownsampleBuffer(const std::vector<double>& input) {
    int factor_val = GetFactorValue();
    std::vector<double> output(input.size() / factor_val);
    
    // Decimate: keep every Nth sample
    for (size_t i = 0; i < output.size(); i++) {
        output[i] = input[i * factor_val];
    }
    
    return output;
}

void Downsampler::ApplyDecimationFilter(std::vector<double>& signal) {
    // Apply low-pass filter to prevent aliasing before downsampling
    if (filter_coeffs.empty()) return;
    
    // Simple FIR filtering implementation
    std::vector<double> temp = signal;
    
    for (size_t i = 0; i < signal.size(); i++) {
        double sum = 0.0;
        for (size_t j = 0; j < filter_coeffs.size(); j++) {
            int src_idx = static_cast<int>(i) - static_cast<int>(j) + static_cast<int>(filter_coeffs.size()/2);
            if (src_idx >= 0 && src_idx < static_cast<int>(signal.size())) {
                sum += temp[src_idx] * filter_coeffs[j];
            }
        }
        signal[i] = sum;
    }
}

void Downsampler::InitializeFilter() {
    int factor_val = GetFactorValue();
    
    // Calculate cutoff frequency (before downsampling to avoid aliasing)
    double cutoff = (output_sample_rate / 2.0) * 0.9; // 90% of Nyquist to be safe
    
    filter_coeffs = OversamplingUtils::GenerateFIRFilterCoeffs(filter_type, 64, cutoff, input_sample_rate);
    
    // Initialize filter state
    filter_state.resize(filter_coeffs.size(), 0.0);
}

// FullOversamplingProcessor implementation
FullOversamplingProcessor::FullOversamplingProcessor(OversamplingFactor factor, 
                                                   FilterType filter_type, 
                                                   double input_sr)
    : OversamplingProcessor(factor, filter_type, input_sr) {
    upsampler = std::make_unique<Upsampler>(factor, filter_type, input_sr);
    downsampler = std::make_unique<Downsampler>(factor, filter_type, input_sr * static_cast<int>(factor));
}

FullOversamplingProcessor::~FullOversamplingProcessor() {
}

double FullOversamplingProcessor::ProcessSample(double input) {
    // This is a simplified implementation
    // In a real implementation, we would properly handle the sample rate conversion
    return input; // Placeholder
}

std::vector<double> FullOversamplingProcessor::ProcessBuffer(const std::vector<double>& input) {
    // Upsample
    std::vector<double> upsampled = upsampler->ProcessBuffer(input);
    
    // Process at high sample rate (in a real implementation, this would be where
    // non-linear processing occurs that benefits from oversampling)
    
    // Downsample
    std::vector<double> downsampled = downsampler->ProcessBuffer(upsampled);
    
    return downsampled;
}

double FullOversamplingProcessor::ProcessWithOversampling(double input, 
                                                         std::function<double(double)> process_callback) {
    // In a real implementation, this would perform the full upsample->process->downsample cycle
    // For now, we'll just apply the process callback directly
    return process_callback(input);
}

std::vector<double> FullOversamplingProcessor::ProcessBufferWithOversampling(
    const std::vector<double>& input,
    std::function<std::vector<double>(const std::vector<double>&)> process_callback) {
    
    // Upsample
    std::vector<double> upsampled = upsampler->ProcessBuffer(input);
    
    // Process at high sample rate
    std::vector<double> processed = process_callback(upsampled);
    
    // Downsample
    std::vector<double> result = downsampler->ProcessBuffer(processed);
    
    return result;
}

// OversamplingUtils implementation
namespace OversamplingUtils {
    std::vector<double> GenerateFIRFilterCoeffs(FilterType type, int order, 
                                               double cutoff_freq, double sample_rate) {
        std::vector<double> coeffs(order, 0.0);
        
        // Calculate normalized frequency
        double nyquist = sample_rate / 2.0;
        double norm_freq = cutoff_freq / nyquist;
        
        // Clamp normalized frequency to valid range
        norm_freq = std::max(0.0, std::min(norm_freq, 1.0));
        
        switch (type) {
            case FilterType::NEAREST:
                // Simple 1-tap filter
                coeffs[0] = 1.0;
                break;
                
            case FilterType::LINEAR:
                // Simple 2-tap linear interpolation
                coeffs.resize(2);
                coeffs[0] = 0.5;
                coeffs[1] = 0.5;
                break;
                
            case FilterType::CUBIC:
                // 4-tap cubic interpolation
                coeffs.resize(4);
                coeffs[0] = -0.5 * norm_freq;
                coeffs[1] = 1.5 * norm_freq;
                coeffs[2] = -1.5 * norm_freq;
                coeffs[3] = 0.5 * norm_freq;
                break;
                
            case FilterType::BUTTERWORTH:
            case FilterType::CHEBYSHEV:
                // Generate windowed-sinc filter (a common approach for anti-aliasing)
                // Using a Hamming window
                for (int n = 0; n < order; n++) {
                    int center = order / 2;
                    int idx = n - center;
                    
                    // Avoid division by zero
                    if (idx == 0) {
                        coeffs[n] = 2.0 * M_PI * norm_freq;
                    } else {
                        coeffs[n] = sin(2.0 * M_PI * norm_freq * idx) / idx;
                    }
                    
                    // Apply Hamming window
                    double window = 0.54 - 0.46 * cos(2.0 * M_PI * n / (order - 1));
                    coeffs[n] *= window;
                }
                
                // Normalize to unit DC gain
                double sum = 0.0;
                for (double c : coeffs) {
                    sum += c;
                }
                for (double& c : coeffs) {
                    c /= sum;
                }
                break;
                
            default:
                // For custom or other types, default to a simple windowed sinc
                for (int n = 0; n < order; n++) {
                    int center = order / 2;
                    int idx = n - center;
                    
                    // Avoid division by zero
                    if (idx == 0) {
                        coeffs[n] = 2.0 * M_PI * norm_freq;
                    } else {
                        coeffs[n] = sin(2.0 * M_PI * norm_freq * idx) / idx;
                    }
                    
                    // Apply Hamming window
                    double window = 0.54 - 0.46 * cos(2.0 * M_PI * n / (order - 1));
                    coeffs[n] *= window;
                }
                
                // Normalize to unit DC gain
                double sum = 0.0;
                for (double c : coeffs) {
                    sum += c;
                }
                for (double& c : coeffs) {
                    c /= sum;
                }
                break;
        }
        
        return coeffs;
    }
    
    void ApplyFIRFilter(std::vector<double>& signal, const std::vector<double>& coeffs) {
        if (coeffs.empty()) return;
        
        std::vector<double> temp = signal;
        
        for (size_t i = 0; i < signal.size(); i++) {
            double sum = 0.0;
            for (size_t j = 0; j < coeffs.size(); j++) {
                int src_idx = static_cast<int>(i) - static_cast<int>(j) + static_cast<int>(coeffs.size()/2);
                if (src_idx >= 0 && src_idx < static_cast<int>(signal.size())) {
                    sum += temp[src_idx] * coeffs[j];
                }
            }
            signal[i] = sum;
        }
    }
    
    void ApplyIIRFilter(std::vector<double>& signal, const std::vector<double>& b_coeffs,
                       const std::vector<double>& a_coeffs) {
        if (b_coeffs.empty()) return;
        
        std::vector<double> temp = signal;
        std::vector<double> output(signal.size(), 0.0);
        
        // Direct form II implementation
        for (size_t i = 0; i < signal.size(); i++) {
            output[i] = 0.0;
            
            // Feedforward part
            for (size_t j = 0; j < b_coeffs.size(); j++) {
                int src_idx = static_cast<int>(i) - static_cast<int>(j);
                if (src_idx >= 0 && src_idx < static_cast<int>(signal.size())) {
                    output[i] += temp[src_idx] * b_coeffs[j];
                }
            }
            
            // Feedback part
            for (size_t j = 1; j < a_coeffs.size(); j++) {
                int out_idx = static_cast<int>(i) - static_cast<int>(j);
                if (out_idx >= 0) {
                    output[i] -= output[out_idx] * a_coeffs[j];
                }
            }
        }
        
        signal = output;
    }
    
    std::vector<double> GenerateTestTone(double frequency, double sample_rate, 
                                        int duration_samples, double amplitude) {
        std::vector<double> tone(duration_samples);
        
        for (int i = 0; i < duration_samples; i++) {
            double time = static_cast<double>(i) / sample_rate;
            tone[i] = amplitude * sin(2.0 * M_PI * frequency * time);
        }
        
        return tone;
    }
    
    double MeasureAliasing(const std::vector<double>& signal, double sample_rate) {
        // A basic aliasing measurement by looking for frequencies 
        // that exceed the Nyquist frequency
        
        if (signal.size() < 2) return 0.0;
        
        // Calculate the fundamental frequency (assuming it's a periodic signal)
        // This is a simplified approach
        double max_val = 0.0, min_val = 0.0;
        for (double val : signal) {
            if (val > max_val) max_val = val;
            if (val < min_val) min_val = val;
        }
        
        // The actual aliasing measurement would require FFT analysis
        // For this implementation, we'll return a dummy value
        // In a real implementation, this would be more sophisticated
        return 0.0; // Placeholder
    }
}

// OversampledEffect implementation
OversampledEffect::OversampledEffect(const std::string& name, OversamplingFactor factor)
    : TimeVaryingEffect(name), oversampling_factor(factor), filter_type(FilterType::BUTTERWORTH) {
    oversampling_processor = std::make_unique<FullOversamplingProcessor>(factor, filter_type, 44100.0);
}

OversampledEffect::~OversampledEffect() {
}

bool OversampledEffect::Tick() {
    // Get input from analog_values[0]
    double input = analog_values[0];
    
    // Process the effect if not bypassed
    double output;
    if (IsBypassed()) {
        output = input;  // Pass through if bypassed
    } else {
        output = ProcessSampleWithOversampling(input);
    }
    
    // Update output in analog_values[1]
    analog_values[1] = output;
    UpdateAnalogValue(0, input);   // Update input
    UpdateAnalogValue(1, output);  // Update output
    
    // Process automation for the current time
    GetAutomator().ProcessAutomation(simulation_time);
    
    return true;
}

void OversampledEffect::SetOversamplingFactor(OversamplingFactor factor) {
    oversampling_factor = factor;
    oversampling_processor = std::make_unique<FullOversamplingProcessor>(factor, filter_type, 44100.0);
}

void OversampledEffect::SetFilterType(FilterType type) {
    filter_type = type;
    oversampling_processor = std::make_unique<FullOversamplingProcessor>(oversampling_factor, type, 44100.0);
}
