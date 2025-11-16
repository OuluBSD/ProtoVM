#include "TubePowerSupply.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// TubePowerSupply implementation
TubePowerSupply::TubePowerSupply(SupplyType type) : supplyType(type) {
    initializeSupply(type);
    actualOutputVoltage = bPlusVoltage;
    rippleAmplitude = (ripplePercent / 100.0) * bPlusVoltage;
}

void TubePowerSupply::initializeSupply(SupplyType type) {
    switch (type) {
        case CLASSIC_EL34:
            inputVoltage = 120.0;
            transformerRatio = 4.2;  // 120V to ~504V AC
            bPlusVoltage = 475.0;    // After rectification and filtering
            maxCurrent = 0.20;       // 200mA max for EL34 push-pull amp
            regulationQuality = 0.7; // Moderate regulation
            ripplePercent = 7.0;     // 7% ripple typical
            break;
            
        case CLASSIC_6V6:
            inputVoltage = 120.0;
            transformerRatio = 3.5;  // 120V to ~420V AC
            bPlusVoltage = 370.0;    // After rectification and filtering
            maxCurrent = 0.15;       // 150mA max for 6V6 amp
            regulationQuality = 0.6; // Poorer regulation (typically lower budget supply)
            ripplePercent = 10.0;    // 10% ripple typical
            break;
            
        case CLASSIC_300B:
            inputVoltage = 120.0;
            transformerRatio = 4.0;  // 120V to ~480V AC
            bPlusVoltage = 450.0;    // After rectification and filtering for SE 300B
            maxCurrent = 0.08;       // 80mA max for 300B SET amp
            regulationQuality = 0.85; // Excellent regulation
            ripplePercent = 3.0;     // 3% ripple typical for high-end SET
            break;
            
        case FLEXIBLE_SUPPLY:
        default:
            // Use default values set in member initialization
            break;
    }
    
    // Calculate DC output voltage after rectification
    double acVoltage = inputVoltage * transformerRatio;
    bPlusVoltage = acVoltage * 1.414 - rectifierDrop;  // Peak AC voltage minus rectifier drop
    
    rippleAmplitude = (ripplePercent / 100.0) * bPlusVoltage;
}

bool TubePowerSupply::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubePowerSupply::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        // Input AC voltage
        double newInputVoltage;
        memcpy(&newInputVoltage, data, sizeof(double));
        inputVoltage = std::abs(newInputVoltage);  // Take absolute value
        
        // Update B+ voltage based on new input
        double acVoltage = inputVoltage * transformerRatio;
        bPlusVoltage = acVoltage * 1.414 - rectifierDrop;
        
        return true;
    }
    return false;
}

bool TubePowerSupply::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(data, &currentOutput, sizeof(double));
        return true;
    } else if (conn_id == currentSensePin && data_bytes == sizeof(double)) {
        double currentDraw = totalLoad;
        memcpy(data, &currentDraw, sizeof(double));
        return true;
    }
    return false;
}

bool TubePowerSupply::Tick() {
    calculateOutputVoltage();
    updateRipple();
    
    // Calculate instantaneous output voltage including ripple
    double ripple = rippleAmplitude * sin(ripplePhase);
    currentOutput = actualOutputVoltage + ripple;
    
    // Update ripple phase for next sample
    ripplePhase += 2.0 * M_PI * rippleFrequency / sampleRate;
    if (ripplePhase >= 2.0 * M_PI) {
        ripplePhase -= 2.0 * M_PI;
    }
    
    return true;
}

void TubePowerSupply::calculateOutputVoltage() {
    // Calculate voltage drop due to load
    double loadFactor = totalLoad / maxCurrent;
    double regulationEffect = (1.0 - regulationQuality * loadFactor);
    
    // Apply sag if enabled
    double sagFactor = 1.0;
    if (sagEnabled) {
        sagFactor = 1.0 - (sagAmount * loadFactor);
    }
    
    // Calculate actual output voltage considering load and sag
    actualOutputVoltage = bPlusVoltage * regulationEffect * sagFactor;
    
    // Ensure voltage doesn't go below reasonable level
    actualOutputVoltage = std::max(actualOutputVoltage, bPlusVoltage * 0.3);
}

void TubePowerSupply::updateRipple() {
    // Update ripple amplitude based on current load (higher load = higher ripple)
    rippleAmplitude = (ripplePercent / 100.0) * actualOutputVoltage * (1.0 + 0.5 * (totalLoad / maxCurrent));
}

void TubePowerSupply::addLoad(double currentDraw) {
    totalLoad += std::max(0.0, currentDraw);
    totalLoad = std::min(totalLoad, maxCurrent * 1.5); // Allow some overload
}

void TubePowerSupply::removeLoad(double currentDraw) {
    totalLoad -= std::max(0.0, currentDraw);
    totalLoad = std::max(0.0, totalLoad);
}


// TubeRectifier implementation
TubeRectifier::TubeRectifier(RectifierType type) : rectifierType(type) {
    initializeRectifier(type);
}

void TubeRectifier::initializeRectifier(RectifierType type) {
    switch (type) {
        case TYPE_5Y3:
            voltageDrop = 45.0;          // 45V typical drop for 5Y3
            maxCurrent = 0.06;           // 60mA max current
            internalResistance = 750.0;  // 750 ohms internal resistance
            break;
            
        case TYPE_5U4:
            voltageDrop = 50.0;          // 50V typical drop for 5U4
            maxCurrent = 0.25;           // 250mA max current
            internalResistance = 500.0;  // 500 ohms internal resistance
            break;
            
        case TYPE_275C3:
            voltageDrop = 55.0;          // 55V typical drop
            maxCurrent = 0.15;           // 150mA max current
            internalResistance = 600.0;  // 600 ohms internal resistance
            break;
            
        case TYPE_GZ37:
            voltageDrop = 100.0;         // 100V typical drop for GZ37
            maxCurrent = 0.05;           // 50mA max current (high voltage = higher drop)
            internalResistance = 1000.0; // 1000 ohms internal resistance
            break;
    }
}

bool TubeRectifier::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeRectifier::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == acInputPin && data_bytes == sizeof(double)) {
        // AC input that needs rectification
        return true;
    }
    return false;
}

bool TubeRectifier::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == highVoltagePin && data_bytes == sizeof(double)) {
        memcpy(data, &currentOutput, sizeof(double));
        return true;
    }
    return false;
}

bool TubeRectifier::Tick() {
    // This would be driven by external AC signal
    // For simulation purposes, we'll assume a standard AC input
    static double acPhase = 0.0;
    static double sampleRate = 44100.0;
    static double acFrequency = 60.0;  // 60Hz
    
    // Simulate AC input
    double acInput = 120.0 * sin(acPhase);
    
    // Apply rectification
    currentOutput = calculateRectifiedOutput(acInput);
    
    // Update phase for next sample
    acPhase += 2.0 * M_PI * acFrequency / sampleRate;
    if (acPhase >= 2.0 * M_PI) {
        acPhase -= 2.0 * M_PI;
    }
    
    return true;
}

double TubeRectifier::calculateRectifiedOutput(double acInput) {
    // Full-wave rectification
    double rectified = std::abs(acInput);
    
    // Apply voltage drop
    rectified = std::max(0.0, rectified - voltageDrop);
    
    // Apply load-dependent voltage drop based on internal resistance
    // For simulation purposes, assume typical load
    double currentFlow = rectified / (internalResistance + 10000.0); // 10k load
    double voltageDropAcrossInternal = currentFlow * internalResistance;
    
    return std::max(0.0, rectified - voltageDropAcrossInternal);
}


// TubeHeaterSupply implementation
TubeHeaterSupply::TubeHeaterSupply(double voltage, double current) 
    : heaterVoltage(voltage), maxHeaterCurrent(current) {
    // Initialize to valid state
    currentOutput = heaterVoltage;
}

bool TubeHeaterSupply::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeHeaterSupply::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Heater supply is typically a fixed DC voltage source, 
    // so we don't expect external writes to change its behavior
    return false;
}

bool TubeHeaterSupply::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &currentOutput, sizeof(double));
        return true;
    }
    return false;
}

bool TubeHeaterSupply::Tick() {
    // Check if we can handle the current load
    if (currentHeaterLoad > maxHeaterCurrent * 0.9) {  // 90% threshold
        // Reduce heater voltage under overload
        currentOutput = heaterVoltage * (1.0 - (currentHeaterLoad - maxHeaterCurrent * 0.9) / (maxHeaterCurrent * 0.1));
        currentOutput = std::max(0.0, currentOutput);
    } else {
        // Normal operation
        currentOutput = heaterVoltage;
    }
    
    return true;
}

void TubeHeaterSupply::addTubeHeaterLoad(double current) {
    currentHeaterLoad += std::max(0.0, current);
    currentHeaterLoad = std::min(currentHeaterLoad, maxHeaterCurrent * 1.2); // Allow some overload
}

void TubeHeaterSupply::removeTubeHeaterLoad(double current) {
    currentHeaterLoad -= std::max(0.0, current);
    currentHeaterLoad = std::max(0.0, currentHeaterLoad);
}

bool TubeHeaterSupply::canHandleLoad(double additionalCurrent) const {
    return (currentHeaterLoad + additionalCurrent) <= maxHeaterCurrent;
}