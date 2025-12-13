#include "AudioQaAnalysis.h"
#include "Common.h"  // Assuming Common.h contains utility functions
#include <cmath>
#include <algorithm>
#include <numeric>
#include <functional>
#include <limits>

namespace AudioQa {

AudioQaAnalysis::AudioQaAnalysis(size_t rate, size_t size) 
    : sample_rate(rate), fft_size(size) {
    
    // Allocate FFT input and output arrays
    fft_in = static_cast<double*>(fftw_malloc(sizeof(double) * fft_size));
    fft_out = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * (fft_size/2 + 1)));
    
    // Create FFTW plan for real-to-complex transform
    fft_plan = fftw_plan_dft_r2c_1d(fft_size, fft_in, fft_out, FFTW_MEASURE);
}

AudioQaAnalysis::~AudioQaAnalysis() {
    if (fft_plan) {
        fftw_destroy_plan(fft_plan);
    }
    if (fft_in) {
        fftw_free(fft_in);
    }
    if (fft_out) {
        fftw_free(fft_out);
    }
}

void AudioQaAnalysis::SetAudioData(const std::vector<float>& left, const std::vector<float>& right) {
    left_channel.resize(left.size());
    right_channel.resize(right.size());
    
    std::copy(left.begin(), left.end(), left_channel.begin());
    std::copy(right.begin(), right.end(), right_channel.begin());
}

void AudioQaAnalysis::SetAudioDataInterleaved(const std::vector<float>& interleaved_data, bool stereo) {
    if (stereo) {
        size_t frame_count = interleaved_data.size() / 2;
        left_channel.resize(frame_count);
        right_channel.resize(frame_count);
        
        for (size_t i = 0; i < frame_count; ++i) {
            left_channel[i] = interleaved_data[2 * i];
            right_channel[i] = interleaved_data[2 * i + 1];
        }
    } else {
        size_t sample_count = interleaved_data.size();
        left_channel.resize(sample_count);
        right_channel.resize(sample_count);  // Duplicate mono to both channels
        
        std::copy(interleaved_data.begin(), interleaved_data.end(), left_channel.begin());
        std::copy(interleaved_data.begin(), interleaved_data.end(), right_channel.begin());
    }
}

AudioQaReport AudioQaAnalysis::Analyze() {
    AudioQaReport report;
    report.sample_count = std::max(left_channel.size(), right_channel.size());
    report.duration_seconds = static_cast<double>(report.sample_count) / sample_rate;
    
    // Calculate all metrics
    if (!left_channel.empty()) {
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::RMSLevel, 
            CalculateRMSLevel(left_channel), 
            "Left channel RMS level"));
        
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::PeakLevel, 
            CalculatePeakLevel(left_channel), 
            "Left channel peak level"));
        
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::DCOffset, 
            CalculateDCOffset(left_channel), 
            "Left channel DC offset"));
            
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::HarmonicEnergy, 
            CalculateHarmonicEnergy(left_channel), 
            "Left channel harmonic energy"));
            
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::SilenceRatio, 
            CalculateSilenceRatio(left_channel), 
            "Left channel silence ratio"));
    }
    
    if (!right_channel.empty()) {
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::RMSLevel, 
            CalculateRMSLevel(right_channel), 
            "Right channel RMS level"));
            
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::PeakLevel, 
            CalculatePeakLevel(right_channel), 
            "Right channel peak level"));
            
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::DCOffset, 
            CalculateDCOffset(right_channel), 
            "Right channel DC offset"));
            
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::HarmonicEnergy, 
            CalculateHarmonicEnergy(right_channel), 
            "Right channel harmonic energy"));
            
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::SilenceRatio, 
            CalculateSilenceRatio(right_channel), 
            "Right channel silence ratio"));
    }
    
    if (!left_channel.empty() && !right_channel.empty()) {
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::StereoBalance, 
            CalculateStereoBalance(), 
            "Stereo balance"));
            
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::StereoWidth, 
            CalculateStereoWidth(), 
            "Stereo width"));
            
        report.AddMetric(AudioQaMetric(
            AudioQaMetricKind::PhaseCorrelation, 
            CalculatePhaseCorrelation(), 
            "Phase correlation"));
    }
    
    // If we have enough samples, calculate fundamental frequency
    if (std::max(left_channel.size(), right_channel.size()) > fft_size) {
        std::vector<double> ch;
        if (!left_channel.empty()) {
            ch = ExtractWindow(left_channel, 0, std::min(static_cast<size_t>(left_channel.size()), fft_size));
        } else if (!right_channel.empty()) {
            ch = ExtractWindow(right_channel, 0, std::min(static_cast<size_t>(right_channel.size()), fft_size));
        }
        
        if (!ch.empty()) {
            report.AddMetric(AudioQaMetric(
                AudioQaMetricKind::FundamentalFrequency, 
                CalculateFundamentalFrequency(ch), 
                "Fundamental frequency"));
        }
    }
    
    return report;
}

AudioQaReport AudioQaAnalysis::AnalyzeSegment(size_t start_sample, size_t end_sample) {
    // Create a temporary analyzer with just the segment
    AudioQaAnalysis temp_analyzer(sample_rate, fft_size);
    
    std::vector<double> left_seg, right_seg;
    if (start_sample < left_channel.size()) {
        size_t end = std::min(end_sample, left_channel.size());
        left_seg.assign(left_channel.begin() + start_sample, left_channel.begin() + end);
    }
    
    if (start_sample < right_channel.size()) {
        size_t end = std::min(end_sample, right_channel.size());
        right_seg.assign(right_channel.begin() + start_sample, right_channel.begin() + end);
    }
    
    temp_analyzer.SetAudioData(std::vector<float>(left_seg.begin(), left_seg.end()),
                              std::vector<float>(right_seg.begin(), right_seg.end()));
    
    return temp_analyzer.Analyze();
}

AudioQaReport AudioQaAnalysis::AnalyzeSlidingWindows(size_t window_size, size_t hop_size) {
    AudioQaReport combined_report;
    
    for (size_t pos = 0; pos + window_size <= std::max(left_channel.size(), right_channel.size()); pos += hop_size) {
        AudioQaReport segment_report = AnalyzeSegment(pos, pos + window_size);
        
        // For sliding window analysis, we might want to aggregate metrics differently
        // For now, we'll just add them all to the combined report
        for (const auto& metric : segment_report.metrics) {
            combined_report.AddMetric(metric);
        }
    }
    
    return combined_report;
}

double AudioQaAnalysis::CalculateRMSLevel(const std::vector<double>& channel_data) {
    if (channel_data.empty()) return 0.0;
    
    double sum_squares = 0.0;
    for (double sample : channel_data) {
        sum_squares += sample * sample;
    }
    
    double rms = std::sqrt(sum_squares / channel_data.size());
    // Convert to dBFS
    return 20.0 * std::log10(std::max(rms, 1e-9)); // Avoid log(0)
}

double AudioQaAnalysis::CalculatePeakLevel(const std::vector<double>& channel_data) {
    if (channel_data.empty()) return -std::numeric_limits<double>::infinity();
    
    double max_abs = 0.0;
    for (double sample : channel_data) {
        max_abs = std::max(max_abs, std::abs(sample));
    }
    
    // Convert to dBFS
    return 20.0 * std::log10(std::max(max_abs, 1e-9)); // Avoid log(0)
}

double AudioQaAnalysis::CalculateDCOffset(const std::vector<double>& channel_data) {
    if (channel_data.empty()) return 0.0;
    
    double sum = std::accumulate(channel_data.begin(), channel_data.end(), 0.0);
    return sum / channel_data.size();
}

double AudioQaAnalysis::CalculateStereoBalance() {
    if (left_channel.empty() || right_channel.empty()) return 0.0;
    
    size_t min_size = std::min(left_channel.size(), right_channel.size());
    if (min_size == 0) return 0.0;
    
    double left_rms = 0.0, right_rms = 0.0;
    for (size_t i = 0; i < min_size; ++i) {
        left_rms += left_channel[i] * left_channel[i];
        right_rms += right_channel[i] * right_channel[i];
    }
    
    left_rms = std::sqrt(left_rms / min_size);
    right_rms = std::sqrt(right_rms / min_size);
    
    // Stereo balance: -1 (full left) to 1 (full right), 0 is balanced
    if (left_rms == 0 && right_rms == 0) return 0.0;
    if (left_rms == 0) return 1.0;  // Full right
    if (right_rms == 0) return -1.0;  // Full left
    
    double total = left_rms + right_rms;
    return (right_rms - left_rms) / total;
}

double AudioQaAnalysis::CalculateStereoWidth() {
    if (left_channel.empty() || right_channel.empty()) return 0.0;
    
    size_t min_size = std::min(left_channel.size(), right_channel.size());
    if (min_size == 0) return 0.0;
    
    double mid_power = 0.0, side_power = 0.0;
    for (size_t i = 0; i < min_size; ++i) {
        double mid = (left_channel[i] + right_channel[i]) / 2.0;  // Mid signal
        double side = (left_channel[i] - right_channel[i]) / 2.0; // Side signal
        
        mid_power += mid * mid;
        side_power += side * side;
    }
    
    mid_power /= min_size;
    side_power /= min_size;
    
    // Stereo width: 0 (mono) to 2 (wide stereo)
    if (mid_power == 0) return 2.0; // Fully separated
    return 2.0 * side_power / (mid_power + side_power);
}

double AudioQaAnalysis::CalculatePhaseCorrelation() {
    if (left_channel.empty() || right_channel.empty()) return 0.0;
    
    size_t min_size = std::min(left_channel.size(), right_channel.size());
    if (min_size == 0) return 0.0;
    
    double numerator = 0.0, left_denom = 0.0, right_denom = 0.0;
    for (size_t i = 0; i < min_size; ++i) {
        numerator += left_channel[i] * right_channel[i];
        left_denom += left_channel[i] * left_channel[i];
        right_denom += right_channel[i] * right_channel[i];
    }
    
    if (left_denom == 0 || right_denom == 0) return 0.0;
    
    double denominator = std::sqrt(left_denom * right_denom);
    return numerator / denominator;  // Correlation coefficient (-1 to 1)
}

double AudioQaAnalysis::CalculateFundamentalFrequency(const std::vector<double>& channel_data) {
    if (channel_data.size() < fft_size) {
        return 0.0;  // Not enough data for analysis
    }
    
    // For fundamental frequency estimation, we'll use a smaller segment
    std::vector<double> segment = ExtractWindow(channel_data, 0, std::min(fft_size, channel_data.size()));
    
    // Compute FFT
    std::vector<std::complex<double>> fft_result = ComputeFFT(segment);
    
    // Estimate fundamental frequency from FFT
    return EstimateFundamentalFrequency(fft_result);
}

double AudioQaAnalysis::CalculateHarmonicEnergy(const std::vector<double>& channel_data) {
    if (channel_data.size() < fft_size) {
        return 0.0;  // Not enough data for analysis
    }
    
    std::vector<double> segment = ExtractWindow(channel_data, 0, std::min(fft_size, channel_data.size()));
    std::vector<std::complex<double>> fft_result = ComputeFFT(segment);
    
    double total_power = 0.0, harmonic_power = 0.0;
    for (const auto& bin : fft_result) {
        double power = std::norm(bin);
        total_power += power;
        
        // For simplicity, consider frequencies in the typical harmonic range as "harmonic power"
        // A more sophisticated implementation would identify harmonic peaks
        size_t bin_idx = &bin - &fft_result[0];
        double freq = static_cast<double>(bin_idx) * sample_rate / fft_size;
        
        // Consider frequencies in the range typically associated with harmonics
        if (freq > 50.0 && freq < 4000.0) {
            harmonic_power += power;
        }
    }
    
    if (total_power == 0.0) return 0.0;
    return harmonic_power / total_power;
}

double AudioQaAnalysis::CalculateSilenceRatio(const std::vector<double>& channel_data, double threshold_db) {
    if (channel_data.empty()) return 1.0; // All silent if empty
    
    double threshold_lin = std::pow(10.0, threshold_db / 20.0);
    size_t silent_samples = 0;
    
    for (double sample : channel_data) {
        if (std::abs(sample) < threshold_lin) {
            silent_samples++;
        }
    }
    
    return static_cast<double>(silent_samples) / channel_data.size();
}

std::vector<std::complex<double>> AudioQaAnalysis::ComputeFFT(const std::vector<double>& time_data) {
    size_t n = std::min(time_data.size(), fft_size);
    
    // Copy data to FFT input buffer
    for (size_t i = 0; i < n; ++i) {
        fft_in[i] = time_data[i];
    }
    // Zero-pad if necessary
    for (size_t i = n; i < fft_size; ++i) {
        fft_in[i] = 0.0;
    }
    
    // Execute the FFT
    fftw_execute(fft_plan);
    
    // Copy FFT output to result vector
    std::vector<std::complex<double>> result(n/2 + 1);
    for (size_t i = 0; i < n/2 + 1; ++i) {
        result[i] = std::complex<double>(fft_out[i][0], fft_out[i][1]);
    }
    
    return result;
}

double AudioQaAnalysis::EstimateFundamentalFrequency(const std::vector<std::complex<double>>& fft_result) {
    if (fft_result.empty()) return 0.0;
    
    // Find the peak magnitude in the spectrum
    double max_magnitude = 0.0;
    size_t peak_bin = 0;
    
    for (size_t i = 1; i < fft_result.size(); ++i) { // Skip DC bin at index 0
        double magnitude = std::abs(fft_result[i]);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            peak_bin = i;
        }
    }
    
    // Convert bin index to frequency
    double freq = static_cast<double>(peak_bin) * sample_rate / fft_size;
    
    // Apply a simple harmonic product spectrum-like approach to refine estimate
    // This looks for potential sub-harmonics of the peak
    for (size_t harmonic = 2; harmonic <= 8; ++harmonic) {
        size_t sub_harmonic_bin = peak_bin / harmonic;
        if (sub_harmonic_bin > 0 && sub_harmonic_bin < fft_result.size()) {
            double candidate_freq = static_cast<double>(sub_harmonic_bin) * sample_rate / fft_size;
            // For now, just return the original estimate - a full implementation would be more complex
        }
    }
    
    return freq;
}

std::vector<double> AudioQaAnalysis::ExtractWindow(const std::vector<double>& data, size_t start, size_t size) {
    size_t end = std::min(start + size, data.size());
    if (start >= data.size()) return std::vector<double>();
    
    return std::vector<double>(data.begin() + start, data.begin() + end);
}

}  // namespace AudioQa