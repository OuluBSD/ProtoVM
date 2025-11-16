#ifndef _ProtoVM_ModulationMatrix_h_
#define _ProtoVM_ModulationMatrix_h_

#include "AnalogCommon.h"
#include <vector>
#include <string>
#include <functional>
#include <map>

// Forward declarations for modulation sources
class VCO;
class ADSR;
class LFO;

// Enum for modulation source types
enum class ModulationSource {
    LFO1,
    LFO2,
    LFO3,
    ADSR1,
    ADSR2,
    ENV_FOLLOW,
    KEY_TRACK,
    VELOCITY,
    AFTERTOUCH,
    WHEEL,
    GATE,
    VELOCITY_FOLLOW,
    PRESSURE,
    RANDOM,
    CUSTOM
};

// Enum for modulation destinations
enum class ModulationDestination {
    VCO1_PITCH,
    VCO2_PITCH,
    VCO3_PITCH,
    VCO_ALL_PITCH,
    VCF_CUTOFF,
    VCA_LEVEL,
    LFO1_RATE,
    LFO2_RATE,
    VCF_RESONANCE,
    VCO1_PWM,
    VCO2_PWM,
    VCO3_PWM,
    CUSTOM
};

// Structure to define a modulation connection
struct ModulationConnection {
    ModulationSource source;
    ModulationDestination destination;
    double amount;  // Modulation amount (-1.0 to 1.0, where 1.0 is 100% modulation)
    bool active;    // Whether this connection is active
    std::string name; // Name of the connection (optional)
    
    ModulationConnection(ModulationSource src, ModulationDestination dest, double amt, bool act = true, const std::string& n = "")
        : source(src), destination(dest), amount(amt), active(act), name(n) {}
};

class ModulationMatrix : public AnalogNodeBase {
public:
    typedef ModulationMatrix CLASSNAME;

    ModulationMatrix(int max_connections = 16);  // Default to 16 modulation connections
    virtual ~ModulationMatrix() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "ModulationMatrix"; }

    // Add a modulation connection
    bool AddConnection(const ModulationConnection& connection);
    
    // Remove a modulation connection by index
    bool RemoveConnection(int index);
    
    // Update an existing connection
    bool UpdateConnection(int index, double new_amount);
    
    // Clear all connections
    void ClearAllConnections();
    
    // Get number of active connections
    int GetActiveConnectionCount() const { return active_connections; }
    
    // Process modulation for a specific destination
    double ProcessModulation(ModulationDestination dest, double base_value);
    
    // Get/set the modulation amount for a specific connection
    double GetModulationAmount(int index) const;
    void SetModulationAmount(int index, double amount);
    
    // Activate/deactivate a connection
    void SetConnectionActive(int index, bool active);
    bool IsConnectionActive(int index) const;
    
    // Set modulation source values (from other components)
    void SetLFOValue(int lfo_id, double value);
    void SetADSRValue(int adsr_id, double value);
    void SetVelocityValue(double value);
    void SetAftertouchValue(double value);
    void SetWheelValue(double value);
    void SetGateValue(double value);
    void SetPressureValue(double value);
    
    // Helper functions to get modulation source names
    static std::string GetSourceName(ModulationSource source);
    static std::string GetDestinationName(ModulationDestination destination);

private:
    std::vector<ModulationConnection> connections;
    int max_connections;
    int active_connections;
    
    // Current values of modulation sources
    std::map<int, double> lfo_values;      // LFO values by ID
    std::map<int, double> adsr_values;     // ADSR values by ID
    double velocity_value;
    double aftertouch_value;
    double wheel_value;
    double gate_value;
    double pressure_value;
    
    // Apply modulation to a base value
    double ApplyModulation(double base_value, double modulation_amount, double modulation_depth);
};

#endif