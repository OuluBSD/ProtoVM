#ifndef SIMULATION_BRIDGE_H
#define SIMULATION_BRIDGE_H

#include "CircuitCanvas.h"
#include "SimulationInterface.h"
#include <unordered_map>
#include <vector>

// Forward declarations
class CircuitCanvas;

/**
 * @brief The SimulationBridge class provides connection between GUI components and the simulation engine
 * 
 * This class maintains mappings between GUI components and their corresponding simulation components,
 * allowing the GUI to visualize simulation states and control the simulation.
 */
class SimulationBridge
{
public:
    SimulationBridge(ISimulationEngine* engine, CircuitCanvas* canvas);
    ~SimulationBridge();

    /**
     * @brief Initialize the bridge by connecting GUI components to simulation components
     */
    void Initialize();

    /**
     * @brief Update the GUI to reflect the current simulation state
     */
    void UpdateGUI();

    /**
     * @brief Add a new component to the bridge
     */
    void AddComponent(Component* guiComponent, ISimulationComponent* simComponent);

    /**
     * @brief Remove a component from the bridge
     */
    void RemoveComponent(Component* guiComponent);

    /**
     * @brief Get the simulation component corresponding to a GUI component
     */
    ISimulationComponent* GetSimComponent(Component* guiComponent) const;

    /**
     * @brief Get the GUI component corresponding to a simulation component
     */
    Component* GetGUIComponent(ISimulationComponent* simComponent) const;

    /**
     * @brief Synchronize changes from GUI to simulation
     */
    void SynchronizeGUItoSimulation();

    /**
     * @brief Synchronize changes from simulation to GUI
     */
    void SynchronizeSimulationToGUI();

    /**
     * @brief Run a single simulation step and update GUI
     */
    void RunSimulationStep();

    /**
     * @brief Initialize the simulation with the current circuit
     */
    void InitializeSimulation();

    /**
     * @brief Reset the simulation to initial state
     */
    void ResetSimulation();

    /**
     * @brief Get the associated engine (simulation engine)
     */
    ISimulationEngine* GetEngine() { return m_engine; }

    /**
     * @brief Get the associated canvas (GUI)
     */
    CircuitCanvas* GetCanvas() { return m_canvas; }

private:
    // Pointer to the simulation engine
    ISimulationEngine* m_engine;

    // Pointer to the GUI canvas
    CircuitCanvas* m_canvas;

    // Mappings between GUI and simulation components
    std::unordered_map<Component*, ISimulationComponent*> m_guiToSimMap;
    std::unordered_map<ISimulationComponent*, Component*> m_simToGuiMap;

    // Mappings between GUI pins and simulation nodes
    std::unordered_map<GuiPin*, ISimulationComponent*> m_pinToNodeMap;
};

#endif // SIMULATION_BRIDGE_H