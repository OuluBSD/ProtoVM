#include "SimulationBridge.h"
#include <iostream>

SimulationBridge::SimulationBridge(ISimulationEngine* engine, CircuitCanvas* canvas)
    : m_engine(engine), m_canvas(canvas)
{
    // Initialize the bridge between GUI and simulation
}

SimulationBridge::~SimulationBridge()
{
    // Clean up mappings
    m_guiToSimMap.clear();
    m_simToGuiMap.clear();
    m_pinToNodeMap.clear();
}

void SimulationBridge::Initialize()
{
    // This method can be a placeholder or removed if not needed
    // The main initialization happens in InitializeSimulation
}

void SimulationBridge::UpdateGUI()
{
    if (!m_canvas) {
        return;
    }
    
    // Update wire states based on simulation results
    for (Wire* guiWire : m_canvas->GetWires()) {
        // For this example, we'll just toggle wire states periodically
        // In a real implementation, we would get the actual state from the simulation
        static int stateCounter = 0;
        guiWire->SetActive(stateCounter % 2 == 0);
    }
    static int stateCounter = 0;
    stateCounter++;
    
    // Update the canvas to reflect changes
    m_canvas->Refresh();
}

void SimulationBridge::AddComponent(Component* guiComponent, ISimulationComponent* simComponent)
{
    if (guiComponent && simComponent) {
        m_guiToSimMap[guiComponent] = simComponent;
        m_simToGuiMap[simComponent] = guiComponent;
    }
}

void SimulationBridge::RemoveComponent(Component* guiComponent)
{
    if (!guiComponent) return;

    auto it = m_guiToSimMap.find(guiComponent);
    if (it != m_guiToSimMap.end()) {
        ISimulationComponent* simComponent = it->second;
        m_simToGuiMap.erase(simComponent);
        m_guiToSimMap.erase(it);
    }
}

ISimulationComponent* SimulationBridge::GetSimComponent(Component* guiComponent) const
{
    if (!guiComponent) return nullptr;

    auto it = m_guiToSimMap.find(guiComponent);
    if (it != m_guiToSimMap.end()) {
        return it->second;
    }
    return nullptr;
}

Component* SimulationBridge::GetGUIComponent(ISimulationComponent* simComponent) const
{
    if (!simComponent) return nullptr;

    auto it = m_simToGuiMap.find(simComponent);
    if (it != m_simToGuiMap.end()) {
        return it->second;
    }
    return nullptr;
}

void SimulationBridge::SynchronizeGUItoSimulation()
{
    if (!m_engine || !m_canvas) {
        return;
    }

    // This function would synchronize any changes made in the GUI to the simulation
    // For example, adding/removing components or changing properties
}

void SimulationBridge::SynchronizeSimulationToGUI()
{
    if (!m_engine || !m_canvas) {
        return;
    }

    // This function would synchronize the simulation state back to the GUI
    // For example, updating component states or wire values based on simulation
}

void SimulationBridge::RunSimulationStep()
{
    if (!m_engine) {
        return;
    }

    // Run a single step of the simulation
    m_engine->Tick();

    // Update the GUI to reflect the simulation state
    UpdateGUI();
}

void SimulationBridge::InitializeSimulation()
{
    if (!m_engine || !m_canvas) {
        return;
    }

    // Clear any existing mappings
    m_guiToSimMap.clear();
    m_simToGuiMap.clear();
    m_pinToNodeMap.clear();

    // Create simulation components for each GUI component
    for (Component* guiComp : m_canvas->GetComponents()) {
        ISimulationComponent* simComp = nullptr;

        // Create corresponding simulation component based on GUI component type
        if (guiComp->GetName() == "NAND") {
            simComp = m_engine->CreateComponent("NAND");
        } else if (guiComp->GetName() == "NOR") {
            simComp = m_engine->CreateComponent("NOR");
        } else if (guiComp->GetName() == "NOT") {
            simComp = m_engine->CreateComponent("NOT");
        } else if (guiComp->GetName() == "BUF") {
            simComp = m_engine->CreateComponent("BUF");
        } else {
            // Default to a basic gate
            simComp = m_engine->CreateComponent("BUF");
        }

        if (simComp) {
            // Add to mapping
            AddComponent(guiComp, simComp);
            
            // Initialize the simulation component properties based on GUI component properties
            // For example, set position, name, etc.
            simComp->SetName(guiComp->GetName().ToStdString() + " (sim)");
        }
    }

    // Now connect simulation components based on GUI wire connections
    for (Wire* guiWire : m_canvas->GetWires()) {
        GuiPin* startPin = guiWire->GetStartPin();
        GuiPin* endPin = guiWire->GetEndPin();

        // Find which GUI components these pins belong to
        Component* startComp = nullptr;
        Component* endComp = nullptr;

        for (Component* comp : m_canvas->GetComponents()) {
            auto& inputPins = comp->GetInputPins();
            auto& outputPins = comp->GetOutputPins();

            // Check if start pin belongs to this component
            for (const auto& pin : inputPins) {
                if (&pin == startPin) {
                    startComp = comp;
                }
            }
            for (const auto& pin : outputPins) {
                if (&pin == startPin) {
                    startComp = comp;
                }
            }
            
            // Check if end pin belongs to this component
            for (const auto& pin : inputPins) {
                if (&pin == endPin) {
                    endComp = comp;
                }
            }
            for (const auto& pin : outputPins) {
                if (&pin == endPin) {
                    endComp = comp;
                }
            }
        }

        // If both components were found, connect their simulation counterparts
        if (startComp && endComp) {
            ISimulationComponent* simStart = GetSimComponent(startComp);
            ISimulationComponent* simEnd = GetSimComponent(endComp);

            if (simStart && simEnd) {
                // For this simplified interface, we'll connect output 0 of start to input 0 of end
                m_engine->ConnectComponents(simStart, 0, simEnd, 0);
            }
        }
    }
}

void SimulationBridge::ResetSimulation()
{
    // Reset the simulation to initial state
    if (m_engine) {
        m_engine->Reset();
    }
    
    // Also reset the GUI component states
    if (m_canvas) {
        for (Wire* wire : m_canvas->GetWires()) {
            wire->SetActive(false);
        }
        m_canvas->Refresh();
    }
}