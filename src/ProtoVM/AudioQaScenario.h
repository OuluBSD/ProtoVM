#ifndef AUDIOQASCENARIO_H
#define AUDIOQASCENARIO_H

#include "AudioQa.h"
#include "AudioQaAnalysis.h"
#include <functional>
#include <vector>
#include <string>

namespace AudioQa {

// Represents a golden audio scenario with expected metrics
struct AudioQaScenario {
    std::string name;
    std::string description;
    AudioQaThresholdProfile expected_thresholds;
    std::function<void()> setup_function;  // Function to set up the test scenario
    std::function<std::pair<std::vector<float>, std::vector<float>>() > render_function;  // Function that renders audio for this scenario
    
    AudioQaScenario(const std::string& n, const std::string& desc) 
        : name(n), description(desc) {}
    
    AudioQaReport Execute() {
        if (setup_function) {
            setup_function();
        }
        
        if (render_function) {
            auto audio_data = render_function();
            AudioQaAnalysis analyzer;
            analyzer.SetAudioData(audio_data.first, audio_data.second);
            return analyzer.Analyze();
        }
        
        return AudioQaReport();
    }
    
    bool Validate(const AudioQaReport& report) {
        for (const auto& metric : report.metrics) {
            auto check_result = expected_thresholds.CheckThreshold(metric.kind, metric.value);
            if (!check_result.first) {
                return false;  // Validation failed
            }
        }
        return true;  // All metrics passed validation
    }
};

// Predefined audio scenarios
class AudioQaScenarioCatalog {
public:
    static std::vector<std::unique_ptr<AudioQaScenario>> GetGoldenScenarios() {
        std::vector<std::unique_ptr<AudioQaScenario>> scenarios;
        
        // 440 Hz sine oscillator scenario
        auto sine_scenario = std::make_unique<AudioQaScenario>(
            "440Hz_Sine_Oscillator", 
            "A pure 440Hz sine wave with expected characteristics");
        
        sine_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::FundamentalFrequency, 435.0, 445.0);  // Allow ±5Hz tolerance
        sine_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::DCOffset, -0.01, 0.01);  // Small DC offset allowed
        sine_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::HarmonicEnergy, 0.95, 1.0);  // Pure sine should have little harmonics
        
        scenarios.push_back(std::move(sine_scenario));
        
        // Pan-LFO test scenario
        auto pan_lfo_scenario = std::make_unique<AudioQaScenario>(
            "Pan_LFO_Test", 
            "Stereo panning LFO modulating from hard left to hard right");
        
        pan_lfo_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::StereoWidth, 0.8, 1.2);  // Expect wide stereo
        // Would need to test over time to validate panning modulation
        
        scenarios.push_back(std::move(pan_lfo_scenario));
        
        // Silence sanity scenario
        auto silence_scenario = std::make_unique<AudioQaScenario>(
            "Silence_Sanity", 
            "Near-silent signal with low RMS");
        
        // Expect very low RMS levels
        silence_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::RMSLevel, -70.0, -60.0);  // Very quiet but not zero
        silence_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::SilenceRatio, 0.9, 1.0);  // Mostly silent
        
        scenarios.push_back(std::move(silence_scenario));
        
        // Square wave harmonic test
        auto square_scenario = std::make_unique<AudioQaScenario>(
            "Square_Wave_Test", 
            "Square wave with odd harmonics");
        
        square_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::HarmonicEnergy, 0.8, 1.0);  // Square waves have significant harmonics
        square_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::DCOffset, -0.1, 0.1);  // Square waves might have slight DC offset
        
        scenarios.push_back(std::move(square_scenario));
        
        // White noise test
        auto noise_scenario = std::make_unique<AudioQaScenario>(
            "White_Noise_Test", 
            "Flat frequency spectrum white noise");
        
        noise_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::PhaseCorrelation, -0.1, 0.1);  // No correlation between channels if truly random
        noise_scenario->expected_thresholds.SetThreshold(
            AudioQaMetricKind::FundamentalFrequency, 0.0, 10.0);  // No dominant frequency
        
        scenarios.push_back(std::move(noise_scenario));
        
        return scenarios;
    }
    
    // Helper method to run all scenarios
    static std::vector<std::pair<std::string, bool>> RunAllScenarios() {
        std::vector<std::pair<std::string, bool>> results;
        auto scenarios = GetGoldenScenarios();
        
        for (auto& scenario : scenarios) {
            auto report = scenario->Execute();
            bool passed = scenario->Validate(report);
            results.emplace_back(scenario->name, passed);
        }
        
        return results;
    }
};

}  // namespace AudioQa

#endif // AUDIOQASCENARIO_H