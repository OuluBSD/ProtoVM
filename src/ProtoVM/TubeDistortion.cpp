#include "TubeDistortion.h"
#include <algorithm>
#include <numeric>

// TubeDistortionModel implementation
TubeDistortionModel::TubeDistortionModel(DistortionType type) 
    : distortionType(type), operatingBias(0.0), preampDrive(1.0), 
      outputImpetance(100000.0), saturationLevel(0.9), cutOffLevel(-0.5), 
      asymmetryFactor(0.1), previousOutput(0.0), capacitorCharge(0.0), 
      timeConstant(0.01) {}

double TubeDistortionModel::processSample(double input, double drive, double bias) {
    // Apply drive and bias adjustments
    double signal = input * drive + bias + operatingBias;
    
    // Apply tube transfer function
    signal = tubeTransferFunction(signal);
    
    // Apply saturation and cutoff effects
    signal = applySaturation(signal);
    signal = applyCutoff(signal);
    signal = applyAsymmetry(signal);
    signal = applyMemoryEffects(signal);
    
    return signal;
}

double TubeDistortionModel::tubeTransferFunction(double input) {
    // A simplified transfer function that models tube characteristics
    // This is a smooth, non-linear function with soft clipping
    
    // Apply preamp drive
    input *= preampDrive;
    
    // Use a function that has a gentle knee and soft limiting
    // This approximates the square-law behavior of tubes
    if (input > 0) {
        // Positive swing - slightly different characteristics
        return 0.9 * tanh(input * 0.8);
    } else {
        // Negative swing - could have different characteristics
        return 0.9 * tanh(input * 0.85);
    }
}

double TubeDistortionModel::applySaturation(double input) {
    // Apply saturation when the signal approaches the power supply limits
    double saturation_point = 0.8;  // Start soft limiting at 80% of max
    
    if (input > saturation_point) {
        // Soft limit using hyperbolic tangent
        double excess = input - saturation_point;
        input = saturation_point + tanh(excess) * 0.1;
    } else if (input < -saturation_point) {
        double excess = input + saturation_point;
        input = -saturation_point + tanh(excess) * 0.1;
    }
    
    return input;
}

double TubeDistortionModel::applyCutoff(double input) {
    // Apply cutoff effect when signal goes below the cutoff point
    if (input < cutOffLevel) {
        // Exponential approach to cutoff
        input = cutOffLevel * exp((input - cutOffLevel) * 2.0);
    }
    
    return input;
}

double TubeDistortionModel::applyAsymmetry(double input) {
    // Apply asymmetry between positive and negative swings
    // Tubes often have slightly different characteristics for positive vs negative swings
    if (input > 0) {
        input *= (1.0 + asymmetryFactor);
    } else {
        input *= (1.0 - asymmetryFactor * 0.5);  // Less asymmetry on negative
    }
    
    return input;
}

double TubeDistortionModel::applyMemoryEffects(double input) {
    // Simulate effects of coupling capacitors and other memory effects
    // This models how previous signal values affect current output
    
    // Simple RC filter effect to simulate coupling capacitors
    capacitorCharge = capacitorCharge * 0.95 + input * 0.05;  // Slow average
    
    // Apply the charge effect to the input
    input = input * 0.9 + capacitorCharge * 0.1;
    
    previousOutput = input;
    return input;
}

std::vector<double> TubeDistortionModel::calculateHarmonics(double input, int numHarmonics) {
    std::vector<double> harmonics(numHarmonics, 0.0);
    
    // Simplified harmonic calculation for demonstration
    // In a real implementation, we'd use FFT or more sophisticated methods
    for (int i = 1; i <= numHarmonics; i++) {
        // Calculate the i-th harmonic content based on non-linear processing
        double harmonic_input = input * i;
        double harmonic_output = processSample(harmonic_input);
        
        // The harmonic magnitude is roughly related to the non-linear processing
        harmonics[i-1] = std::abs(harmonic_output) / i;
    }
    
    // Normalize harmonics
    double fundamental = harmonics[0];
    if (fundamental > 0) {
        for (auto& h : harmonics) {
            h /= fundamental;
        }
    }
    
    return harmonics;
}

double TubeDistortionModel::calculateTHD(double inputLevel, int numHarmonics) {
    auto harmonics = calculateHarmonics(inputLevel, numHarmonics);
    
    // THD = sqrt(sum of squares of harmonics) / fundamental
    double harmonic_sum = 0.0;
    for (int i = 1; i < harmonics.size(); i++) {
        harmonic_sum += harmonics[i] * harmonics[i];
    }
    
    if (harmonics[0] > 0) {
        return sqrt(harmonic_sum) / harmonics[0];
    }
    
    return 0.0;
}

void TubeDistortionModel::setTubeType(const std::string& tubeName) {
    tubeType = tubeName;
    
    // Set parameters based on tube type
    if (tubeName == "12AX7" || tubeName == "12AX7A") {
        // High gain preamp triode
        saturationLevel = 0.85;
        cutOffLevel = -2.0;
        asymmetryFactor = 0.05;
    } else if (tubeName == "EL34") {
        // Power pentode
        saturationLevel = 0.75;
        cutOffLevel = -1.0;
        asymmetryFactor = 0.1;
    } else if (tubeName == "6V6") {
        // Power beam tetrode
        saturationLevel = 0.80;
        cutOffLevel = -0.8;
        asymmetryFactor = 0.08;
    } else if (tubeName == "300B") {
        // Direct heated triode for SET amps
        saturationLevel = 0.90;
        cutOffLevel = -0.3;
        asymmetryFactor = 0.02;
    }
}


// TubeAmplifierSimulation implementation
TubeAmplifierSimulation::TubeAmplifierSimulation() 
    : hasPhaseSplitter(false), inputGain(1.0), outputGain(0.5), 
      currentOutput(0.0), bassControl(0.5), midControl(0.5), 
      trebleControl(0.5), presence(0.5) {}

double TubeAmplifierSimulation::processSample(double input) {
    // Apply input gain
    double signal = input * inputGain;
    
    // Process through preamp stages
    for (auto& stage : preampStages) {
        signal = stage.distortionModel.processSample(signal, stage.drive, 0.0);
        signal *= stage.gain;
    }
    
    // Process through phase splitter if present
    if (hasPhaseSplitter) {
        // Phase splitter creates inverted and non-inverted signals for push-pull
        // For simulation purposes, we'll just process the signal
        signal = signal * 0.8;  // Slight loss in phase splitter
    }
    
    // Process through power amp stages
    for (auto& stage : powerStages) {
        signal = stage.distortionModel.processSample(signal, stage.drive, 0.0);
        signal *= 0.8;  // Power stage gain < 1 for class AB
    }
    
    // Apply tone stack
    signal = toneStackResponse(signal, bassControl, midControl, trebleControl);
    
    // Apply presence control
    signal *= (1.0 - 0.3 * presence);  // Presence affects high end
    
    // Apply master volume
    signal *= outputGain;
    
    // Apply output limiting
    if (signal > 1.0) signal = 1.0;
    if (signal < -1.0) signal = -1.0;
    
    currentOutput = signal;
    return signal;
}

void TubeAmplifierSimulation::addPreamplifierStage(const std::string& tubeType, double gain, double drive) {
    Stage stage;
    stage.tubeType = tubeType;
    stage.gain = gain;
    stage.drive = drive;
    stage.distortionModel.setTubeType(tubeType);
    stage.distortionModel.setDriveLevel(drive);
    
    preampStages.push_back(stage);
}

void TubeAmplifierSimulation::addPhaseSplitterStage(const std::string& tubeType) {
    hasPhaseSplitter = true;
    // In a real implementation, we'd add components for phase splitting
}

void TubeAmplifierSimulation::addPowerAmplifierStage(const std::string& tubeType, int numTubes) {
    Stage stage;
    stage.tubeType = tubeType;
    stage.gain = 0.8;  // Power stages typically have gain < 1 for class AB
    stage.drive = 1.0;
    stage.distortionModel.setTubeType(tubeType);
    stage.distortionModel.setDriveLevel(1.0);
    
    powerStages.push_back(stage);
}

void TubeAmplifierSimulation::setToneControls(double bass, double mid, double treble) {
    bassControl = std::max(0.0, std::min(1.0, bass));
    midControl = std::max(0.0, std::min(1.0, mid));
    trebleControl = std::max(0.0, std::min(1.0, treble));
}

void TubeAmplifierSimulation::reset() {
    currentOutput = 0.0;
    for (auto& stage : preampStages) {
        // Reset distortion model internal states
    }
    for (auto& stage : powerStages) {
        // Reset distortion model internal states
    }
}

double TubeAmplifierSimulation::toneStackResponse(double input, double bass, double mid, double treble) {
    // Simplified tone stack response
    // A real tone stack is a complex RLC network with interaction between controls
    
    // Apply simple shelving filters for demonstration
    double output = input;
    
    // Bass control (low frequency shelving)
    output *= (0.5 + bass * 0.5);
    
    // Mid control (parametric EQ simulation)
    output *= (0.7 + mid * 0.6);
    
    // Treble control (high frequency shelving) 
    output *= (0.6 + treble * 0.8);
    
    return output;
}


// TubeConfigurationModel implementation
TubeConfigurationModel::TubeConfigurationModel(Configuration config) 
    : configuration(config), plateVoltage(250.0), screenVoltage(100.0), 
      cathodeResistor(1500.0), plateResistor(100000.0), feedbackRatio(0.1) {}

double TubeConfigurationModel::processSample(double input) {
    switch (configuration) {
        case SINGLE_ENDED_TRIODE:
            return singleEndedTriode(input);
        case SINGLE_ENDED_PENTODE:
            return singleEndedPentode(input);
        case PUSH_PULL_CLASS_AB:
            return pushPullClassAB(input);
        case CATHODE_FOLLOWER:
            return cathodeFollower(input);
        case DIFFERENTIAL_PAIR:
            return differentialPair(input);
        default:
            return input;  // No processing
    }
}

double TubeConfigurationModel::singleEndedTriode(double input) {
    distortionModel.setTubeType("300B");  // Classic SET power tube
    return distortionModel.processSample(input, 1.0, 0.0);
}

double TubeConfigurationModel::singleEndedPentode(double input) {
    distortionModel.setTubeType("EL34");
    return distortionModel.processSample(input, 0.8, 0.0);
}

double TubeConfigurationModel::pushPullClassAB(double input) {
    // Push-pull configuration cancels even harmonics
    double signal1 = distortionModel.processSample(input, 0.9, 0.0);
    double signal2 = distortionModel.processSample(-input, 0.9, 0.0);  // Inverted signal
    
    // Combine the outputs (simplified)
    return (signal1 - signal2) * 0.7;  // Output transformer effect
}

double TubeConfigurationModel::cathodeFollower(double input) {
    // Cathode follower has gain close to 1, high input impedance, low output impedance
    // For simulation, we'll model the slight non-linearity and buffering effect
    double output = input * 0.98;  // Slight loss but follows input
    return distortionModel.processSample(output, 0.2, 0.0);  // Minimal distortion
}

double TubeConfigurationModel::differentialPair(double input) {
    // Differential pair (long-tailed pair) - commonly used in phase splitters
    double signal1 = distortionModel.processSample(input, 0.8, 0.0);
    double signal2 = distortionModel.processSample(-input, 0.8, 0.0);
    
    // Output is the difference
    return (signal1 - signal2) * 0.5;
}