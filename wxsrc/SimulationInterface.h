#ifndef SIMULATION_INTERFACE_H
#define SIMULATION_INTERFACE_H

#include <string>
#include <vector>

// Abstract interface for simulation components
class ISimulationComponent {
public:
    virtual ~ISimulationComponent() = default;
    
    virtual std::string GetName() const = 0;
    virtual void SetName(const std::string& name) = 0;
    
    // Methods for component state
    virtual void SetInput(int pinIndex, bool value) = 0;
    virtual bool GetOutput(int pinIndex) const = 0;
    virtual void Process() = 0;
};

// Abstract interface for the simulation machine/engine
class ISimulationEngine {
public:
    virtual ~ISimulationEngine() = default;
    
    virtual ISimulationComponent* CreateComponent(const std::string& type) = 0;
    virtual void ConnectComponents(ISimulationComponent* output, int outputPin, 
                                   ISimulationComponent* input, int inputPin) = 0;
    virtual bool Tick() = 0;
    virtual void Reset() = 0;
    virtual void Clear() = 0;
    virtual int GetCurrentTick() const = 0;
};

#endif // SIMULATION_INTERFACE_H