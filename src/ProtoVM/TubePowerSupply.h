#ifndef TUBE_POWER_SUPPLY_H
#define TUBE_POWER_SUPPLY_H

#include "ElectricNodeBase.h"
#include <vector>
#include <memory>

// Class to simulate a tube amplifier power supply
class TubePowerSupply : public ElectricNodeBase {
public:
    enum SupplyType {
        CLASSIC_EL34,
        CLASSIC_6V6,
        CLASSIC_300B,
        FLEXIBLE_SUPPLY
    };
    
    TubePowerSupply(SupplyType type = CLASSIC_EL34);
    virtual ~TubePowerSupply() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure supply parameters
    void setTransformerRatio(double ratio) { transformerRatio = ratio; }
    void setInputVoltage(double volts) { inputVoltage = volts; }
    void setOutputVoltage(double volts) { bPlusVoltage = volts; }
    void setOutputCurrent(double amps) { maxCurrent = amps; }
    void setRegulationQuality(double quality) { regulationQuality = quality; } // 0.0=poor, 1.0=excellent
    
    // Get supply parameters
    double getInputVoltage() const { return inputVoltage; }
    double getOutputVoltage() const { return bPlusVoltage; }
    double getOutputCurrent() const { return maxCurrent; }
    
    // Simulate load regulation
    void addLoad(double currentDraw);
    void removeLoad(double currentDraw);
    
    // Get current load and voltage under load
    double getCurrentLoad() const { return totalLoad; }
    double getVoltageUnderLoad() const { return actualOutputVoltage; }
    
    // Enable/disable sag simulation
    void enableSag(bool enable) { sagEnabled = enable; }
    void setSagAmount(double amount) { sagAmount = std::max(0.0, std::min(1.0, amount)); }
    
    // Set ripple parameters
    void setRipplePercent(double percent) { ripplePercent = std::max(0.0, std::min(100.0, percent)); }
    void setRippleFrequency(double freq) { rippleFrequency = std::max(0.1, freq); }

private:
    SupplyType supplyType;
    
    // Input parameters
    double inputVoltage = 120.0;      // AC input voltage (RMS)
    double transformerRatio = 4.2;    // Typical 120V to 500V+ transformer ratio
    
    // Output parameters
    double bPlusVoltage = 250.0;      // Typical B+ voltage after rectification/filtering
    double maxCurrent = 0.1;          // Maximum current output (100mA for example)
    
    // Internal components
    double rectifierDrop = 1.4;       // Voltage drop across full-wave rectifier (2 diodes)
    double regulationQuality = 0.8;   // How well it maintains voltage under load (0.0-1.0)
    
    // Load parameters
    double totalLoad = 0.0;           // Current being drawn by connected circuits
    double actualOutputVoltage;       // Voltage under load
    std::vector<double> loadSources;  // Track individual loads
    
    // Ripple parameters
    double ripplePercent = 5.0;       // AC ripple as percentage of DC
    double rippleFrequency = 120.0;   // Ripple frequency (120Hz for full-wave rectified 60Hz)
    double rippleAmplitude;
    double ripplePhase = 0.0;
    double sampleRate = 44100.0;
    
    // Sag parameters
    bool sagEnabled = true;
    double sagAmount = 0.1;           // Amount of voltage sag under load
    double recoveryTime = 0.05;       // Time constant for voltage recovery
    
    // Pin connections
    int inputPin = 0;                 // AC input
    int bPlusPin = 1;                 // High voltage DC output
    int groundPin = 2;                // Ground reference
    int currentSensePin = 3;          // Current sensing output
    
    // State variables
    double currentOutput = 0.0;
    bool isOutputValid = false;
    
    // Initialize the supply based on type
    void initializeSupply(SupplyType type);
    
    // Calculate output voltage considering load and regulation
    void calculateOutputVoltage();
    
    // Update ripple based on current conditions
    void updateRipple();
};

// Class for tube rectifier simulation (e.g., 5Y3, 5U4)
class TubeRectifier : public ElectricNodeBase {
public:
    enum RectifierType {
        TYPE_5Y3,
        TYPE_5U4,
        TYPE_275C3,
        TYPE_GZ37
    };
    
    TubeRectifier(RectifierType type = TYPE_5Y3);
    virtual ~TubeRectifier() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Get rectifier parameters
    double getVoltageDrop() const { return voltageDrop; }
    double getMaxCurrent() const { return maxCurrent; }
    
    // Calculate rectified output
    double calculateRectifiedOutput(double acInput);

private:
    RectifierType rectifierType;
    double voltageDrop = 40.0;        // Forward voltage drop (varies by tube type)
    double maxCurrent = 0.06;         // Maximum current (60mA for 5Y3)
    double internalResistance = 750.0; // Internal resistance in ohms
    
    // Pin connections
    int acInputPin = 0;               // AC input from transformer
    int highVoltagePin = 1;           // High voltage output
    int lowVoltagePin = 2;            // Low voltage output (for full wave)
    int groundPin = 3;                // Heater/filament ground
    
    // State variables
    double currentOutput = 0.0;
    
    // Initialize rectifier parameters based on type
    void initializeRectifier(RectifierType type);
};

// Component that simulates heater supply for tubes
class TubeHeaterSupply : public ElectricNodeBase {
public:
    TubeHeaterSupply(double voltage = 6.3, double current = 0.9);
    virtual ~TubeHeaterSupply() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Get heater parameters
    double getHeaterVoltage() const { return heaterVoltage; }
    double getHeaterCurrent() const { return maxHeaterCurrent; }
    
    // Simulate heater tube load
    void addTubeHeaterLoad(double current);
    void removeTubeHeaterLoad(double current);

private:
    double heaterVoltage = 6.3;       // Typical heater voltage
    double maxHeaterCurrent = 0.9;    // Maximum heater current (900mA for example)
    double currentHeaterLoad = 0.0;   // Current load on heater supply
    bool heaterSupplyValid = true;
    
    // Pin connections
    int outputPin = 0;                // Heater voltage output
    int groundPin = 1;                // Heater ground
    
    // State variables
    double currentOutput = 0.0;
    
    // Check if heater supply can handle the load
    bool canHandleLoad(double additionalCurrent) const;
};

#endif // TUBE_POWER_SUPPLY_H