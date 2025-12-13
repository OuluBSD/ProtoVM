#ifndef AUDIOQADIFF_H
#define AUDIOQADIFF_H

#include "AudioQa.h"
#include <vector>
#include <string>
#include <map>
#include <json/json.h>  // Using JsonCpp library for JSON serialization

namespace AudioQa {

enum class AudioQaDiffVerdict {
    regression,
    improvement,
    neutral,
    error
};

struct AudioQaDiffEntry {
    AudioQaMetricKind metric_kind;
    double before_value;
    double after_value;
    double delta;
    AudioQaDiffVerdict verdict;
    std::string description;
    
    Json::Value ToJson() const {
        Json::Value json;
        json["metric"] = MetricKindToString(metric_kind);
        json["before"] = before_value;
        json["after"] = after_value;
        json["delta"] = delta;
        json["verdict"] = VerdictToString(verdict);
        json["description"] = description;
        return json;
    }
    
private:
    std::string MetricKindToString(AudioQaMetricKind kind) const {
        switch (kind) {
            case AudioQaMetricKind::RMSLevel: return "RMSLevel";
            case AudioQaMetricKind::PeakLevel: return "PeakLevel";
            case AudioQaMetricKind::DCOffset: return "DCOffset";
            case AudioQaMetricKind::StereoBalance: return "StereoBalance";
            case AudioQaMetricKind::StereoWidth: return "StereoWidth";
            case AudioQaMetricKind::PhaseCorrelation: return "PhaseCorrelation";
            case AudioQaMetricKind::FundamentalFrequency: return "FundamentalFrequency";
            case AudioQaMetricKind::HarmonicEnergy: return "HarmonicEnergy";
            case AudioQaMetricKind::SilenceRatio: return "SilenceRatio";
            default: return "Unknown";
        }
    }
    
    std::string VerdictToString(AudioQaDiffVerdict verdict) const {
        switch (verdict) {
            case AudioQaDiffVerdict::regression: return "regression";
            case AudioQaDiffVerdict::improvement: return "improvement";
            case AudioQaDiffVerdict::neutral: return "neutral";
            case AudioQaDiffVerdict::error: return "error";
            default: return "unknown";
        }
    }
};

struct AudioQaDiff {
    std::vector<AudioQaDiffEntry> entries;
    std::string comparison_timestamp;
    std::string scenario_name;
    
    void AddDiffEntry(const AudioQaDiffEntry& entry) {
        entries.push_back(entry);
    }
    
    Json::Value ToJson() const {
        Json::Value json;
        json["scenario"] = scenario_name;
        json["timestamp"] = comparison_timestamp;
        
        Json::Value diffs(Json::arrayValue);
        for (const auto& entry : entries) {
            diffs.append(entry.ToJson());
        }
        json["diffs"] = diffs;
        
        return json;
    }
    
    // Helper function to determine overall regression status
    bool HasRegression() const {
        for (const auto& entry : entries) {
            if (entry.verdict == AudioQaDiffVerdict::regression) {
                return true;
            }
        }
        return false;
    }
    
    // Count regressions
    size_t CountRegressions() const {
        size_t count = 0;
        for (const auto& entry : entries) {
            if (entry.verdict == AudioQaDiffVerdict::regression) {
                count++;
            }
        }
        return count;
    }
};

struct AudioQaRegression {
    std::string issue_description;
    std::vector<AudioQaDiffEntry> contributing_factors;
    std::string suggested_fix;
    double severity_score;  // 0.0 to 1.0, higher is more severe
    
    Json::Value ToJson() const {
        Json::Value json;
        json["issue_description"] = issue_description;
        json["severity_score"] = severity_score;
        json["suggested_fix"] = suggested_fix;
        
        Json::Value factors(Json::arrayValue);
        for (const auto& factor : contributing_factors) {
            factors.append(factor.ToJson());
        }
        json["contributing_factors"] = factors;
        
        return json;
    }
};

class AudioQaComparator {
public:
    static AudioQaDiff CompareReports(const AudioQaReport& before, const AudioQaReport& after, 
                                      const std::string& scenario_name = "") {
        AudioQaDiff diff;
        diff.scenario_name = scenario_name;
        
        // Map metrics from 'before' report by kind for quick lookup
        std::map<AudioQaMetricKind, const AudioQaMetric*> before_map;
        for (const auto& metric : before.metrics) {
            before_map[metric.kind] = &metric;
        }
        
        // Compare each metric in 'after' report
        for (const auto& metric_after : after.metrics) {
            AudioQaDiffEntry entry;
            entry.metric_kind = metric_after.kind;
            entry.after_value = metric_after.value;
            entry.description = metric_after.description;
            
            auto it = before_map.find(metric_after.kind);
            if (it != before_map.end()) {
                // Both reports have this metric
                entry.before_value = it->second->value;
                entry.delta = metric_after.value - it->second->value;
                
                // Determine verdict based on metric type and change
                entry.verdict = DetermineVerdict(metric_after.kind, it->second->value, metric_after.value);
            } else {
                // Only 'after' has this metric
                entry.before_value = 0.0;  // or NaN
                entry.delta = metric_after.value;
                entry.verdict = AudioQaDiffVerdict::neutral;  // Can't determine change direction
            }
            
            diff.AddDiffEntry(entry);
        }
        
        // Also check for metrics that disappeared in 'after' report
        for (const auto& metric_before : before.metrics) {
            if (std::find_if(after.metrics.begin(), after.metrics.end(),
                            [&metric_before](const AudioQaMetric& m) {
                                return m.kind == metric_before.kind;
                            }) == after.metrics.end()) {
                // This metric existed in 'before' but not in 'after'
                AudioQaDiffEntry entry;
                entry.metric_kind = metric_before.kind;
                entry.before_value = metric_before.value;
                entry.after_value = 0.0;  // Doesn't exist anymore
                entry.delta = -metric_before.value;
                entry.description = metric_before.description;
                entry.verdict = AudioQaDiffVerdict::regression;  // Missing data is usually bad
                
                diff.AddDiffEntry(entry);
            }
        }
        
        return diff;
    }

private:
    static AudioQaDiffVerdict DetermineVerdict(AudioQaMetricKind kind, double before, double after) {
        // Define what constitutes regression/improvement for each metric
        switch (kind) {
            case AudioQaMetricKind::RMSLevel:
                // Small changes in RMS might be neutral, large changes either way could be issues
                if (std::abs(after - before) > 3.0) {  // 3dB change
                    return after < before ? AudioQaDiffVerdict::regression : AudioQaDiffVerdict::improvement;
                }
                break;
                
            case AudioQaMetricKind::PeakLevel:
                // Large increases in peak level could cause clipping (regression)
                if (after > before && after > -0.5) {  //接近0dBFS可能引起削波
                    return AudioQaDiffVerdict::regression;
                }
                break;
                
            case AudioQaMetricKind::DCOffset:
                // DC offset is generally undesirable, so decrease is improvement
                if (std::abs(after) < std::abs(before)) {
                    return AudioQaDiffVerdict::improvement;
                } else if (std::abs(after) > std::abs(before)) {
                    return AudioQaDiffVerdict::regression;
                }
                break;
                
            case AudioQaMetricKind::StereoBalance:
                // Changes in balance might be intentional, but dramatic shifts could be issues
                if (std::abs(after) > 0.9 && std::abs(before) < 0.1) {
                    // Went from centered to extreme - could be a regression if unintended
                    return AudioQaDiffVerdict::regression;
                }
                break;
                
            case AudioQaMetricKind::StereoWidth:
                // Increase in width is often improvement, decrease might be regression
                if (after > before) {
                    return AudioQaDiffVerdict::improvement;
                } else if (after < before) {
                    return AudioQaDiffVerdict::regression;
                }
                break;
                
            case AudioQaMetricKind::PhaseCorrelation:
                // Values near -1 or +1 indicate phase issues, 0 is ideal for decorrelation
                if (std::abs(after) > 0.7 && std::abs(before) <= 0.7) {
                    return AudioQaDiffVerdict::regression;
                } else if (std::abs(after) <= 0.7 && std::abs(before) > 0.7) {
                    return AudioQaDiffVerdict::improvement;
                }
                break;
                
            case AudioQaMetricKind::FundamentalFrequency:
                // Changes in fundamental could mean pitch shifted
                if (std::abs(after - before) > 20.0) { // 20Hz tolerance
                    return AudioQaDiffVerdict::regression;
                }
                break;
                
            case AudioQaMetricKind::HarmonicEnergy:
                // Depends on context - might be ok change for some instruments but not others
                if (std::abs(after - before) > 0.2) { // 20% change
                    // Could be regression or improvement depending on intent
                    return AudioQaDiffVerdict::neutral; 
                }
                break;
                
            case AudioQaMetricKind::SilenceRatio:
                // More silence might be regression (output cut out), less might be improvement
                if (after > before + 0.1) {  // 10% more silence
                    return AudioQaDiffVerdict::regression;
                }
                break;
        }
        
        return AudioQaDiffVerdict::neutral;
    }
};

}  // namespace AudioQa

#endif // AUDIOQADIFF_H