#ifndef TUBE_CIRCUIT_TOPOLOGIES_H
#define TUBE_CIRCUIT_TOPOLOGIES_H

#include "ElectricNodeBase.h"
#include "TubeComponents.h"
#include <vector>
#include <memory>

// Class for cathode follower circuit (voltage buffer)
class CathodeFollower : public ElectricNodeBase {
public:
    CathodeFollower(const std::string& tubeType = "12AX7");
    virtual ~CathodeFollower() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure cathode follower parameters
    void setTubeType(const std::string& type) { tubeType = type; updateParams(); }
    void setCathodeResistor(double resistance) { cathodeResistor = resistance; updateParams(); }
    void setLoadResistor(double resistance) { loadResistor = resistance; updateParams(); }
    void setBPlusVoltage(double voltage) { bPlusVoltage = voltage; }
    void setOperatingPoint(double bias) { operatingBias = bias; }
    
    // Get parameters
    double getCathodeResistor() const { return cathodeResistor; }
    double getLoadResistor() const { return loadResistor; }
    double getBPlusVoltage() const { return bPlusVoltage; }
    double getOperatingPoint() const { return operatingBias; }
    
    // Get gain and impedance characteristics
    double getGain() const { return calculatedGain; }
    double getInputImpedance() const { return inputImpedance; }
    double getOutputImpedance() const { return outputImpedance; }

private:
    std::string tubeType = "12AX7";
    double cathodeResistor = 1500.0;     // 1.5kΩ typical cathode resistor
    double loadResistor = 100000.0;      // 100kΩ typical load for triode
    double bPlusVoltage = 250.0;         // B+ voltage
    double operatingBias = -1.5;         // Grid bias voltage
    double gridResistor = 1000000.0;     // 1MΩ grid leak resistor
    
    // Calculated parameters
    double calculatedGain = 0.95;        // Close to unity for cathode follower
    double inputImpedance = 1000000.0;   // High input impedance
    double outputImpedance = 1000.0;     // Low output impedance
    
    // Tube model for simulation
    std::unique_ptr<TubeModel> tubeModel;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int gridPin = 0;                     // Same as input for cathode follower
    int cathodePin = 2;
    int bPlusPin = 3;
    int groundPin = 4;
    
    // Operating state
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double gridVoltage = 0.0;
    double cathodeVoltage = 0.0;
    double plateVoltage = 0.0;
    double current = 0.0;
    
    // Initialize parameters based on tube type
    void updateParams();
    
    // Process the signal through the cathode follower
    void processSignal();
};

// Class for grounded cathode amplifier (common cathode)
class GroundedCathodeAmp : public ElectricNodeBase {
public:
    GroundedCathodeAmp(const std::string& tubeType = "12AX7");
    virtual ~GroundedCathodeAmp() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure amplifier parameters
    void setTubeType(const std::string& type) { tubeType = type; updateParams(); }
    void setPlateResistor(double resistance) { plateResistor = resistance; updateParams(); }
    void setGridResistor(double resistance) { gridResistor = resistance; }
    void setCathodeResistor(double resistance) { cathodeResistor = resistance; updateParams(); }
    void setBPlusVoltage(double voltage) { bPlusVoltage = voltage; }
    void setOperatingPoint(double bias) { operatingBias = bias; }
    
    // Get parameters
    double getPlateResistor() const { return plateResistor; }
    double getGridResistor() const { return gridResistor; }
    double getCathodeResistor() const { return cathodeResistor; }
    double getBPlusVoltage() const { return bPlusVoltage; }
    double getOperatingPoint() const { return operatingBias; }
    
    // Get gain and impedance characteristics
    double getGain() const { return calculatedGain; }
    double getInputImpedance() const { return inputImpedance; }
    double getOutputImpedance() const { return outputImpedance; }

private:
    std::string tubeType = "12AX7";
    double plateResistor = 100000.0;     // 100kΩ typical plate resistor
    double gridResistor = 1000000.0;     // 1MΩ grid leak resistor
    double cathodeResistor = 1500.0;     // 1.5kΩ cathode resistor
    double bPlusVoltage = 250.0;
    double operatingBias = -1.5;
    
    // Calculated parameters
    double calculatedGain = -35.0;       // Negative for inverting amplifier
    double inputImpedance = 1000000.0;
    double outputImpedance = 100000.0;   // Close to plate resistor value
    
    // Tube model for simulation
    std::unique_ptr<TubeModel> tubeModel;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int gridPin = 0;                     // Same as input
    int cathodePin = 2;                  // Grounded via cathode resistor
    int platePin = 1;                    // Output taken from plate
    int bPlusPin = 3;
    int groundPin = 4;
    
    // Operating state
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double gridVoltage = 0.0;
    double cathodeVoltage = 0.0;
    double plateVoltage = 0.0;
    double current = 0.0;
    
    // Initialize parameters based on tube type
    void updateParams();
    
    // Process the signal through the grounded cathode amplifier
    void processSignal();
};

// Class for grounded grid amplifier (common grid)
class GroundedGridAmp : public ElectricNodeBase {
public:
    GroundedGridAmp(const std::string& tubeType = "6DJ8");
    virtual ~GroundedGridAmp() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure amplifier parameters
    void setTubeType(const std::string& type) { tubeType = type; updateParams(); }
    void setPlateResistor(double resistance) { plateResistor = resistance; updateParams(); }
    void setCathodeResistor(double resistance) { cathodeResistor = resistance; updateParams(); }
    void setBPlusVoltage(double voltage) { bPlusVoltage = voltage; }
    void setGridBias(double bias) { gridBias = bias; }  // Grid bias for this configuration
    
    // Get parameters
    double getPlateResistor() const { return plateResistor; }
    double getCathodeResistor() const { return cathodeResistor; }
    double getBPlusVoltage() const { return bPlusVoltage; }
    double getGridBias() const { return gridBias; }
    
    // Get gain and impedance characteristics
    double getGain() const { return calculatedGain; }
    double getInputImpedance() const { return inputImpedance; }
    double getOutputImpedance() const { return outputImpedance; }

private:
    std::string tubeType = "6DJ8";
    double plateResistor = 47000.0;      // 47kΩ for grounded grid
    double cathodeResistor = 820.0;      // Lower cathode resistor for grounded grid
    double bPlusVoltage = 250.0;
    double gridBias = 0.0;               // Grid at ground potential (or biased to ground)
    
    // Calculated parameters
    double calculatedGain = 15.0;        // Moderate gain, non-inverting
    double inputImpedance = 1000.0;      // Lower input impedance than common cathode
    double outputImpedance = 47000.0;    // Close to plate resistor value
    
    // Tube model for simulation
    std::unique_ptr<TubeModel> tubeModel;
    
    // Pin connections
    int inputPin = 0;                    // Input to cathode
    int outputPin = 1;                   // Output from plate
    int gridPin = 2;                     // Grid grounded
    int cathodePin = 0;                  // Input connected to cathode
    int platePin = 1;                    // Output taken from plate
    int bPlusPin = 3;
    int groundPin = 4;
    int gridGroundPin = 2;               // Additional pin for grid ground
    
    // Operating state
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double gridVoltage = 0.0;
    double cathodeVoltage = 0.0;
    double plateVoltage = 0.0;
    double current = 0.0;
    
    // Initialize parameters based on tube type
    void updateParams();
    
    // Process the signal through the grounded grid amplifier
    void processSignal();
};

// Class for long-tailed pair (differential amplifier)
class LongTailedPair : public ElectricNodeBase {
public:
    LongTailedPair(const std::string& tubeType = "12AX7");
    virtual ~LongTailedPair() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure differential pair parameters
    void setTubeType(const std::string& type) { tubeType = type; updateParams(); }
    void setPlateResistors(double resistance) { plateResistor = resistance; updateParams(); }
    void setTailResistor(double resistance) { tailResistor = resistance; updateParams(); }
    void setBPlusVoltage(double voltage) { bPlusVoltage = voltage; }
    void setOperatingPoint(double bias) { operatingBias = bias; }
    
    // Get parameters
    double getPlateResistors() const { return plateResistor; }
    double getTailResistor() const { return tailResistor; }
    double getBPlusVoltage() const { return bPlusVoltage; }
    
    // Get gain and common-mode rejection characteristics
    double getDifferentialGain() const { return diffGain; }
    double getCommonModeRejection() const { return cmrr; }

private:
    std::string tubeType = "12AX7";
    double plateResistor = 100000.0;     // 100kΩ plate resistors
    double tailResistor = 220000.0;      // 220kΩ tail resistor (or current source)
    double bPlusVoltage = 250.0;
    double operatingBias = -1.5;
    
    // Calculated parameters
    double diffGain = -35.0;             // Differential gain
    double cmrr = 30.0;                  // Common-mode rejection ratio (dB)
    
    // Tube models for the two triodes
    std::unique_ptr<TubeModel> tubeModel1;
    std::unique_ptr<TubeModel> tubeModel2;
    
    // Pin connections
    int input1Pin = 0;                   // Non-inverting input
    int input2Pin = 1;                   // Inverting input
    int output1Pin = 2;                  // Non-inverting output
    int output2Pin = 3;                  // Inverting output
    int grid1Pin = 0;                    // First tube grid
    int grid2Pin = 1;                    // Second tube grid
    int cathode1Pin = 4;                 // First tube cathode
    int cathode2Pin = 5;                 // Second tube cathode, connected together
    int plate1Pin = 2;                   // First tube plate output
    int plate2Pin = 3;                   // Second tube plate output
    int bPlusPin = 6;
    int groundPin = 7;
    
    // Operating state
    double input1Signal = 0.0;
    double input2Signal = 0.0;
    double output1Signal = 0.0;
    double output2Signal = 0.0;
    double cathodeCommonVoltage = 0.0;   // Common cathode voltage
    double current1 = 0.0;
    double current2 = 0.0;
    
    // Initialize parameters based on tube type
    void updateParams();
    
    // Process the signal through the long-tailed pair
    void processSignal();
};

// Class for phase inverter circuits (common in tube amps)
class PhaseInverter : public ElectricNodeBase {
public:
    enum InverterType {
        SPLIT_LOAD,       // Simple split-load phase inverter
        CATHODE_COUPLED,  // Cathode-coupled (concertina) phase inverter
        DIFFERENTIAL      // Differential pair phase inverter
    };
    
    PhaseInverter(InverterType type = DIFFERENTIAL, const std::string& tubeType = "12AX7");
    virtual ~PhaseInverter() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure phase inverter parameters
    void setInverterType(InverterType type) { inverterType = type; updateParams(); }
    void setTubeType(const std::string& type) { tubeType = type; updateParams(); }
    void setPlateResistors(double resistance) { plateResistor = resistance; updateParams(); }
    void setCathodeResistor(double resistance) { cathodeResistor = resistance; updateParams(); }
    void setBPlusVoltage(double voltage) { bPlusVoltage = voltage; }
    
    // Get parameters
    double getPlateResistors() const { return plateResistor; }
    double getCathodeResistor() const { return cathodeResistor; }
    double getBPlusVoltage() const { return bPlusVoltage; }
    
    // Get performance characteristics
    double getPhaseBalance() const { return phaseBalance; }
    double getGainBalance() const { return gainBalance; }

private:
    InverterType inverterType;
    std::string tubeType = "12AX7";
    double plateResistor = 100000.0;     // 100kΩ plate resistors
    double cathodeResistor = 8200.0;     // 8.2kΩ cathode resistor
    double bPlusVoltage = 250.0;
    
    // Calculated parameters
    double phaseBalance = 180.0;         // Degrees phase difference
    double gainBalance = 1.0;            // Ratio of gains
    
    // Tube model(s) depending on type
    std::unique_ptr<TubeModel> tubeModel1;
    std::unique_ptr<TubeModel> tubeModel2;  // For differential type
    
    // Pin connections (vary by type)
    int inputPin = 0;
    int outputInPhasePin = 1;            // In-phase output
    int outputOutOfPhasePin = 2;         // 180-degree out-of-phase output
    int bPlusPin = 3;
    int groundPin = 4;
    
    // Operating state
    double inputSignal = 0.0;
    double outputInPhase = 0.0;
    double outputOutOfPhase = 0.0;
    
    // Initialize parameters based on type and tube
    void updateParams();
    
    // Process the signal through the phase inverter
    void processSignal();
    
    // Specific implementations for each type
    void processSplitLoad();
    void processCathodeCoupled();
    void processDifferential();
};

// Class to represent a complete tube stage with proper biasing
class TubeStage : public ElectricNodeBase {
public:
    enum StageType {
        VOLTAGE_AMPLIFIER,
        CURRENT_BUFFER,
        VOLTAGE_BUFFER,
        PHASE_SPLITTER
    };
    
    TubeStage(StageType type, const std::string& tubeType = "12AX7");
    virtual ~TubeStage() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Get access to internal components
    CathodeFollower* getCathodeFollower() { return cathodeFollower.get(); }
    GroundedCathodeAmp* getGroundedCathodeAmp() { return groundedCathodeAmp.get(); }
    LongTailedPair* getLongTailedPair() { return longTailedPair.get(); }
    PhaseInverter* getPhaseInverter() { return phaseInverter.get(); }
    
    // Get stage parameters
    double getGain() const { return calculatedGain; }
    double getOutputImpedance() const { return outputImpedance; }
    double getInputImpedance() const { return inputImpedance; }

private:
    StageType stageType;
    std::string tubeType = "12AX7";
    
    // Internal circuit components based on stage type
    std::unique_ptr<CathodeFollower> cathodeFollower;
    std::unique_ptr<GroundedCathodeAmp> groundedCathodeAmp;
    std::unique_ptr<LongTailedPair> longTailedPair;
    std::unique_ptr<PhaseInverter> phaseInverter;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int bPlusPin = 2;
    int groundPin = 3;
    
    // Operating parameters
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double calculatedGain = 1.0;
    double outputImpedance = 100000.0;
    double inputImpedance = 1000000.0;
    
    // Initialize based on stage type
    void initializeStage(StageType type);
    
    // Process the signal
    void processSignal();
};

#endif // TUBE_CIRCUIT_TOPOLOGIES_H