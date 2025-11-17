#ifndef TUBE_AMPLIFIER_COMPONENT_H
#define TUBE_AMPLIFIER_COMPONENT_H

#include "Common.h"
#include "TubeDistortion.h"
#include "TubeComponents.h"
#include <vector>
#include <memory>

// Component that simulates a complete tube amplifier with distortion modeling
class TubeAmplifierComponent : public ElectricNodeBase {
public:
    TubeAmplifierComponent();
    virtual ~TubeAmplifierComponent() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Configure the amplifier
    void addPreamplifierStage(const std::string& tubeType = "12AX7", double gain = 30.0, double drive = 1.0);
    void addPhaseSplitterStage(const std::string& tubeType = "12AX7");
    void addPowerAmplifierStage(const std::string& tubeType = "EL34", int numTubes = 2);
    void setToneControls(double bass, double mid, double treble);
    void setPresenceControl(double presence) { simulator.setPresenceControl(presence); }
    void setMasterVolume(double volume) { simulator.setMasterVolume(volume); }
    void setInputLevel(double level) { simulator.setInputLevel(level); }
    
    // Get output
    double getOutput() const { return currentOutput; }
    
    // Get reference to the simulator for more advanced configuration
    TubeAmplifierSimulation* getSimulator() { return &simulator; }

private:
    TubeAmplifierSimulation simulator;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int bPlusPin = 2;   // High voltage supply
    int groundPin = 3;  // Ground reference
    
    // Operational parameters
    double inputSignal = 0.0;
    double currentOutput = 0.0;
    double bPlusVoltage = 250.0;  // Default B+ voltage
    
    // Process the input signal through the amplifier model
    void processSignal();
};

// Component for specific tube circuits
class TubeCircuitComponent : public ElectricNodeBase {
public:
    enum CircuitType {
        CATHODE_FOLLOWER,
        DIFFERENTIAL_PAIR,
        LONG_TAILED_PAIR,
        CLASS_A_SINGLE_ENDED,
        CLASS_A_PUSH_PULL,
        CLASS_AB_PUSH_PULL
    };
    
    TubeCircuitComponent(CircuitType type);
    virtual ~TubeCircuitComponent() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Set circuit parameters
    void setTubeType(const std::string& type) { tubeConfig.setTubeType(type); }
    void setConfiguration(TubeConfigurationModel::Configuration config) { 
        tubeConfig.setConfiguration(config); 
    }
    
    // Get output
    double getOutput() const { return currentOutput; }

private:
    TubeConfigurationModel tubeConfig;
    CircuitType circuitType;
    double inputSignal = 0.0;
    double currentOutput = 0.0;
    
    // Pin connections vary by circuit type
    std::vector<int> inputPins;   // Multiple inputs for differential circuits
    int outputPin = 1;
    int supplyPin = 2;            // Power supply
    int groundPin = 3;            // Ground reference
    
    void processSignal();
};

#endif // TUBE_AMPLIFIER_COMPONENT_H