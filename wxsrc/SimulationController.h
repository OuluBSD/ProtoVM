#ifndef SIMULATIONCONTROLLER_H
#define SIMULATIONCONTROLLER_H

#include <wx/wx.h>
#include <vector>
#include <map>
#include <functional>

// Forward declarations
class Component;
class Wire;

struct SimulationState {
    // Represents the state of a component or wire at a given time
    bool value;  // Logic value (true for high, false for low)
    double voltage;  // Actual voltage value (for analog simulation)
    int strength;   // Drive strength (0=high-Z, 1=pull-down, 2=pull-up, 3=strong drive)
    long timestamp; // Time when this state was computed
};

class SimulationController : public wxEvtHandler {
public:
    SimulationController();
    
    void SetCanvas(void* canvas); // void* to avoid including CircuitCanvas in header
    void SetSimulationBridge(class SimulationBridge* bridge) { m_simulationBridge = bridge; }
    
    // Simulation control methods
    void StartSimulation();
    void StopSimulation();
    void PauseSimulation();
    void ResetSimulation();
    void StepSimulation();  // Single step
    
    // Get/set simulation parameters
    void SetSimulationSpeed(int speed);  // 1-10 scale
    int GetSimulationSpeed() const { return m_speed; }
    bool IsRunning() const { return m_running; }
    bool IsPaused() const { return m_paused; }
    
    // Signal visualization methods
    SimulationState GetComponentState(void* component) const;
    SimulationState GetWireState(void* wire) const;
    void UpdateSignalVisualization();
    
    // Methods to handle circuit changes during simulation
    void CircuitChanged();
    
    // Set callback for UI updates during simulation
    void SetUpdateCallback(std::function<void()> callback) { m_updateCallback = callback; }

private:
    bool m_running;
    bool m_paused;
    int m_speed;
    long m_currentTime;
    
    // Map to store states of components and wires
    std::map<void*, SimulationState> m_componentStates;
    std::map<void*, SimulationState> m_wireStates;
    
    // Simulation timer
    wxTimer* m_simulationTimer;
    
    // Canvas reference (as void* to avoid header dependency)
    void* m_canvas;
    class SimulationBridge* m_simulationBridge;
    
    // Callback for UI updates
    std::function<void()> m_updateCallback;
    
    // Event handler for timer
    void OnSimulationTimer(wxTimerEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};

#endif // SIMULATIONCONTROLLER_H