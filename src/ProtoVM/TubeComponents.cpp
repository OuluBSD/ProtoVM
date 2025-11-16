#include "TubeComponents.h"
#include "TubeModels.h"
#include <cmath>

// TubeComponent implementation
TubeComponent::TubeComponent() {
    // Initialize with a default triode model
    tubeModel = std::make_unique<TriodeModel>();
}

bool TubeComponent::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        updateTubeState();
        return true;
    }
    return false;
}

bool TubeComponent::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 0) {
        if (data_bytes == 1) {
            // Simple voltage assignment - each byte represents voltage at a pin
            // This is a simplified representation; in a real implementation, 
            // we'd need proper pin mapping
            return true;
        }
    }
    return false;
}

bool TubeComponent::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 0) {
        if (data_bytes == 1) {
            // Return output voltage (anode voltage in this simplified model)
            double outputVoltage = anodeVoltage;
            data[0] = static_cast<byte>(outputVoltage * 10); // Scale for byte representation
            return true;
        }
    }
    return false;
}

// TriodeComponent implementation
TriodeComponent::TriodeComponent() {
    tubeModel = std::make_unique<TriodeModel>();
    tubeModel->setAmplificationFactor(100.0);
    tubeModel->setTransconductance(0.00165);
}

void TriodeComponent::updateTubeState() {
    // Calculate voltages relative to cathode (assuming cathode is at 0V)
    // This is a simplified model - in reality, all voltages are differential
    double v_gk = gridVoltage - cathodeVoltage;  // Grid-to-cathode voltage
    double v_ak = anodeVoltage - cathodeVoltage; // Anode-to-cathode voltage
    
    // Calculate anode current using the triode model
    anodeCurrent = dynamic_cast<TriodeModel*>(tubeModel.get())->calculateAnodeCurrent(v_gk, v_ak);
    
    // For this simulation, we'll update the anode voltage based on current and load
    // This is a very simplified representation for demonstration
    double loadResistor = 100000.0; // 100k load resistor
    anodeVoltage = 250.0 - anodeCurrent * loadResistor;  // Assuming 250V supply
}

// PentodeComponent implementation
PentodeComponent::PentodeComponent() {
    tubeModel = std::make_unique<PentodeModel>();
    tubeModel->setAmplificationFactor(95.0);
    tubeModel->setTransconductance(0.007);
}

void PentodeComponent::updateTubeState() {
    // Calculate voltages relative to cathode
    double v_gk = gridVoltage - cathodeVoltage;  // Grid-to-cathode voltage
    double v_ak = anodeVoltage - cathodeVoltage; // Anode-to-cathode voltage
    
    // Calculate anode current using the pentode model
    anodeCurrent = dynamic_cast<PentodeModel*>(tubeModel.get())->calculateAnodeCurrent(v_gk, v_ak);
    
    // Calculate screen current
    screenCurrent = dynamic_cast<PentodeModel*>(tubeModel.get())->calculateScreenCurrent(v_gk, screenVoltage);
    
    // Update anode voltage based on current
    double loadResistor = 100000.0;
    anodeVoltage = 250.0 - anodeCurrent * loadResistor;
}

// TetrodeComponent implementation
TetrodeComponent::TetrodeComponent() {
    tubeModel = std::make_unique<TetrodeModel>();
    tubeModel->setAmplificationFactor(140.0);
    tubeModel->setTransconductance(0.005);
}

void TetrodeComponent::updateTubeState() {
    // Calculate voltages relative to cathode
    double v_gk = gridVoltage - cathodeVoltage;  // Grid-to-cathode voltage
    double v_ak = anodeVoltage - cathodeVoltage; // Anode-to-cathode voltage
    
    // Calculate anode current using the tetrode model
    anodeCurrent = dynamic_cast<TetrodeModel*>(tubeModel.get())->calculateAnodeCurrent(v_gk, v_ak);
    
    // Calculate screen current
    screenCurrent = dynamic_cast<TetrodeModel*>(tubeModel.get())->calculateScreenCurrent(v_gk, screenVoltage);
    
    // Update anode voltage based on current
    double loadResistor = 100000.0;
    anodeVoltage = 250.0 - anodeCurrent * loadResistor;
}

// TubeAmplifierStage implementation
TubeAmplifierStage::TubeAmplifierStage(VacuumTube::TubeType type) {
    tube = std::make_unique<VacuumTube>(type);
    
    // Set typical values for an audio amplifier stage
    plateResistor = 100000.0;   // 100k plate resistor
    cathodeResistor = 1500.0;   // 1.5k cathode resistor for self-bias
}

bool TubeAmplifierStage::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        calculateOutput();
        return true;
    }
    return false;
}

bool TubeAmplifierStage::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        // Input signal to the grid
        memcpy(&inputVoltage, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        // B+ supply voltage
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    } else if (conn_id == screenSupplyPin && data_bytes == sizeof(double)) {
        // Screen supply voltage (for pentodes/tetrodes)
        memcpy(&screenVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeAmplifierStage::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        // Output signal from the plate
        memcpy(data, &outputVoltage, sizeof(double));
        return true;
    }
    return false;
}

void TubeAmplifierStage::calculateOutput() {
    // Get the tube model
    auto tubeModel = tube->getTubeModel();
    
    // Calculate grid voltage relative to cathode
    // In a real amp, the cathode would be at some positive voltage due to cathode resistor
    double cathodeVoltage = 0.0; // Simplified - in real circuits, this would be calculated
    if (tube->getTubeModel()) {  // If tube exists
        // For this example, we'll just use the input voltage directly
        // In a real circuit, we'd calculate the bias point
        double gridVoltage = inputVoltage;
        
        // Update the tube with the calculated voltages
        tube->updateState(gridVoltage, bPlusVoltage, screenVoltage);
        
        // Calculate plate voltage based on the current and load
        double anodeCurrent = tube->getAnodeCurrent();
        outputVoltage = bPlusVoltage - anodeCurrent * plateResistor;
    }
}