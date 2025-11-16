#ifndef TUBE_DISTORTION_H
#define TUBE_DISTORTION_H

#include <vector>
#include <cmath>

// Class to model tube distortion characteristics
class TubeDistortionModel {
public:
    enum DistortionType {
        CLASS_A,      // Pure class A operation
        CLASS_AB,     // Class AB operation
        PUSH_PULL,    // Push-pull configuration
        SINGLE_ENDED  // Single-ended configuration
    };

    TubeDistortionModel(DistortionType type = CLASS_A);
    
    // Calculate the output waveform with distortion based on input
    double processSample(double input, double drive = 1.0, double bias = 0.0);
    
    // Calculate harmonic content
    std::vector<double> calculateHarmonics(double input, int numHarmonics = 5);
    
    // Set tube parameters
    void setTubeType(const std::string& tubeName);  // e.g., "12AX7", "EL34", "6V6"
    void setBiasPoint(double bias) { operatingBias = bias; }
    void setDriveLevel(double drive) { preampDrive = drive; }
    void setOutputLoading(double loading) { outputImpedance = loading; }
    
    // Get parameters
    double getBiasPoint() const { return operatingBias; }
    double getDriveLevel() const { return preampDrive; }
    
    // Apply non-linear transfer function typical of tubes
    double tubeTransferFunction(double input);
    
    // Calculate total harmonic distortion (THD)
    double calculateTHD(double inputLevel, int numHarmonics = 10);

private:
    DistortionType distortionType;
    std::string tubeType;
    double operatingBias;        // DC bias point
    double preampDrive;          // Drive level control
    double outputImpetance;      // Plate/load impedance
    double saturationLevel;      // Point where tube begins to saturate
    double cutOffLevel;          // Point where tube cuts off
    double asymmetryFactor;      // How much positive vs negative swing differs
    
    // Internal methods for distortion calculation
    double applySaturation(double input);
    double applyCutoff(double input);
    double applyAsymmetry(double input);
    double applyMemoryEffects(double input);  // For capacitor coupling effects
    
    // Time-based effects (for modeling coupling capacitors, etc.)
    double previousOutput;
    double capacitorCharge;
    double timeConstant;  // RC time constant for coupling caps
};

// A complete tube amplifier stage with preamp and power sections
class TubeAmplifierSimulation {
public:
    TubeAmplifierSimulation();
    
    // Process an input sample through the entire amplifier chain
    double processSample(double input);
    
    // Add a tube stage to the amplifier
    void addPreamplifierStage(const std::string& tubeType = "12AX7", double gain = 30.0, double drive = 1.0);
    void addPhaseSplitterStage(const std::string& tubeType = "12AX7");
    void addPowerAmplifierStage(const std::string& tubeType = "EL34", int numTubes = 2);  // 2 for push-pull
    
    // Set global parameters
    void setInputLevel(double level) { inputGain = level; }
    void setMasterVolume(double level) { outputGain = level; }
    void setToneControls(double bass, double mid, double treble);
    void setPresenceControl(double presence) { this->presence = presence; }
    
    // Get output level
    double getOutputLevel() const { return currentOutput; }
    
    // Reset the simulation state
    void reset();
    
private:
    struct Stage {
        std::string tubeType;
        double gain;
        double drive;
        TubeDistortionModel distortionModel;
    };
    
    std::vector<Stage> preampStages;
    bool hasPhaseSplitter;
    std::vector<Stage> powerStages;
    
    double inputGain;
    double outputGain;
    double currentOutput;
    double bassControl;
    double midControl;
    double trebleControl;
    double presence;
    
    // Tone stack simulation (simplified)
    double toneStackResponse(double input, double bass, double mid, double treble);
};

// Class to simulate different tube configurations and their characteristic sounds
class TubeConfigurationModel {
public:
    enum Configuration {
        SINGLE_ENDED_TRIODE,      // Single-ended triode (SET)
        SINGLE_ENDED_PENTODE,     // Single-ended pentode 
        PUSH_PULL_CLASS_AB,       // Push-pull class AB
        CATHODE_FOLLOWER,         // Cathode follower (voltage buffer)
        DIFFERENTIAL_PAIR         // Differential pair (long-tailed pair)
    };
    
    TubeConfigurationModel(Configuration config = SINGLE_ENDED_TRIODE);
    
    double processSample(double input);
    void setConfiguration(Configuration config) { configuration = config; }
    
private:
    Configuration configuration;
    TubeDistortionModel distortionModel;
    
    // Configuration-specific parameters
    double plateVoltage;
    double screenVoltage;
    double cathodeResistor;
    double plateResistor;
    double feedbackRatio;
    
    // Methods for specific configurations
    double singleEndedTriode(double input);
    double singleEndedPentode(double input);
    double pushPullClassAB(double input);
    double cathodeFollower(double input);
    double differentialPair(double input);
};

#endif // TUBE_DISTORTION_H