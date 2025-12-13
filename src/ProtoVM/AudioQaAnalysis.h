#ifndef AUDIOQAANALYSIS_H
#define AUDIOQAANALYSIS_H

#include "AudioQa.h"
#include <vector>
#include <complex>
#include <fftw3.h>

namespace AudioQa {

// Analysis engine for computing audio metrics
class AudioQaAnalysis {
private:
    std::vector<double> left_channel;
    std::vector<double> right_channel;
    size_t sample_rate;
    size_t fft_size;
    fftw_plan fft_plan;
    fftw_complex* fft_out;
    double* fft_in;

public:
    AudioQaAnalysis(size_t rate = 48000, size_t size = 4096);
    ~AudioQaAnalysis();
    
    void SetAudioData(const std::vector<float>& left, const std::vector<float>& right);
    void SetAudioDataInterleaved(const std::vector<float>& interleaved_data, bool stereo = true);
    
    AudioQaReport Analyze();
    AudioQaReport AnalyzeSegment(size_t start_sample, size_t end_sample);
    AudioQaReport AnalyzeSlidingWindows(size_t window_size, size_t hop_size);
    
    // Individual metric calculations
    double CalculateRMSLevel(const std::vector<double>& channel_data);
    double CalculatePeakLevel(const std::vector<double>& channel_data);
    double CalculateDCOffset(const std::vector<double>& channel_data);
    double CalculateStereoBalance();
    double CalculateStereoWidth();
    double CalculatePhaseCorrelation();
    double CalculateFundamentalFrequency(const std::vector<double>& channel_data);
    double CalculateHarmonicEnergy(const std::vector<double>& channel_data);
    double CalculateSilenceRatio(const std::vector<double>& channel_data, double threshold = -60.0);
    
private:
    std::vector<std::complex<double>> ComputeFFT(const std::vector<double>& time_data);
    double EstimateFundamentalFrequency(const std::vector<std::complex<double>>& fft_result);
    std::vector<double> ExtractWindow(const std::vector<double>& data, size_t start, size_t size);
};

}  // namespace AudioQa

#endif // AUDIOQAANALYSIS_H