#ifndef TUBE_COMPONENTS_H
#define TUBE_COMPONENTS_H

#include "ElectricNodeBase.h"
#include "TubeModels.h"
#include <memory>

// Forward declarations
class TubeModel;

// Base class for tube components in ProtoVM
class TubeComponent : public ElectricNodeBase {
public:
    TubeComponent();
    virtual ~TubeComponent() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // Get tube model
    TubeModel* getTubeModel() { return tubeModel.get(); }
    
    // Update tube state based on pin voltages
    virtual void updateTubeState() = 0;
    
protected:
    std::unique_ptr<TubeModel> tubeModel;
    
    // Pin connections - will be assigned during initialization
    int cathodePin = -1;
    int anodePin = -1;
    int gridPin = -1;
    int screenGridPin = -1;  // For pentodes and tetrodes
    int suppressorGridPin = -1;  // For pentodes
    
    // Internal voltages
    double cathodeVoltage = 0.0;
    double anodeVoltage = 0.0;
    double gridVoltage = 0.0;
    double screenVoltage = 0.0;
    
    // Internal currents
    double anodeCurrent = 0.0;
    double gridCurrent = 0.0;
    double screenCurrent = 0.0;
};

// Triode component
class TriodeComponent : public TubeComponent {
public:
    TriodeComponent();
    virtual ~TriodeComponent() = default;
    
    virtual void updateTubeState() override;
};

// Pentode component
class PentodeComponent : public TubeComponent {
public:
    PentodeComponent();
    virtual ~PentodeComponent() = default;
    
    virtual void updateTubeState() override;
};

// Tetrode component
class TetrodeComponent : public TubeComponent {
public:
    TetrodeComponent();
    virtual ~TetrodeComponent() = default;
    
    virtual void updateTubeState() override;
};

// Common tube-based circuits
class TubeAmplifierStage : public ElectricNodeBase {
public:
    TubeAmplifierStage(VacuumTube::TubeType type = VacuumTube::TRIODE);
    virtual ~TubeAmplifierStage() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // Get the internal tube
    VacuumTube* getTube() { return tube.get(); }
    
    // Set component values
    void setPlateResistor(double r) { plateResistor = r; }
    void setCathodeResistor(double r) { cathodeResistor = r; }
    void setScreenResistor(double r) { screenResistor = r; }  // For pentodes/tetrodes
    
    // Get component values
    double getPlateResistor() const { return plateResistor; }
    double getCathodeResistor() const { return cathodeResistor; }
    double getScreenResistor() const { return screenResistor; }
    
private:
    std::unique_ptr<VacuumTube> tube;
    
    // Associated passive components
    double plateResistor = 100000.0;    // 100kΩ plate load
    double cathodeResistor = 1500.0;    // 1.5kΩ cathode resistor for auto bias
    double screenResistor = 100000.0;   // 100kΩ screen load (for pentodes/tetrodes)
    
    // Pin connections for the stage
    int inputPin = -1;
    int outputPin = -1;
    int bPlusPin = -1;        // High voltage supply
    int groundPin = -1;       // Ground reference
    int screenSupplyPin = -1; // Screen supply for pentodes/tetrodes
    
    // Operating point voltages
    double inputVoltage = 0.0;
    double outputVoltage = 0.0;
    double bPlusVoltage = 250.0;  // Typical B+ voltage
    double screenVoltage = 100.0; // Typical screen voltage for pentodes
    
    // Calculate the output voltage based on tube operation
    void calculateOutput();
};

#endif // TUBE_COMPONENTS_H