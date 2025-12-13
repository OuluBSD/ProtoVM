#ifndef AUDIQA_H
#define AUDIQA_H

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <complex>
#include <fftw3.h>

// Enum for different types of audio quality metrics
enum class AudioQaMetricKind {
    RMSLevel,
    PeakLevel,
    DCOffset,
    StereoBalance,
    StereoWidth,
    PhaseCorrelation,
    FundamentalFrequency,
    HarmonicEnergy,
    SilenceRatio
};

// Structure to hold a single metric measurement
struct AudioQaMetric {
    AudioQaMetricKind kind;
    double value;
    std::string description;
    
    AudioQaMetric(AudioQaMetricKind k, double v, const std::string& desc = "") 
        : kind(k), value(v), description(desc) {}
};

// Report containing all metrics from an analysis
struct AudioQaReport {
    std::vector<AudioQaMetric> metrics;
    std::string analysis_timestamp;
    uint64_t sample_count;
    double duration_seconds;
    
    AudioQaReport() : sample_count(0), duration_seconds(0.0) {}
    
    void AddMetric(const AudioQaMetric& metric) {
        metrics.push_back(metric);
    }
    
    AudioQaMetric* FindMetric(AudioQaMetricKind kind) {
        for (auto& metric : metrics) {
            if (metric.kind == kind) {
                return &metric;
            }
        }
        return nullptr;
    }
};

// Threshold profile for comparing metrics against acceptable values
struct AudioQaThresholdProfile {
    std::map<AudioQaMetricKind, std::pair<double, double>> thresholds; // min and max acceptable values
    
    void SetThreshold(AudioQaMetricKind kind, double min_val, double max_val) {
        thresholds[kind] = std::make_pair(min_val, max_val);
    }
    
    std::pair<bool, std::string> CheckThreshold(AudioQaMetricKind kind, double value) const {
        auto it = thresholds.find(kind);
        if (it != thresholds.end()) {
            double min_val = it->second.first;
            double max_val = it->second.second;
            
            if (value < min_val || value > max_val) {
                return {false, "Value " + std::to_string(value) + " outside threshold range [" + 
                               std::to_string(min_val) + ", " + std::to_string(max_val) + "]"};
            }
            return {true, ""};
        }
        return {true, "No threshold defined for this metric"};
    }
};

#endif // AUDIQA_H