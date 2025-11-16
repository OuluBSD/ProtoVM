#ifndef TUBE_MODELS_H
#define TUBE_MODELS_H

#include <cmath>
#include <vector>

// Common constants for tube modeling
namespace TubeConstants {
    static const double electronCharge = 1.60217662e-19;  // Coulombs
    static const double epsilon0 = 8.854187817e-12;      // F/m (permittivity of free space)
    static const double k = 1.38064852e-23;              // Boltzmann constant J/K
    static const double pi = 3.14159265358979323846;
}

// Abstract base class for tube models
class TubeModel {
public:
    virtual ~TubeModel() = default;
    
    // Input: voltage between control grid and cathode
    // Output: resulting anode current
    virtual double calculateAnodeCurrent(double v_gk, double v_ak) = 0;
    
    // Set tube parameters
    virtual void setAmplificationFactor(double mu) { amplificationFactor = mu; }
    virtual void setTransconductance(double gm) { transconductance = gm; }
    virtual void setAnodeResistance(double ra) { anodeResistance = ra; }
    
    // Get tube parameters
    double getAmplificationFactor() const { return amplificationFactor; }
    double getTransconductance() const { return transconductance; }
    double getAnodeResistance() const { return anodeResistance; }
    
protected:
    double amplificationFactor = 10.0;    // mu - amplification factor
    double transconductance = 0.001;      // gm - in mhos (1/ohms)
    double anodeResistance = 100000.0;   // ra - in ohms
};

// Triode tube model
class TriodeModel : public TubeModel {
public:
    TriodeModel();
    
    // Calculate anode current using basic triode equations
    virtual double calculateAnodeCurrent(double v_gk, double v_ak) override;
    
    // Set tube-specific parameters
    void setPlateResistance(double rp) { plateResistance = rp; }
    void setMu(double mu) { amplificationFactor = mu; }
    
    double getPlateResistance() const { return plateResistance; }
    
private:
    double plateResistance = 6200.0;     // rp in ohms (for 12AX7)
    double emissionConstant = 1.0;       // For the Child-Langmuir law
    double cutoffBias = -1.5;            // Approximate cutoff voltage
    
    // Parameters specific to 12AX7
    void set12AX7Params();
};

// Pentode tube model
class PentodeModel : public TubeModel {
public:
    PentodeModel();
    
    // Calculate anode current for pentode
    virtual double calculateAnodeCurrent(double v_gk, double v_ak) override;
    
    // Calculate screen grid current
    double calculateScreenCurrent(double v_gk, double v_sk);  // v_sk = voltage between screen grid and cathode
    
    // Set pentode-specific parameters
    void setScreenResistance(double rs) { screenResistance = rs; }
    void setScreenTransconductance(double gms) { screenTransconductance = gms; }
    void setSuppressionRatio(double s) { suppressionRatio = s; }
    
    double getScreenResistance() const { return screenResistance; }
    double getScreenTransconductance() const { return screenTransconductance; }
    double getSuppressionRatio() const { return suppressionRatio; }
    
private:
    double screenResistance = 2000.0;         // rs in ohms
    double screenTransconductance = 0.0005;  // gms in mhos
    double suppressionRatio = 0.02;          // sigma - ratio of screen to control grid effect
    double screenVoltage = 100.0;            // Fixed screen voltage (simplified model)
};

// Tetrode tube model
class TetrodeModel : public TubeModel {
public:
    TetrodeModel();
    
    // Calculate anode current for tetrode
    virtual double calculateAnodeCurrent(double v_gk, double v_ak) override;
    
    // Calculate screen grid current
    double calculateScreenCurrent(double v_gk, double v_sk);
    
    // Set tetrode-specific parameters
    void setScreenResistance(double rs) { screenResistance = rs; }
    void setScreenTransconductance(double gms) { screenTransconductance = gms; }
    void setSecondaryEmissionRatio(double se) { secondaryEmissionRatio = se; }
    
    double getScreenResistance() const { return screenResistance; }
    double getScreenTransconductance() const { return screenTransconductance; }
    double getSecondaryEmissionRatio() const { return secondaryEmissionRatio; }
    
private:
    double screenResistance = 1500.0;         // rs in ohms
    double screenTransconductance = 0.0008;  // gms in mhos
    double secondaryEmissionRatio = 0.3;     // gamma - secondary emission effect
    double screenVoltage = 125.0;            // Fixed screen voltage (simplified model)
    
    // Tetrode-specific characteristics
    double kinkEffectFactor = 0.1;           // Factor for kink effect in tetrodes
};

// Class to represent a complete tube component with multiple elements
class VacuumTube {
public:
    enum TubeType {
        TRIODE,
        PENTODE,
        TETRODE
    };
    
    VacuumTube(TubeType type);
    
    // Calculate currents for all elements
    void updateState(double gridVoltage, double anodeVoltage, double screenVoltage = 0.0);
    
    // Get currents
    double getAnodeCurrent() const { return anodeCurrent; }
    double getGridCurrent() const { return gridCurrent; }
    double getScreenCurrent() const { return screenCurrent; }  // For pentodes/tetrodes only
    
    // Get voltages
    double getGridVoltage() const { return gridVoltage; }
    double getAnodeVoltage() const { return anodeVoltage; }
    double getScreenVoltage() const { return screenVoltage; }
    
    // Get the internal tube model
    TubeModel* getTubeModel() { return tubeModel.get(); }
    
private:
    TubeType tubeType;
    std::unique_ptr<TubeModel> tubeModel;
    
    // Current state
    double gridVoltage = 0.0;
    double anodeVoltage = 0.0;
    double screenVoltage = 0.0;
    double anodeCurrent = 0.0;
    double gridCurrent = 0.0;
    double screenCurrent = 0.0;      // For pentodes/tetrodes
    
    void initializeModel();
};

#endif // TUBE_MODELS_H