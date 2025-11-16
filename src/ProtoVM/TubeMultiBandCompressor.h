#ifndef _ProtoVM_TubeMultiBandCompressor_h_
#define _ProtoVM_TubeMultiBandCompressor_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based multi-band compressor circuits for independent frequency range control
class TubeMultiBandCompressor : public ElectricNodeBase {
public:
    typedef TubeMultiBandCompressor CLASSNAME;

    enum MultiBandConfig {
        THREE_BAND,         // Low / Mid / High
        FIVE_BAND,          // Very Low / Low / Mid / High / Very High
        SEVEN_BAND,         // Multiple bands across spectrum
        PARAMETRIC_BAND     // Fully parametric with adjustable bands
    };

    TubeMultiBandCompressor(MultiBandConfig config = FIVE_BAND);
    virtual ~TubeMultiBandCompressor();

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configuration methods for multi-band compressor parameters
    void SetBandThreshold(int band, double threshold);   // in dB
    void SetBandRatio(int band, double ratio);           // compression ratio (e.g., 4.0 for 4:1)
    void SetBandAttackTime(int band, double time);       // in seconds
    void SetBandReleaseTime(int band, double time);      // in seconds
    void SetMakeupGain(double gain);                     // overall makeup gain in dB
    void SetCrossoverFreq(int band, double freq);        // crossover frequency in Hz
    void SetBandSolo(int band, bool solo);               // solo a specific band
    void SetBandMute(int band, bool mute);               // mute a specific band
    void SetOverallGain(double gain);                    // overall gain in dB

    // Get parameters
    double GetBandThreshold(int band) const;
    double GetBandRatio(int band) const;
    double GetBandAttackTime(int band) const;
    double GetBandReleaseTime(int band) const;
    double GetMakeupGain() const { return makeupGain; }
    double GetCrossoverFreq(int band) const;
    bool GetBandSolo(int band) const;
    bool GetBandMute(int band) const;
    double GetOverallGain() const { return overallGain; }

    // Enable/disable features
    void EnableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }
    void EnableLinkBands(bool link) { linkBands = link; }  // Link all bands' compression

private:
    MultiBandConfig config;

    // Number of bands based on configuration
    int numBands;

    // Band-specific parameters
    std::vector<double> thresholds;       // Threshold in dB for each band
    std::vector<double> ratios;           // Ratio for each band
    std::vector<double> attackTimes;      // Attack time in seconds for each band
    std::vector<double> releaseTimes;     // Release time in seconds for each band
    std::vector<double> crossoverFreqs;   // Crossover frequencies in Hz
    std::vector<double> bandGains;        // Current gain for each band
    std::vector<double> prevBandGains;    // Previous gain for each band (for smoothing)
    std::vector<double> attackCoeffs;     // Attack coefficients for each band
    std::vector<double> releaseCoeffs;    // Release coefficients for each band
    std::vector<bool> bandSolan;          // Solo state for each band
    std::vector<bool> bandMuted;          // Mute state for each band
    
    // Overall parameters
    double makeupGain = 0.0;              // Overall makeup gain in dB
    double overallGain = 0.0;             // Overall gain in dB
    bool linkBands = false;               // Whether to link bands together

    // Filter coefficients for crossover networks
    struct FilterCoeffs {
        double low_a1, low_a2, low_b0, low_b1, low_b2;  // Low-pass filter
        double high_a1, high_a2, high_b0, high_b1, high_b2; // High-pass filter
    };
    std::vector<FilterCoeffs> filterCoeffs;  // Filter coefficients for each crossover

    // Band processing state
    std::vector<double> bandSignals;      // Processed signals for each band
    std::vector<double> filterStateX;     // Filter state (previous input)
    std::vector<double> filterStateY;     // Filter state (previous output)

    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tubeGain = 25.0;               // Base tube gain for multi-band processing

    // Circuit parameters
    bool tubeCharacteristicsEnabled = true;  // Apply tube characteristics
    bool autoMakeupEnabled = true;           // Apply automatic makeup gain

    // Sample rate for calculations
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                   // For external control of compression

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;

    // Initialize multi-band compressor based on configuration
    void InitializeMultiBand(MultiBandConfig config);

    // Process signal through multi-band algorithm
    void ProcessSignal();

    // Process a single band (apply compression)
    double ProcessBand(int band, double input, double detectionLevel);

    // Apply crossover filters to separate frequency bands
    void ApplyCrossoverFilters();

    // Calculate filter coefficients for crossover
    void CalculateFilterCoeffs(int band, double freq);

    // Apply tube characteristics
    void ApplyTubeCharacteristics();
};

#endif