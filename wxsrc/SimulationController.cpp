#include "SimulationController.h"
#include "CircuitCanvas.h"
#include <wx/timer.h>

wxBEGIN_EVENT_TABLE(SimulationController, wxPanel)
    EVT_TIMER(wxID_ANY, SimulationController::OnSimulationTimer)
wxEND_EVENT_TABLE()

SimulationController::SimulationController()
    : m_running(false), m_paused(false), m_speed(5), m_currentTime(0), m_simulationTimer(nullptr), m_canvas(nullptr)
{
    m_simulationTimer = new wxTimer(this);
}

void SimulationController::SetCanvas(void* canvas)
{
    m_canvas = canvas;
}

void SimulationController::StartSimulation()
{
    if (!m_running) {
        m_running = true;
        m_paused = false;
        
        // Start the simulation timer
        int interval = 1000 / (m_speed * 2); // Convert speed to milliseconds per step
        m_simulationTimer->Start(interval);
    } else if (m_paused) {
        m_paused = false;
        int interval = 1000 / (m_speed * 2);
        m_simulationTimer->Start(interval);
    }
}

void SimulationController::StopSimulation()
{
    if (m_running) {
        m_running = false;
        m_paused = false;
        m_simulationTimer->Stop();
    }
}

void SimulationController::PauseSimulation()
{
    if (m_running && !m_paused) {
        m_paused = true;
        m_simulationTimer->Stop();
    }
}

void SimulationController::ResetSimulation()
{
    StopSimulation();
    m_currentTime = 0;
    
    // Reset all component and wire states to initial values
    for (auto& pair : m_componentStates) {
        pair.second.value = false;
        pair.second.voltage = 0.0;
        pair.second.strength = 0;
    }
    
    for (auto& pair : m_wireStates) {
        pair.second.value = false;
        pair.second.voltage = 0.0;
        pair.second.strength = 0;
    }
    
    // Call the update callback to refresh the UI
    if (m_updateCallback) {
        m_updateCallback();
    }
}

void SimulationController::StepSimulation()
{
    if (m_running && m_paused) {
        // Perform a single simulation step
        // In a real implementation, this would compute the next state of the circuit
        
        // For now, we'll just toggle some values to demonstrate
        m_currentTime++;
        
        // Update the callback to refresh UI
        if (m_updateCallback) {
            m_updateCallback();
        }
    }
}

void SimulationController::SetSimulationSpeed(int speed)
{
    m_speed = wxMax(1, wxMin(speed, 10)); // Clamp between 1 and 10
    
    if (m_running && !m_paused) {
        // Restart timer with new interval
        m_simulationTimer->Stop();
        int interval = 1000 / (m_speed * 2);
        m_simulationTimer->Start(interval);
    }
}

SimulationState SimulationController::GetComponentState(void* component) const
{
    auto it = m_componentStates.find(component);
    if (it != m_componentStates.end()) {
        return it->second;
    }
    
    // Return default state if not found
    SimulationState defaultState;
    defaultState.value = false;
    defaultState.voltage = 0.0;
    defaultState.strength = 0;
    defaultState.timestamp = m_currentTime;
    return defaultState;
}

SimulationState SimulationController::GetWireState(void* wire) const
{
    auto it = m_wireStates.find(wire);
    if (it != m_wireStates.end()) {
        return it->second;
    }
    
    // Return default state if not found
    SimulationState defaultState;
    defaultState.value = false;
    defaultState.voltage = 0.0;
    defaultState.strength = 0;
    defaultState.timestamp = m_currentTime;
    return defaultState;
}

void SimulationController::UpdateSignalVisualization()
{
    // In a real implementation, this would update the visual representation 
    // of the circuit based on the current simulation state
    
    // For now, we'll just refresh the UI if possible
    if (m_updateCallback) {
        m_updateCallback();
    }
}

void SimulationController::CircuitChanged()
{
    // When the circuit changes, reset the simulation state
    ResetSimulation();
}

void SimulationController::OnSimulationTimer(wxTimerEvent& event)
{
    // In a real implementation, this would run one step of the simulation
    // by evaluating all components based on their inputs and updating their outputs
    
    m_currentTime++;
    
    // For demonstration purposes, we'll just toggle wire states periodically
    // In a real implementation, this would involve complex logic to evaluate
    // the circuit based on component types and connections
    
    if (m_updateCallback) {
        m_updateCallback();
    }
}