#include "TubeModels.h"
#include <memory>

// TriodeModel implementation
TriodeModel::TriodeModel() {
    set12AX7Params();
}

void TriodeModel::set12AX7Params() {
    amplificationFactor = 100.0;  // mu for 12AX7
    transconductance = 0.00165;   // gm = 1650 µmhos for 12AX7
    plateResistance = 62000.0;    // rp in ohms for 12AX7
    cutoffBias = -1.5;            // Approximate cutoff voltage for 12AX7
}

double TriodeModel::calculateAnodeCurrent(double v_gk, double v_ak) {
    // Simplified triode model based on the basic triode equations
    // Using the formulation: Ia = gm * (mu * Vgk + Vak) for small signals
    // For a more accurate model, we'd use the square-law relationship:
    // Ia = Ip0 * (mu * Vgk + Vak)^1.5 (for Vgk > cutoff and Vak > 0)
    
    // First, check if the tube is in cutoff
    if (v_gk < cutoffBias) {
        return 0.0;  // Tube is cut off, no current flows
    }
    
    // For a more accurate model, implement the square law with space charge effects
    // Ia = K * (mu * Vgk + Vak)^1.5 where K is a construction-dependent constant
    double k = transconductance / (1.5 * pow(amplificationFactor, 0.5));
    
    double effective_voltage = amplificationFactor * v_gk + v_ak;
    if (effective_voltage <= 0.0) {
        return 0.0;  // No forward bias
    }
    
    double current = k * pow(effective_voltage, 1.5);
    
    // Limit current based on anode resistance effect
    current = std::min(current, v_ak / plateResistance);
    
    return current;
}


// PentodeModel implementation
PentodeModel::PentodeModel() {
    // Typical pentode parameters for EF86
    amplificationFactor = 95.0;     // mu
    transconductance = 0.007;       // gm
    screenTransconductance = 0.0005; // gms
    suppressionRatio = 0.02;        // sigma
    screenResistance = 470.0;       // rs in ohms
    screenVoltage = 100.0;          // Typical screen voltage
}

double PentodeModel::calculateAnodeCurrent(double v_gk, double v_ak) {
    // In pentodes, anode current is mostly controlled by grid voltage
    // The screen grid is usually held at a fixed positive voltage
    // Ia ≈ gm * μ * Vgk, where effective μ is reduced due to screen
    
    // Effective amplification factor considering screen grid
    double effective_mu = amplificationFactor * suppressionRatio;
    
    // Calculate anode current based on control grid voltage
    double effective_voltage = v_gk + (v_ak / effective_mu);
    
    // If control grid is too negative, current is cut off
    if (effective_voltage < -2.0) {  // Approximate cutoff
        return 0.0;
    }
    
    double current = transconductance * effective_mu * std::max(0.0, effective_voltage);
    
    // Add some dependency on anode voltage (penteode still has some anode voltage effect)
    current *= (1.0 + 0.1 * std::tanh(v_ak / 100.0));  // Gentle anode voltage effect
    
    return current;
}

double PentodeModel::calculateScreenCurrent(double v_gk, double v_sk) {
    // Screen current is dependent on both grid and screen voltages
    // Typically not as strongly controlled by grid as anode current
    double grid_influence = 0.1;  // Screen current is less affected by grid
    double screen_current = screenTransconductance * (v_sk + grid_influence * v_gk);
    
    return std::max(0.0, screen_current);
}


// TetrodeModel implementation
TetrodeModel::TetrodeModel() {
    // Typical tetrode parameters
    amplificationFactor = 140.0;    // mu
    transconductance = 0.005;       // gm
    screenTransconductance = 0.0008; // gms
    secondaryEmissionRatio = 0.3;   // gamma
    screenResistance = 680.0;       // rs in ohms
    screenVoltage = 125.0;          // Typical screen voltage
    kinkEffectFactor = 0.1;         // Factor for kink effect
}

double TetrodeModel::calculateAnodeCurrent(double v_gk, double v_ak) {
    // Tetrode current model including secondary emission effects
    double primary_current = transconductance * amplificationFactor * std::max(0.0, v_gk);
    
    // Secondary emission can reduce anode current when anode voltage is low
    // This creates the characteristic "kink" in tetrode characteristics
    double secondary_effect = 0.0;
    if (v_ak < screenVoltage * 0.8) {  // Significant secondary emission when Va < ~0.8*Vs
        // Calculate secondary emission effect
        secondary_effect = secondaryEmissionRatio * primary_current * (1.0 - v_ak/(screenVoltage * 0.8));
    }
    
    double current = primary_current - secondary_effect;
    
    // Apply kink effect correction
    if (v_ak < screenVoltage * 0.5 && v_ak > 10.0) {
        current *= (1.0 - kinkEffectFactor * sin(pi * v_ak / (0.5 * screenVoltage)));
    }
    
    return std::max(0.0, current);
}

double TetrodeModel::calculateScreenCurrent(double v_gk, double v_sk) {
    // Screen current in tetrode - higher than in pentode due to secondary emission
    double grid_influence = 0.15;  // More influence from grid than pentode
    double screen_current = screenTransconductance * (v_sk + grid_influence * v_gk);
    
    return std::max(0.0, screen_current);
}


// VacuumTube implementation
VacuumTube::VacuumTube(TubeType type) : tubeType(type) {
    initializeModel();
}

void VacuumTube::initializeModel() {
    switch (tubeType) {
        case TRIODE:
            tubeModel = std::make_unique<TriodeModel>();
            break;
        case PENTODE:
            tubeModel = std::make_unique<PentodeModel>();
            break;
        case TETRODE:
            tubeModel = std::make_unique<TetrodeModel>();
            break;
    }
}

void VacuumTube::updateState(double gridVoltage, double anodeVoltage, double screenVoltage) {
    this->gridVoltage = gridVoltage;
    this->anodeVoltage = anodeVoltage;
    this->screenVoltage = screenVoltage;
    
    // Calculate anode current using the model
    anodeCurrent = tubeModel->calculateAnodeCurrent(gridVoltage, anodeVoltage);
    
    // Calculate grid current (usually very small, mainly due to grid-cathode leakage)
    gridCurrent = 0.000001 * std::max(0.0, gridVoltage);  // Very small leakage
    
    // Calculate screen current for pentodes and tetrodes
    if (tubeType == PENTODE || tubeType == TETRODE) {
        if (auto pentode = dynamic_cast<PentodeModel*>(tubeModel.get())) {
            screenCurrent = pentode->calculateScreenCurrent(gridVoltage, screenVoltage);
        } else if (auto tetrode = dynamic_cast<TetrodeModel*>(tubeModel.get())) {
            screenCurrent = tetrode->calculateScreenCurrent(gridVoltage, screenVoltage);
        }
    } else {
        screenCurrent = 0.0;
    }
}