#include "CircuitCanvas.h"
#include "CircuitData.h"
#include "UndoRedo.h"
#include "SimulationController.h"
#include "CircuitAnalysis.h"  // Include circuit analysis for AnalysisResult
#include "CircuitAnalyzer.h"  // Include ConcreteCircuitAnalyzer implementation
#include <wx/dcbuffer.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>

// Forward declaration of MainFrame to access properties panel
class MainFrame;


void CircuitCanvas::OnMouseRightDown(wxMouseEvent& event)
{
    // For context menu or wire creation
    wxPoint pos = PhysicalToLogical(event.GetPosition());
    
    // Check if we right-clicked on a component's pin
    bool pinFound = false;
    for (Component* comp : m_components)
    {
        // Check input pins
        for (Pin& pin : comp->GetInputPins())
        {
            wxPoint pinPos = pin.GetPosition();
            if (wxRect(pinPos.x - 4, pinPos.y - 4, 8, 8).Contains(pos))
            {
                // Start wire creation mode from this pin
                m_wireCreationMode = true;
                m_startPin = &pin;
                m_currentWireEndPoint = pos;
                pinFound = true;
                Refresh();
                break;
            }
        }
        
        if (pinFound) break;
        
        // Check output pins
        for (Pin& pin : comp->GetOutputPins())
        {
            wxPoint pinPos = pin.GetPosition();
            if (wxRect(pinPos.x - 4, pinPos.y - 4, 8, 8).Contains(pos))
            {
                // Start wire creation mode from this pin
                m_wireCreationMode = true;
                m_startPin = &pin;
                m_currentWireEndPoint = pos;
                pinFound = true;
                Refresh();
                break;
            }
        }
        
        if (pinFound) break;
    }
    
    if (!pinFound) {
        // Right-clicked on empty space or on a component but not on a pin
        // We could show a context menu here for adding components, etc.
    }
}

void CircuitCanvas::SerializeToData(CircuitData& data) const
{
    // Clear the existing data
    data.components.clear();
    data.wires.clear();
    
    // Set default circuit name and description
    data.name = "Untitled Circuit";
    data.description = "A digital logic circuit created with ProtoVM";
    
    // Convert each component in the canvas to ComponentData
    for (size_t i = 0; i < m_components.size(); ++i)
    {
        Component* comp = m_components[i];
        
        ComponentData compData;
        compData.type = comp->GetName().ToStdString();  // In a real implementation, we'd have specific type info
        compData.name = comp->GetName().ToStdString();
        wxPoint pos = comp->GetPosition();
        compData.x = pos.x;
        compData.y = pos.y;
        
        // Add input pins
        for (const Pin& pin : comp->GetInputPins())
        {
            PinData pinData;
            pinData.name = pin.GetName().ToStdString();
            pinData.is_input = pin.IsInput();
            wxPoint pinPos = pin.GetPosition();
            pinData.x = pinPos.x;
            pinData.y = pinPos.y;
            compData.inputs.push_back(pinData);
        }
        
        // Add output pins
        for (const Pin& pin : comp->GetOutputPins())
        {
            PinData pinData;
            pinData.name = pin.GetName().ToStdString();
            pinData.is_input = pin.IsInput();
            wxPoint pinPos = pin.GetPosition();
            pinData.x = pinPos.x;
            pinData.y = pinPos.y;
            compData.outputs.push_back(pinData);
        }
        
        data.components.push_back(compData);
    }
    
    // Precompute component index mapping for efficient wire serialization
    std::unordered_map<const Component*, int> compToIndex;
    for (size_t i = 0; i < m_components.size(); ++i) {
        compToIndex[m_components[i]] = i;
    }

    // Convert each wire in the canvas to WireData with better performance
    data.wires.reserve(m_wires.size()); // Reserve space to avoid repeated allocations
    
    for (size_t i = 0; i < m_wires.size(); ++i)
    {
        Wire* wire = m_wires[i];

        // Use helper function to find components that own these pins
        Component* startComp = GetComponentForPin(wire->GetStartPin());
        Component* endComp = GetComponentForPin(wire->GetEndPin());

        auto startIt = compToIndex.find(startComp);
        auto endIt = compToIndex.find(endComp);
        
        if (startIt != compToIndex.end() && endIt != compToIndex.end())
        {
            WireData wireData;
            wireData.start_component_id = startIt->second;
            wireData.start_pin_name = wire->GetStartPin()->GetName().ToStdString();
            wireData.end_component_id = endIt->second;
            wireData.end_pin_name = wire->GetEndPin()->GetName().ToStdString();
            
            data.wires.push_back(wireData);
        }
    }
}

void CircuitCanvas::DeserializeFromData(const CircuitData& data)
{
    // Clear existing components and wires
    for (Component* comp : m_components) {
        delete comp;
    }
    for (Wire* wire : m_wires) {
        delete wire;
    }
    
    m_components.clear();
    m_wires.clear();
    
    // Create components from the data
    for (const ComponentData& compData : data.components)
    {
        Component* comp = nullptr;
        
        // Create appropriate component based on type
        if (compData.type == "NAND") {
            comp = new NANDGateComponent(compData.x, compData.y);
        } else if (compData.type == "NOR") {
            comp = new NORGateComponent(compData.x, compData.y);
        } else if (compData.type == "NOT") {
            comp = new NOTGateComponent(compData.x, compData.y);
        } else if (compData.type == "BUF") {
            comp = new BufferComponent(compData.x, compData.y);
        } else {
            // Default to a buffer if type is unknown
            comp = new BufferComponent(compData.x, compData.y);
        }
        
        if (comp) {
            m_components.push_back(comp);
        }
    }
    
    // Create wires from the data
    for (const WireData& wireData : data.wires)
    {
        if (wireData.start_component_id < m_components.size() && 
            wireData.end_component_id < m_components.size())
        {
            Component* startComp = m_components[wireData.start_component_id];
            Component* endComp = m_components[wireData.end_component_id];
            
            // Find the specific pins to connect
            Pin* startPin = nullptr;
            Pin* endPin = nullptr;
            
            // Look for the start pin in the start component
            for (Pin& pin : startComp->GetInputPins()) {
                if (pin.GetName().ToStdString() == wireData.start_pin_name) {
                    startPin = &pin;
                    break;
                }
            }
            if (!startPin) {
                for (Pin& pin : startComp->GetOutputPins()) {
                    if (pin.GetName().ToStdString() == wireData.start_pin_name) {
                        startPin = &pin;
                        break;
                    }
                }
            }
            
            // Look for the end pin in the end component
            for (Pin& pin : endComp->GetInputPins()) {
                if (pin.GetName().ToStdString() == wireData.end_pin_name) {
                    endPin = &pin;
                    break;
                }
            }
            if (!endPin) {
                for (Pin& pin : endComp->GetOutputPins()) {
                    if (pin.GetName().ToStdString() == wireData.end_pin_name) {
                        endPin = &pin;
                        break;
                    }
                }
            }
            
            // Create the wire if both pins were found
            if (startPin && endPin) {
                m_wires.push_back(new SimpleWire(startPin, endPin));
            }
        }
    }
    
    // Refresh the canvas to show the new components and wires
    Refresh();
}

void CircuitCanvas::StartAnimation()
{
    // Activate animation for all wires
    for (Wire* wire : m_wires) {
        wire->SetAnimationActive(true);
    }
    
    // Start a timer to update the animation
    if (!m_animationTimer) {
        m_animationTimer = new wxTimer(this);
        Bind(wxEVT_TIMER, &CircuitCanvas::OnAnimationTimer, this);
        m_animationTimer->Start(16); // ~60 FPS (1000ms / 60fps ≈ 16ms)
    }
}

void CircuitCanvas::StopAnimation()
{
    // Deactivate animation for all wires
    for (Wire* wire : m_wires) {
        wire->SetAnimationActive(false);
    }
    
    // Stop the animation timer
    if (m_animationTimer) {
        m_animationTimer->Stop();
        Unbind(wxEVT_TIMER, &CircuitCanvas::OnAnimationTimer, this);
        delete m_animationTimer;
        m_animationTimer = nullptr;
    }
}

void CircuitCanvas::UpdateAnimation(float deltaTime)
{
    // Update animation for all wires
    for (Wire* wire : m_wires) {
        wire->UpdateAnimation(deltaTime);
    }
    
    // Refresh the canvas to show the updated animation
    Refresh();
}

void CircuitCanvas::UpdateWireStates()
{
    // In a real implementation, this would update wire states based on component outputs
    // For now, we'll just toggle wire states periodically for demonstration
    for (size_t i = 0; i < m_wires.size(); i++) {
        // Toggle wire state every 2 seconds (for demo purposes)
        m_wires[i]->SetActive(i % 2 == 0);  // Alternate active state for each wire
    }
}

void CircuitCanvas::OnAnimationTimer(wxTimerEvent& event)
{
    // Update wire states periodically
    static int stateUpdateCounter = 0;
    stateUpdateCounter++;
    if (stateUpdateCounter % 120 == 0) { // Update states roughly every 2 seconds at 60 FPS
        UpdateWireStates();
        stateUpdateCounter = 0;
    }
    
    // Update animation with a fixed delta time (16ms for 60 FPS)
    UpdateAnimation(0.016f);
}

void CircuitCanvas::PushUndoCommand(std::unique_ptr<UndoCommand> command)
{
    if (m_undoRedoManager) {
        m_undoRedoManager->PushCommand(std::move(command));
    }
}

bool CircuitCanvas::CanUndo() const
{
    return m_undoRedoManager && m_undoRedoManager->CanUndo();
}

bool CircuitCanvas::CanRedo() const
{
    return m_undoRedoManager && m_undoRedoManager->CanRedo();
}

void CircuitCanvas::Undo()
{
    if (m_undoRedoManager) {
        m_undoRedoManager->Undo();
    }
}

void CircuitCanvas::Redo()
{
    if (m_undoRedoManager) {
        m_undoRedoManager->Redo();
    }
}

void CircuitCanvas::OnSize(wxSizeEvent& event)
{
    Refresh();
    event.Skip();
}

void CircuitCanvas::OnMouseLeftDown(wxMouseEvent& event)
{
    wxPoint pos = PhysicalToLogical(event.GetPosition());
    
    if (m_wireCreationMode)
    {
        // In wire creation mode, we're trying to connect pins
        bool pinFound = false;
        
        // Check if we clicked on any component's pin
        for (Component* comp : m_components)
        {
            // Check input pins
            for (Pin& pin : comp->GetInputPins())
            {
                wxPoint pinPos = pin.GetPosition();
                if (wxRect(pinPos.x - 4, pinPos.y - 4, 8, 8).Contains(pos))
                {
                    if (!m_startPin)
                    {
                        // This is our starting pin
                        m_startPin = &pin;
                    }
                    else if (m_startPin != &pin)  // Can't connect pin to itself
                    {
                        // This is our ending pin - create the wire
                        // Find which component the starting pin belongs to
                        Component* startComp = nullptr;
                        for (Component* c : m_components)
                        {
                            auto& inputs = c->GetInputPins();
                            auto& outputs = c->GetOutputPins();
                            
                            // Check if m_startPin is in this component's pins
                            bool isStartInThisComp = false;
                            for (Pin& p : inputs)
                            {
                                if (&p == m_startPin)
                                {
                                    isStartInThisComp = true;
                                    startComp = c;
                                    break;
                                }
                            }
                            if (!isStartInThisComp) {
                                for (Pin& p : outputs)
                                {
                                    if (&p == m_startPin)
                                    {
                                        isStartInThisComp = true;
                                        startComp = c;
                                        break;
                                    }
                                }
                            }
                            
                            if (isStartInThisComp) break;
                        }
                        
                        if (startComp)
                        {
                            // Create and execute the add wire command
                            std::unique_ptr<AddWireCommand> cmd = std::make_unique<AddWireCommand>(this, m_startPin, &pin);
                            PushUndoCommand(std::move(cmd));
                            
                            // Mark pins as connected
                            m_startPin->SetConnected(true);
                            pin.SetConnected(true);
                        }
                        
                        // Exit wire creation mode
                        m_wireCreationMode = false;
                        m_startPin = nullptr;
                    }
                    pinFound = true;
                    break;
                }
            }
            
            if (pinFound) break;
            
            // Check output pins
            for (Pin& pin : comp->GetOutputPins())
            {
                wxPoint pinPos = pin.GetPosition();
                if (wxRect(pinPos.x - 4, pinPos.y - 4, 8, 8).Contains(pos))
                {
                    if (!m_startPin)
                    {
                        // This is our starting pin
                        m_startPin = &pin;
                    }
                    else if (m_startPin != &pin)  // Can't connect pin to itself
                    {
                        // This is our ending pin - create the wire
                        // Find which component the starting pin belongs to
                        Component* startComp = nullptr;
                        for (Component* c : m_components)
                        {
                            auto& inputs = c->GetInputPins();
                            auto& outputs = c->GetOutputPins();
                            
                            // Check if m_startPin is in this component's pins
                            bool isStartInThisComp = false;
                            for (Pin& p : inputs)
                            {
                                if (&p == m_startPin)
                                {
                                    isStartInThisComp = true;
                                    startComp = c;
                                    break;
                                }
                            }
                            if (!isStartInThisComp) {
                                for (Pin& p : outputs)
                                {
                                    if (&p == m_startPin)
                                    {
                                        isStartInThisComp = true;
                                        startComp = c;
                                        break;
                                    }
                                }
                            }
                            
                            if (isStartInThisComp) break;
                        }
                        
                        if (startComp)
                        {
                            // Create and execute the add wire command
                            std::unique_ptr<AddWireCommand> cmd = std::make_unique<AddWireCommand>(this, m_startPin, &pin);
                            PushUndoCommand(std::move(cmd));
                            
                            // Mark pins as connected
                            m_startPin->SetConnected(true);
                            pin.SetConnected(true);
                        }
                        
                        // Exit wire creation mode
                        m_wireCreationMode = false;
                        m_startPin = nullptr;
                    }
                    pinFound = true;
                    break;
                }
            }
            
            if (pinFound) break;
        }

        if (!pinFound)
        {
            // Clicked on empty space - exit wire creation mode
            m_wireCreationMode = false;
            m_startPin = nullptr;
        }
    }
    else
    {
        // Check if we clicked on a component using spatial indexing for performance with large circuits
        bool componentClicked = false;
        
        // Use spatial indexing to find components near the click position
        wxRect searchArea(pos.x - 10, pos.y - 10, 20, 20);  // Small area around the click
        std::vector<Component*> nearbyComponents = GetComponentsInArea(searchArea);
        
        // Check components in reverse order to get topmost first (like the original implementation)
        for (auto it = nearbyComponents.rbegin(); it != nearbyComponents.rend(); ++it)
        {
            Component* comp = *it;
            if (comp->Contains(pos))
            {
                // Check if Ctrl is pressed for multi-selection
                bool ctrlPressed = event.ControlDown();

                if (comp->IsSelected() && ctrlPressed) {
                    // If component is already selected and Ctrl is pressed, deselect it
                    auto it = std::find(m_selectedComponents.begin(), m_selectedComponents.end(), comp);
                    if (it != m_selectedComponents.end()) {
                        m_selectedComponents.erase(it);
                        comp->SetSelected(false);

                        // Update the primary selection to the last selected component
                        if (!m_selectedComponents.empty()) {
                            m_selectedComponent = m_selectedComponents.back();
                        } else {
                            m_selectedComponent = nullptr;
                        }
                    }
                } else if (!comp->IsSelected()) {
                    // If component is not selected, select it
                    if (!ctrlPressed) {
                        // If Ctrl is not pressed, clear previous selection
                        ClearSelection();
                    }

                    // Add the component to selection
                    m_selectedComponents.push_back(comp);
                    comp->SetSelected(true);
                    m_selectedComponent = comp;

                    // Notify that selection changed if callback is set
                    if (m_selectionChangedCallback) {
                        m_selectionChangedCallback(comp);
                    }
                }

                componentClicked = true;
                m_dragging = true;
                m_lastMousePos = pos;

                // Store the original positions of all selected components for potential move command
                m_originalPositions.clear();
                for (Component* c : m_selectedComponents) {
                    m_originalPositions[c] = c->GetPosition();
                }
                break;
            }
        }

        if (!componentClicked) {
            // Clicked on empty space - deselect all (unless Shift is pressed for marquee selection)
            if (!event.ShiftDown()) {
                ClearSelection();
            }

            m_dragging = true;
            m_lastMousePos = pos;
        }
    }

    CaptureMouse();
    Refresh();
}

#include "UndoRedo.h"

CircuitCanvas::CircuitCanvas(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id), m_dragging(false), m_selectedComponent(nullptr), 
      m_wireCreationMode(false), m_startPin(nullptr), m_animationTimer(nullptr),
      m_undoRedoManager(new UndoRedoManager()), m_gridEnabled(true), m_snapToGrid(true), m_gridSpacing(20),
      m_zoomFactor(1.0), m_panOffset(0, 0), m_lastMousePosition(0, 0), m_panning(false),
      m_simulationController(nullptr)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);
    SetFocus(); // Allow the canvas to receive keyboard events
    SetDoubleBuffered(true); // Reduce flickering
    
    // Initialize spatial index for performance with large circuits
    RebuildSpatialIndex();
}

CircuitCanvas::~CircuitCanvas()
{
    if (m_undoRedoManager) {
        delete m_undoRedoManager;
        m_undoRedoManager = nullptr;
    }
    
    if (m_animationTimer) {
        m_animationTimer->Stop();
        delete m_animationTimer;
        m_animationTimer = nullptr;
    }
    
    // Clear spatial grid to avoid dangling pointers
    m_spatialGrid.clear();
}



Component* CircuitCanvas::GetComponentForPin(const Pin* pin) const
{
    // Find which component a pin belongs to - this is an expensive operation that should be optimized
    // In a real implementation, pins would have back-references to their components
    for (Component* comp : m_components)
    {
        // Check input pins
        std::vector<Pin>& inputs = comp->GetInputPins();
        for (Pin& p : inputs)
        {
            if (&p == pin) {
                return comp;
            }
        }

        // Check output pins
        std::vector<Pin>& outputs = comp->GetOutputPins();
        for (Pin& p : outputs)
        {
            if (&p == pin) {
                return comp;
            }
        }
    }
    return nullptr;
}

void CircuitCanvas::AddComponent(Component* component)
{
    m_components.push_back(component);
    AddComponentToSpatialGrid(component);  // Add to spatial index for performance
    Refresh();
}

void CircuitCanvas::AddWire(Wire* wire)
{
    m_wires.push_back(wire);
    Refresh();
}


// NAND Gate Component Implementation
NANDGateComponent::NANDGateComponent(int x, int y) : Component(x, y, "NAND")
{
    // Initialize input pins
    wxRect bodyRect = GetBodyRect();
    m_inputPins.emplace_back(bodyRect.x - 10, bodyRect.y + bodyRect.height/3, "A", true);  // Input A
    m_inputPins.emplace_back(bodyRect.x - 10, bodyRect.y + 2*bodyRect.height/3, "B", true); // Input B
    
    // Initialize output pin
    m_outputPins.emplace_back(bodyRect.x + bodyRect.width * 2/3 + 8, bodyRect.y + bodyRect.height/2, "Y", false); // Output Y
}

void NANDGateComponent::Draw(wxDC& dc)
{
    wxRect bodyRect = GetBodyRect();
    
    // Draw the NAND gate shape (triangle with bubble)
    wxPoint gatePoints[] = {
        wxPoint(bodyRect.x, bodyRect.y),
        wxPoint(bodyRect.x + bodyRect.width * 2/3, bodyRect.y + bodyRect.height/2),
        wxPoint(bodyRect.x, bodyRect.y + bodyRect.height)
    };
    
    if (IsSelected())
    {
        dc.SetPen(wxPen(*wxRED, 2));
    }
    else
    {
        dc.SetPen(wxPen(*wxBLACK, 1));
    }
    
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawPolygon(3, gatePoints);
    
    // Draw the bubble (inversion) at the output
    int bubbleX = bodyRect.x + bodyRect.width * 2/3 + 8;
    int bubbleY = bodyRect.y + bodyRect.height/2;
    dc.DrawCircle(bubbleX, bubbleY, 4);
    
    // Draw input pins and labels
    for (size_t i = 0; i < m_inputPins.size(); i++)
    {
        Pin& pin = m_inputPins[i];
        wxPoint pinPos = pin.GetPosition();
        
        // Draw line from pin to gate
        dc.DrawLine(pinPos.x, pinPos.y, bodyRect.x, bodyRect.y + (i+1)*(bodyRect.height/3));
        
        // Draw pin as a small circle
        dc.SetBrush(pin.IsConnected() ? wxBrush(wxColour(0, 255, 0)) : wxBrush(*wxBLACK_BRUSH));
        dc.DrawCircle(pinPos.x, pinPos.y, 3);
        
        // Draw pin label
        dc.DrawText(pin.GetName(), pinPos.x + 5, pinPos.y - 8);
    }
    
    // Draw output pin and label
    if (!m_outputPins.empty()) {
        Pin& pin = m_outputPins[0];  // NAND gate has one output
        wxPoint pinPos = pin.GetPosition();
        
        // Draw line from gate to pin
        dc.DrawLine(bodyRect.x + bodyRect.width * 2/3, bodyRect.y + bodyRect.height/2,
                   pinPos.x - 8, pinPos.y);  // -8 to account for bubble
        
        // Draw pin as a small circle
        dc.SetBrush(pin.IsConnected() ? wxBrush(wxColour(0, 255, 0)) : wxBrush(*wxBLACK_BRUSH));
        dc.DrawCircle(pinPos.x, pinPos.y, 3);
        
        // Draw pin label
        dc.DrawText(pin.GetName(), pinPos.x + 5, pinPos.y - 8);
    }
    
    // Draw component name
    dc.DrawText(GetName(), bodyRect.x + 5, bodyRect.y + bodyRect.height/2 - 5);
}

bool NANDGateComponent::Contains(const wxPoint& pos) const
{
    wxRect bounds = GetBounds();
    return bounds.Contains(pos);
}

wxRect NANDGateComponent::GetBounds() const
{
    wxRect bodyRect = GetBodyRect();
    
    // Calculate the full bounds including pins
    int totalLeft = bodyRect.x - 20;  // Space for input pins
    int totalTop = bodyRect.y;
    int totalRight = bodyRect.x + bodyRect.width + 20;  // Space for output pin and bubble
    int totalBottom = bodyRect.y + bodyRect.height;
    
    return wxRect(totalLeft, totalTop, totalRight - totalLeft, totalBottom - totalTop);
}

wxRect NANDGateComponent::GetBodyRect() const
{
    wxPoint pos = GetPosition();
    return wxRect(pos.x, pos.y, 60, 50);
}

std::vector<Pin>& NANDGateComponent::GetInputPins()
{
    return m_inputPins;
}

std::vector<Pin>& NANDGateComponent::GetOutputPins()
{
    return m_outputPins;
}

// NOR Gate Component Implementation
NORGateComponent::NORGateComponent(int x, int y) : Component(x, y, "NOR")
{
    // Initialize input pins
    wxRect bodyRect = GetBodyRect();
    m_inputPins.emplace_back(bodyRect.x - 10, bodyRect.y + bodyRect.height/3, "A", true);  // Input A
    m_inputPins.emplace_back(bodyRect.x - 10, bodyRect.y + 2*bodyRect.height/3, "B", true); // Input B
    
    // Initialize output pin
    m_outputPins.emplace_back(bodyRect.x + 15 + bodyRect.width * 2/3 + 8, bodyRect.y + bodyRect.height/2, "Y", false); // Output Y
}

void NORGateComponent::Draw(wxDC& dc)
{
    wxRect bodyRect = GetBodyRect();
    
    // Draw the NOR gate shape (triangle with bubble and arc)
    wxPoint gatePoints[] = {
        wxPoint(bodyRect.x + 15, bodyRect.y),
        wxPoint(bodyRect.x + 15 + bodyRect.width * 2/3, bodyRect.y + bodyRect.height/2),
        wxPoint(bodyRect.x + 15, bodyRect.y + bodyRect.height)
    };
    
    if (IsSelected())
    {
        dc.SetPen(wxPen(*wxRED, 2));
    }
    else
    {
        dc.SetPen(wxPen(*wxBLACK, 1));
    }
    
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawPolygon(3, gatePoints);
    
    // Draw the arc for NOR - using 3 wxPoint parameters (start, end, center)
    dc.DrawArc(wxPoint(bodyRect.x, bodyRect.y + bodyRect.height/2), 
               wxPoint(bodyRect.x + 15, bodyRect.y + bodyRect.height/2),
               wxPoint(bodyRect.x + 15, bodyRect.y));
    dc.DrawArc(wxPoint(bodyRect.x, bodyRect.y + bodyRect.height/2), 
               wxPoint(bodyRect.x + 15, bodyRect.y + bodyRect.height/2),
               wxPoint(bodyRect.x + 15, bodyRect.y + bodyRect.height));
    
    // Draw the bubble (inversion) at the output
    int bubbleX = bodyRect.x + 15 + bodyRect.width * 2/3 + 8;
    int bubbleY = bodyRect.y + bodyRect.height/2;
    dc.DrawCircle(bubbleX, bubbleY, 4);
    
    // Draw input pins and labels
    for (size_t i = 0; i < m_inputPins.size(); i++)
    {
        Pin& pin = m_inputPins[i];
        wxPoint pinPos = pin.GetPosition();
        
        // Draw line from pin to gate
        dc.DrawLine(pinPos.x, pinPos.y, bodyRect.x + 15, bodyRect.y + (i+1)*(bodyRect.height/3));
        
        // Draw pin as a small circle
        dc.SetBrush(pin.IsConnected() ? wxBrush(wxColour(0, 255, 0)) : wxBrush(*wxBLACK_BRUSH));
        dc.DrawCircle(pinPos.x, pinPos.y, 3);
        
        // Draw pin label
        dc.DrawText(pin.GetName(), pinPos.x + 5, pinPos.y - 8);
    }
    
    // Draw output pin and label
    if (!m_outputPins.empty()) {
        Pin& pin = m_outputPins[0];  // NOR gate has one output
        wxPoint pinPos = pin.GetPosition();
        
        // Draw line from gate to pin
        dc.DrawLine(bodyRect.x + 15 + bodyRect.width * 2/3, bodyRect.y + bodyRect.height/2,
                   pinPos.x - 8, pinPos.y);  // -8 to account for bubble
        
        // Draw pin as a small circle
        dc.SetBrush(pin.IsConnected() ? wxBrush(wxColour(0, 255, 0)) : wxBrush(*wxBLACK_BRUSH));
        dc.DrawCircle(pinPos.x, pinPos.y, 3);
        
        // Draw pin label
        dc.DrawText(pin.GetName(), pinPos.x + 5, pinPos.y - 8);
    }
    
    // Draw component name
    dc.DrawText(GetName(), bodyRect.x + 20, bodyRect.y + bodyRect.height/2 - 5);
}

bool NORGateComponent::Contains(const wxPoint& pos) const
{
    wxRect bounds = GetBounds();
    return bounds.Contains(pos);
}

wxRect NORGateComponent::GetBounds() const
{
    wxRect bodyRect = GetBodyRect();
    
    // Calculate the full bounds including pins and arc
    int totalLeft = bodyRect.x - 20;  // Space for input pins and arc
    int totalTop = bodyRect.y;
    int totalRight = bodyRect.x + bodyRect.width + 25;  // Space for output pin and bubble
    int totalBottom = bodyRect.y + bodyRect.height;
    
    return wxRect(totalLeft, totalTop, totalRight - totalLeft, totalBottom - totalTop);
}

wxRect NORGateComponent::GetBodyRect() const
{
    wxPoint pos = GetPosition();
    return wxRect(pos.x, pos.y, 60, 50);
}

std::vector<Pin>& NORGateComponent::GetInputPins()
{
    return m_inputPins;
}

std::vector<Pin>& NORGateComponent::GetOutputPins()
{
    return m_outputPins;
}

// NOT Gate (Inverter) Component Implementation
NOTGateComponent::NOTGateComponent(int x, int y) : Component(x, y, "NOT")
{
    // Initialize input pin
    wxRect bodyRect = GetBodyRect();
    m_inputPins.emplace_back(bodyRect.x - 10, bodyRect.y + bodyRect.height/2, "A", true);  // Input A
    
    // Initialize output pin
    m_outputPins.emplace_back(bodyRect.x + bodyRect.width + 8, bodyRect.y + bodyRect.height/2, "Y", false); // Output Y
}

void NOTGateComponent::Draw(wxDC& dc)
{
    wxRect bodyRect = GetBodyRect();
    
    // Draw the NOT gate shape (triangle with bubble)
    wxPoint gatePoints[] = {
        wxPoint(bodyRect.x, bodyRect.y),
        wxPoint(bodyRect.x + bodyRect.width, bodyRect.y + bodyRect.height/2),
        wxPoint(bodyRect.x, bodyRect.y + bodyRect.height)
    };
    
    if (IsSelected())
    {
        dc.SetPen(wxPen(*wxRED, 2));
    }
    else
    {
        dc.SetPen(wxPen(*wxBLACK, 1));
    }
    
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawPolygon(3, gatePoints);
    
    // Draw the bubble (inversion) at the output
    int bubbleX = bodyRect.x + bodyRect.width + 8;
    int bubbleY = bodyRect.y + bodyRect.height/2;
    dc.DrawCircle(bubbleX, bubbleY, 4);
    
    // Draw input pin and label
    if (!m_inputPins.empty()) {
        Pin& pin = m_inputPins[0];  // NOT gate has one input
        wxPoint pinPos = pin.GetPosition();
        
        // Draw line from pin to gate
        dc.DrawLine(pinPos.x, pinPos.y, bodyRect.x, bodyRect.y + bodyRect.height/2);
        
        // Draw pin as a small circle
        dc.SetBrush(pin.IsConnected() ? wxBrush(wxColour(0, 255, 0)) : wxBrush(*wxBLACK_BRUSH));
        dc.DrawCircle(pinPos.x, pinPos.y, 3);
        
        // Draw pin label
        dc.DrawText(pin.GetName(), pinPos.x + 5, pinPos.y - 8);
    }
    
    // Draw output pin and label
    if (!m_outputPins.empty()) {
        Pin& pin = m_outputPins[0];  // NOT gate has one output
        wxPoint pinPos = pin.GetPosition();
        
        // Draw line from gate to pin
        dc.DrawLine(bodyRect.x + bodyRect.width, bodyRect.y + bodyRect.height/2,
                   pinPos.x - 8, pinPos.y);  // -8 to account for bubble
        
        // Draw pin as a small circle
        dc.SetBrush(pin.IsConnected() ? wxBrush(wxColour(0, 255, 0)) : wxBrush(*wxBLACK_BRUSH));
        dc.DrawCircle(pinPos.x, pinPos.y, 3);
        
        // Draw pin label
        dc.DrawText(pin.GetName(), pinPos.x + 5, pinPos.y - 8);
    }
    
    // Draw component name
    dc.DrawText(GetName(), bodyRect.x + 5, bodyRect.y + bodyRect.height/2 - 5);
}

bool NOTGateComponent::Contains(const wxPoint& pos) const
{
    wxRect bounds = GetBounds();
    return bounds.Contains(pos);
}

wxRect NOTGateComponent::GetBounds() const
{
    wxRect bodyRect = GetBodyRect();
    
    // Calculate the full bounds including pins
    int totalLeft = bodyRect.x - 20;  // Space for input pin
    int totalTop = bodyRect.y;
    int totalRight = bodyRect.x + bodyRect.width + 20;  // Space for output pin and bubble
    int totalBottom = bodyRect.y + bodyRect.height;
    
    return wxRect(totalLeft, totalTop, totalRight - totalLeft, totalBottom - totalTop);
}

wxRect NOTGateComponent::GetBodyRect() const
{
    wxPoint pos = GetPosition();
    return wxRect(pos.x, pos.y, 40, 40);
}

std::vector<Pin>& NOTGateComponent::GetInputPins()
{
    return m_inputPins;
}

std::vector<Pin>& NOTGateComponent::GetOutputPins()
{
    return m_outputPins;
}

// Buffer Component Implementation
BufferComponent::BufferComponent(int x, int y) : Component(x, y, "BUF")
{
    // Initialize input pin
    wxRect bodyRect = GetBodyRect();
    m_inputPins.emplace_back(bodyRect.x - 10, bodyRect.y + bodyRect.height/2, "A", true);  // Input A
    
    // Initialize output pin
    m_outputPins.emplace_back(bodyRect.x + bodyRect.width, bodyRect.y + bodyRect.height/2, "Y", false); // Output Y
}

void BufferComponent::Draw(wxDC& dc)
{
    wxRect bodyRect = GetBodyRect();
    
    // Draw the buffer shape (triangle)
    wxPoint gatePoints[] = {
        wxPoint(bodyRect.x, bodyRect.y),
        wxPoint(bodyRect.x + bodyRect.width, bodyRect.y + bodyRect.height/2),
        wxPoint(bodyRect.x, bodyRect.y + bodyRect.height)
    };
    
    if (IsSelected())
    {
        dc.SetPen(wxPen(*wxRED, 2));
    }
    else
    {
        dc.SetPen(wxPen(*wxBLACK, 1));
    }
    
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawPolygon(3, gatePoints);
    
    // Draw input pin and label
    if (!m_inputPins.empty()) {
        Pin& pin = m_inputPins[0];  // Buffer has one input
        wxPoint pinPos = pin.GetPosition();
        
        // Draw line from pin to gate
        dc.DrawLine(pinPos.x, pinPos.y, bodyRect.x, bodyRect.y + bodyRect.height/2);
        
        // Draw pin as a small circle
        dc.SetBrush(pin.IsConnected() ? wxBrush(wxColour(0, 255, 0)) : wxBrush(*wxBLACK_BRUSH));
        dc.DrawCircle(pinPos.x, pinPos.y, 3);
        
        // Draw pin label
        dc.DrawText(pin.GetName(), pinPos.x + 5, pinPos.y - 8);
    }
    
    // Draw output pin and label
    if (!m_outputPins.empty()) {
        Pin& pin = m_outputPins[0];  // Buffer has one output
        wxPoint pinPos = pin.GetPosition();
        
        // Draw line from gate to pin
        dc.DrawLine(bodyRect.x + bodyRect.width, bodyRect.y + bodyRect.height/2,
                   pinPos.x, pinPos.y);
        
        // Draw pin as a small circle
        dc.SetBrush(pin.IsConnected() ? wxBrush(wxColour(0, 255, 0)) : wxBrush(*wxBLACK_BRUSH));
        dc.DrawCircle(pinPos.x, pinPos.y, 3);
        
        // Draw pin label
        dc.DrawText(pin.GetName(), pinPos.x + 5, pinPos.y - 8);
    }
    
    // Draw component name
    dc.DrawText(GetName(), bodyRect.x + 5, bodyRect.y + bodyRect.height/2 - 5);
}

bool BufferComponent::Contains(const wxPoint& pos) const
{
    wxRect bounds = GetBounds();
    return bounds.Contains(pos);
}

wxRect BufferComponent::GetBounds() const
{
    wxRect bodyRect = GetBodyRect();
    
    // Calculate the full bounds including pins
    int totalLeft = bodyRect.x - 20;  // Space for input pin
    int totalTop = bodyRect.y;
    int totalRight = bodyRect.x + bodyRect.width + 15;  // Space for output pin
    int totalBottom = bodyRect.y + bodyRect.height;
    
    return wxRect(totalLeft, totalTop, totalRight - totalLeft, totalBottom - totalTop);
}

wxRect BufferComponent::GetBodyRect() const
{
    wxPoint pos = GetPosition();
    return wxRect(pos.x, pos.y, 40, 40);
}

std::vector<Pin>& BufferComponent::GetInputPins()
{
    return m_inputPins;
}

std::vector<Pin>& BufferComponent::GetOutputPins()
{
    return m_outputPins;
}

// Simple Wire Implementation
SimpleWire::SimpleWire(Pin* start, Pin* end)
    : Wire(start, end)
{
}

void SimpleWire::Draw(wxDC& dc)
{
    wxPoint start = GetStartPin()->GetPosition();
    wxPoint end = GetEndPin()->GetPosition();
    
    // Draw the main wire line
    wxPen wirePen = IsActive() ? wxPen(wxColour(100, 100, 100), 2) : wxPen(*wxBLACK, 1);
    dc.SetPen(wirePen);
    
    // Draw a line with possible bends for better visual appearance
    int midX = (start.x + end.x) / 2;
    dc.DrawLine(start.x, start.y, midX, start.y);
    dc.DrawLine(midX, start.y, midX, end.y);
    dc.DrawLine(midX, end.y, end.x, end.y);
    
    // If animation is active, draw the propagating signal
    if (IsAnimationActive())
    {
        float pos = GetPropagationPosition();
        
        // Calculate the animated point along the wire path
        wxPoint animPt;
        if (pos < 0.5f) {
            // First segment: start -> midX, start.y
            float localPos = pos * 2.0f;  // Normalize to [0, 1] for this segment
            animPt.x = start.x + (midX - start.x) * localPos;
            animPt.y = start.y;
        } else {
            // Second segment: midX, start.y -> midX, end.y
            float localPos = (pos - 0.5f) * 2.0f;  // Normalize to [0, 1] for this segment
            animPt.x = midX;
            animPt.y = start.y + (end.y - start.y) * localPos;
        }
        
        // Draw the animated signal as a colored circle
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(IsActive() ? wxColour(0, 255, 0) : wxColour(255, 255, 0))); // Green for active, yellow for inactive
        dc.DrawCircle(animPt.x, animPt.y, 5);
    }
}

void Wire::UpdateAnimation(float deltaTime)
{
    if (m_animationActive) {
        // Move the propagation position forward
        // The speed can be adjusted based on the signal type or other factors
        m_propagationPosition += deltaTime * 0.5f; // Adjust speed as needed
        
        if (m_propagationPosition >= 1.0f) {
            m_propagationPosition = 0.0f; // Reset for continuous animation
        }
    }
}

void CircuitCanvas::OnMouseMotion(wxMouseEvent& event)
{
    if (m_dragging && !m_selectedComponents.empty() && event.Dragging())
    {
        wxPoint pos = PhysicalToLogical(event.GetPosition());
        int dx = pos.x - m_lastMousePos.x;
        int dy = pos.y - m_lastMousePos.y;
        
        // Move all selected components by the same offset
        for (Component* comp : m_selectedComponents) {
            wxPoint currentPos = comp->GetPosition();
            wxPoint newPos(currentPos.x + dx, currentPos.y + dy);
            
            // Apply snapping if enabled
            if (m_snapToGrid) {
                newPos = SnapToGrid(newPos);
            }
            
            // Update component position
            comp->SetPosition(newPos);
        }
        
        m_lastMousePos = pos;
        Refresh();
    }
    else if (m_wireCreationMode && m_startPin)
    {
        // Update the temporary wire endpoint
        m_currentWireEndPoint = PhysicalToLogical(event.GetPosition());
        Refresh();
    }
}

void CircuitCanvas::OnMouseLeftUp(wxMouseEvent& event)
{
    if (m_dragging && !m_selectedComponents.empty())
    {
        // Check if any of the selected components actually moved and create move commands
        for (Component* comp : m_selectedComponents) {
            auto it = m_originalPositions.find(comp);
            if (it != m_originalPositions.end()) {
                wxPoint originalPos = it->second;
                wxPoint currentPos = comp->GetPosition();
                
                // Only create a command if the position actually changed
                if (originalPos != currentPos) {
                    std::unique_ptr<MoveComponentCommand> cmd = std::make_unique<MoveComponentCommand>(
                        this, comp,
                        originalPos.x, originalPos.y,
                        currentPos.x, currentPos.y
                    );
                    PushUndoCommand(std::move(cmd));
                }
            }
        }
        
        // Clear the stored original positions
        m_originalPositions.clear();
    }
    
    if (HasCapture())
        ReleaseMouse();
    
    m_dragging = false;
    m_selectedComponent = nullptr;
}

wxPoint CircuitCanvas::SnapToGrid(const wxPoint& point) const
{
    if (!m_snapToGrid || m_gridSpacing <= 0) {
        return point;
    }
    
    int snappedX = (int)(point.x / m_gridSpacing + 0.5) * m_gridSpacing;
    int snappedY = (int)(point.y / m_gridSpacing + 0.5) * m_gridSpacing;
    
    return wxPoint(snappedX, snappedY);
}

void CircuitCanvas::OnPaint(wxPaintEvent& event)
{
    wxBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    
    // Apply zoom and pan transformations using world transformations
    dc.SetUserScale(m_zoomFactor, m_zoomFactor);
    dc.SetDeviceOrigin(m_panOffset.x, m_panOffset.y);
    
    // Draw grid if enabled
    if (m_gridEnabled) {
        wxSize clientSize = GetClientSize();
        // Calculate the visible area in logical coordinates
        int minX = (int)(-m_panOffset.x / m_zoomFactor);
        int minY = (int)(-m_panOffset.y / m_zoomFactor);
        int maxX = (int)((clientSize.x - m_panOffset.x) / m_zoomFactor);
        int maxY = (int)((clientSize.y - m_panOffset.y) / m_zoomFactor);
        
        wxPen gridPen(wxColour(240, 240, 240), 1, wxPENSTYLE_SOLID);
        dc.SetPen(gridPen);
        
        // Draw vertical grid lines
        for (int x = (minX / m_gridSpacing) * m_gridSpacing; x <= maxX; x += m_gridSpacing) {
            dc.DrawLine(x, minY, x, maxY);
        }
        
        // Draw horizontal grid lines
        for (int y = (minY / m_gridSpacing) * m_gridSpacing; y <= maxY; y += m_gridSpacing) {
            dc.DrawLine(minX, y, maxX, y);
        }
    }
    
    // Draw all wires first (so they appear under components)
    for (Wire* wire : m_wires)
    {
        // If simulation is running, get the actual signal state from the simulation controller
        bool wireActive = wire->IsActive();
        if (m_simulationController && m_simulationController->IsRunning()) {
            // Get simulation state for this wire
            // For now, we'll just use the wire's internal active state
            // In a full implementation, we would query the simulation controller
        }
        
        // Draw the wire with its active state
        wxPoint start = wire->GetStartPin()->GetPosition();
        wxPoint end = wire->GetEndPin()->GetPosition();
        
        // Draw the main wire line
        wxPen wirePen = wireActive ? wxPen(wxColour(100, 100, 100), 2) : wxPen(*wxBLACK, 1);
        dc.SetPen(wirePen);
        
        // Draw a line with possible bends for better visual appearance
        int midX = (start.x + end.x) / 2;
        dc.DrawLine(start.x, start.y, midX, start.y);
        dc.DrawLine(midX, start.y, midX, end.y);
        dc.DrawLine(midX, end.y, end.x, end.y);
        
        // If animation is active, draw the propagating signal
        if (wire->IsAnimationActive())
        {
            float pos = wire->GetPropagationPosition();
            
            // Calculate the animated point along the wire path
            wxPoint animPt;
            if (pos < 0.5f) {
                // First segment: start -> midX, start.y
                float localPos = pos * 2.0f;  // Normalize to [0, 1] for this segment
                animPt.x = start.x + (midX - start.x) * localPos;
                animPt.y = start.y;
            } else {
                // Second segment: midX, start.y -> midX, end.y
                float localPos = (pos - 0.5f) * 2.0f;  // Normalize to [0, 1] for this segment
                animPt.x = midX;
                animPt.y = start.y + (end.y - start.y) * localPos;
            }
            
            // Draw the animated signal as a colored circle
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(wireActive ? wxColour(0, 255, 0) : wxColour(255, 255, 0))); // Green for active, yellow for inactive
            dc.DrawCircle(animPt.x, animPt.y, 5);
        }
    }
    
    // If in wire creation mode, draw the temporary wire
    if (m_wireCreationMode && m_startPin)
    {
        dc.SetPen(wxPen(*wxBLACK, 1)); // Black for inactive
        
        wxPoint start = m_startPin->GetPosition();
        wxPoint end = m_currentWireEndPoint;
        
        // Draw a line with possible bends for better visual appearance
        int midX = (start.x + end.x) / 2;
        dc.DrawLine(start.x, start.y, midX, start.y);
        dc.DrawLine(midX, start.y, midX, end.y);
        dc.DrawLine(midX, end.y, end.x, end.y);
    }
    
    // Draw all components
    for (Component* comp : m_components)
    {
        comp->Draw(dc);
    }
    
    // Reset transformation for UI elements (like selection rectangle)
    dc.SetUserScale(1.0, 1.0);
    dc.SetDeviceOrigin(0, 0);
    
    // If dragging, draw a selection rectangle
    if (m_dragging && !m_selectedComponent)
    {
        wxPen pen(*wxBLUE, 1, wxPENSTYLE_DOT);
        dc.SetPen(pen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        // Draw selection rectangle
    }
}

void CircuitCanvas::SetZoomFactor(double factor)
{
    m_zoomFactor = wxMax(0.1, wxMin(factor, 5.0)); // Limit zoom between 0.1x and 5x
    Refresh();
}

void CircuitCanvas::ZoomIn()
{
    SetZoomFactor(m_zoomFactor * 1.2);
}

void CircuitCanvas::ZoomOut()
{
    SetZoomFactor(m_zoomFactor / 1.2);
}

void CircuitCanvas::ResetZoom()
{
    m_zoomFactor = 1.0;
    m_panOffset = wxPoint(0, 0);
    Refresh();
}

void CircuitCanvas::Pan(int dx, int dy)
{
    m_panOffset.x += dx;
    m_panOffset.y += dy;
    Refresh();
}

wxPoint CircuitCanvas::LogicalToPhysical(const wxPoint& logicalPoint) const
{
    wxPoint point;
    point.x = (int)(logicalPoint.x * m_zoomFactor) + m_panOffset.x;
    point.y = (int)(logicalPoint.y * m_zoomFactor) + m_panOffset.y;
    return point;
}

wxPoint CircuitCanvas::PhysicalToLogical(const wxPoint& physicalPoint) const
{
    wxPoint point;
    point.x = (int)((physicalPoint.x - m_panOffset.x) / m_zoomFactor);
    point.y = (int)((physicalPoint.y - m_panOffset.y) / m_zoomFactor);
    return point;
}

void CircuitCanvas::SelectComponent(Component* comp, bool additive)
{
    if (!additive) {
        // Clear all selections
        for (Component* c : m_selectedComponents) {
            c->SetSelected(false);
        }
        m_selectedComponents.clear();
    }
    
    if (comp) {
        // Check if component is already selected
        auto it = std::find(m_selectedComponents.begin(), m_selectedComponents.end(), comp);
        if (it == m_selectedComponents.end()) {
            // Add to selection
            m_selectedComponents.push_back(comp);
            comp->SetSelected(true);
        } else {
            // If already selected and not additive, deselect it
            if (!additive) {
                m_selectedComponents.erase(it);
                comp->SetSelected(false);
            }
        }
        
        m_selectedComponent = comp;  // Set as primary selection
    }
    
    // Notify about selection change
    if (m_selectionChangedCallback) {
        m_selectionChangedCallback(comp);
    }
}

void CircuitCanvas::SelectAllComponents()
{
    // Clear existing selection
    for (Component* c : m_selectedComponents) {
        c->SetSelected(false);
    }
    m_selectedComponents.clear();
    
    // Select all components
    for (Component* comp : m_components) {
        comp->SetSelected(true);
        m_selectedComponents.push_back(comp);
    }
    
    // Set the last component as the primary selection
    if (!m_components.empty()) {
        m_selectedComponent = m_components.back();
        
        // Notify about selection change
        if (m_selectionChangedCallback) {
            m_selectionChangedCallback(m_selectedComponent);
        }
    }
}

void CircuitCanvas::ClearSelection()
{
    // Clear all selections
    for (Component* c : m_selectedComponents) {
        c->SetSelected(false);
    }
    m_selectedComponents.clear();
    m_selectedComponent = nullptr;
    
    // Notify about selection change
    if (m_selectionChangedCallback) {
        m_selectionChangedCallback(nullptr);
    }
}

void CircuitCanvas::DeleteSelectedComponents()
{
    // Create delete commands for each selected component
    for (Component* comp : m_selectedComponents) {
        std::unique_ptr<DeleteComponentCommand> cmd = std::make_unique<DeleteComponentCommand>(this, comp);
        PushUndoCommand(std::move(cmd));
    }
    
    // Clear selection after deletion
    ClearSelection();
}

bool CircuitCanvas::IsComponentSelected(Component* comp) const
{
    return std::find(m_selectedComponents.begin(), m_selectedComponents.end(), comp) != m_selectedComponents.end();
}

void CircuitCanvas::OnMouseWheel(wxMouseEvent& event)
{
    // Get the mouse position in logical coordinates before zoom
    wxPoint mousePosLogical = PhysicalToLogical(event.GetPosition());
    
    // Zoom in or out based on mouse wheel direction
    if (event.GetWheelRotation() > 0) {
        // Zoom in
        SetZoomFactor(m_zoomFactor * 1.2);
    } else {
        // Zoom out
        SetZoomFactor(m_zoomFactor / 1.2);
    }
    
    // Adjust pan offset to keep mouse position fixed during zoom
    wxPoint newMousePosPhysical = LogicalToPhysical(mousePosLogical);
    m_panOffset.x += event.GetX() - newMousePosPhysical.x;
    m_panOffset.y += event.GetY() - newMousePosPhysical.y;
    
    Refresh();
}

void CircuitCanvas::OnMiddleMouseDown(wxMouseEvent& event)
{
    m_panning = true;
    m_lastMousePosition = event.GetPosition();
    SetCursor(wxCursor(wxCURSOR_HAND));
    CaptureMouse();
}

void CircuitCanvas::OnMiddleMouseUp(wxMouseEvent& event)
{
    m_panning = false;
    if (HasCapture()) {
        ReleaseMouse();
    }
    SetCursor(wxCursor(wxCURSOR_ARROW));
}

void CircuitCanvas::OnMouseMove(wxMouseEvent& event)
{
    if (m_panning && event.Dragging()) {
        wxPoint currentPos = event.GetPosition();
        int dx = currentPos.x - m_lastMousePosition.x;
        int dy = currentPos.y - m_lastMousePosition.y;
        
        m_panOffset.x += dx;
        m_panOffset.y += dy;
        
        m_lastMousePosition = currentPos;
        Refresh();
    }
    else if (m_wireCreationMode && m_startPin) {
        // Update the temporary wire endpoint with the current mouse position
        m_currentWireEndPoint = PhysicalToLogical(event.GetPosition());
        Refresh();
    }
}

void CircuitCanvas::OnKeyDown(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();
    
    // Handle Ctrl+Z for undo
    if (event.ControlDown() && keyCode == 'Z') {
        Undo();
        return;
    }
    
    // Handle Ctrl+Y for redo
    if (event.ControlDown() && keyCode == 'Y') {
        Redo();
        return;
    }
    
    // Handle zoom keys
    if (event.ControlDown() && keyCode == '+') {
        ZoomIn();
        return;
    }
    if (event.ControlDown() && keyCode == '-') {
        ZoomOut();
        return;
    }
    if (event.ControlDown() && keyCode == '0') {  // Ctrl+0 to reset zoom
        ResetZoom();
        return;
    }
    
    // Handle Ctrl+A for select all
    if (event.ControlDown() && keyCode == 'A') {
        SelectAllComponents();
        Refresh();
        return;
    }
    
    // Handle delete key to delete selected components
    if (keyCode == WXK_DELETE) {
        DeleteSelectedComponents();
        Refresh();
        return;
    }
    
    // Handle arrow keys for moving selected components
    if (!m_selectedComponents.empty()) {
        int dx = 0, dy = 0;
        
        switch (keyCode) {
            case WXK_UP:
                dy = -5;  // Move up
                break;
            case WXK_DOWN:
                dy = 5;   // Move down
                break;
            case WXK_LEFT:
                dx = -5;  // Move left
                break;
            case WXK_RIGHT:
                dx = 5;   // Move right
                break;
            default:
                event.Skip();  // Let other keys be handled normally
                return;
        }
        
        // Move all selected components
        for (Component* comp : m_selectedComponents) {
            wxPoint oldPos = comp->GetPosition();
            wxPoint newPos = SnapToGrid(wxPoint(oldPos.x + dx, oldPos.y + dy));
            
            // Actually move the component
            comp->SetPosition(newPos);
            
            // Create a move command 
            std::unique_ptr<MoveComponentCommand> cmd = std::make_unique<MoveComponentCommand>(
                this, comp,
                oldPos.x, oldPos.y,
                newPos.x, newPos.y
            );
            PushUndoCommand(std::move(cmd));
        }
        
        Refresh();
    } else {
        event.Skip();  // Let other keys be handled normally
    }
}

wxBEGIN_EVENT_TABLE(CircuitCanvas, wxPanel)
    EVT_PAINT(CircuitCanvas::OnPaint)
    EVT_SIZE(CircuitCanvas::OnSize)
    EVT_LEFT_DOWN(CircuitCanvas::OnMouseLeftDown)
    EVT_LEFT_UP(CircuitCanvas::OnMouseLeftUp)
    EVT_MOTION(CircuitCanvas::OnMouseMotion)
    EVT_RIGHT_DOWN(CircuitCanvas::OnMouseRightDown)
    EVT_MIDDLE_DOWN(CircuitCanvas::OnMiddleMouseDown)
    EVT_MIDDLE_UP(CircuitCanvas::OnMiddleMouseUp)
    EVT_MOUSEWHEEL(CircuitCanvas::OnMouseWheel)
    EVT_KEY_DOWN(CircuitCanvas::OnKeyDown)
    EVT_TIMER(wxID_ANY, CircuitCanvas::OnAnimationTimer)
wxEND_EVENT_TABLE()