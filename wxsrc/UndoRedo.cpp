#include "UndoRedo.h"
#include "CircuitCanvas.h"
#include <wx/wx.h>

// AddComponentCommand implementation
AddComponentCommand::AddComponentCommand(CircuitCanvas* canvas, int x, int y, const wxString& type)
    : m_canvas(canvas), m_x(x), m_y(y), m_type(type), m_component(nullptr), m_executed(false)
{
}

void AddComponentCommand::Execute()
{
    if (!m_executed) {
        // Create component based on type
        Component* comp = nullptr;
        if (m_type == "NAND") {
            comp = new NANDGateComponent(m_x, m_y);
        } else if (m_type == "NOR") {
            comp = new NORGateComponent(m_x, m_y);
        } else if (m_type == "NOT") {
            comp = new NOTGateComponent(m_x, m_y);
        } else if (m_type == "BUF") {
            comp = new BufferComponent(m_x, m_y);
        } else {
            // Default to buffer if type is unknown
            comp = new BufferComponent(m_x, m_y);
        }
        
        if (comp) {
            m_canvas->AddComponent(comp);
            m_component = comp;
            m_executed = true;
        }
    }
}

void AddComponentCommand::Undo()
{
    if (m_executed) {
        std::vector<Component*>& components = m_canvas->GetComponents();
        auto it = std::find(components.begin(), components.end(), static_cast<Component*>(m_component));
        if (it != components.end()) {
            components.erase(it);
            m_canvas->Refresh();
            m_executed = false;
        }
    }
}

void AddComponentCommand::Redo()
{
    if (!m_executed) {
        Execute();  // Execute does the adding again
    }
}

// DeleteComponentCommand implementation
DeleteComponentCommand::DeleteComponentCommand(CircuitCanvas* canvas, void* component)
    : m_canvas(canvas), m_component(component), m_executed(false)
{
    // Store component data for potential restoration
    Component* comp = static_cast<Component*>(component);
    wxPoint pos = comp->GetPosition();
    m_x = pos.x;
    m_y = pos.y;
    m_name = comp->GetName();
    m_type = comp->GetName();  // In a full implementation, we'd have a specific type
}

void DeleteComponentCommand::Execute()
{
    if (!m_executed) {
        std::vector<Component*>& components = m_canvas->GetComponents();
        auto it = std::find(components.begin(), components.end(), static_cast<Component*>(m_component));
        if (it != components.end()) {
            components.erase(it);
            m_canvas->Refresh();
            m_executed = true;
        }
    }
}

void DeleteComponentCommand::Undo()
{
    if (m_executed) {
        // Recreate the component
        Component* comp = nullptr;
        if (m_type == "NAND") {
            comp = new NANDGateComponent(m_x, m_y);
        } else if (m_type == "NOR") {
            comp = new NORGateComponent(m_x, m_y);
        } else if (m_type == "NOT") {
            comp = new NOTGateComponent(m_x, m_y);
        } else if (m_type == "BUF") {
            comp = new BufferComponent(m_x, m_y);
        } else {
            // Default to buffer if type is unknown
            comp = new BufferComponent(m_x, m_y);
        }
        
        if (comp) {
            m_canvas->AddComponent(comp);
            m_executed = false;
        }
    }
}

void DeleteComponentCommand::Redo()
{
    if (!m_executed) {
        Execute();  // Execute does the deletion again
    }
}

// MoveComponentCommand implementation
MoveComponentCommand::MoveComponentCommand(CircuitCanvas* canvas, void* component, int oldX, int oldY, int newX, int newY)
    : m_canvas(canvas), m_component(component), 
      m_oldX(oldX), m_oldY(oldY), m_newX(newX), m_newY(newY), m_executed(false)
{
}

void MoveComponentCommand::Execute()
{
    if (!m_executed) {
        Component* comp = static_cast<Component*>(m_component);
        comp->SetPosition(wxPoint(m_newX, m_newY));
        m_canvas->Refresh();
        m_executed = true;
    }
}

void MoveComponentCommand::Undo()
{
    if (m_executed) {
        Component* comp = static_cast<Component*>(m_component);
        comp->SetPosition(wxPoint(m_oldX, m_oldY));
        m_canvas->Refresh();
        m_executed = false;
    }
}

void MoveComponentCommand::Redo()
{
    if (!m_executed) {
        Execute();  // Execute changes position to new location again
    }
}

// AddWireCommand implementation
AddWireCommand::AddWireCommand(CircuitCanvas* canvas, void* startPin, void* endPin)
    : m_canvas(canvas), m_startPin(startPin), m_endPin(endPin), m_wire(nullptr), m_executed(false)
{
}

void AddWireCommand::Execute()
{
    if (!m_executed) {
        Pin* start = static_cast<Pin*>(m_startPin);
        Pin* end = static_cast<Pin*>(m_endPin);
        Wire* wire = new SimpleWire(start, end);
        m_canvas->AddWire(wire);
        m_wire = wire;
        m_executed = true;
    }
}

void AddWireCommand::Undo()
{
    if (m_executed) {
        std::vector<Wire*>& wires = m_canvas->GetWires();
        auto it = std::find(wires.begin(), wires.end(), static_cast<Wire*>(m_wire));
        if (it != wires.end()) {
            wires.erase(it);
            m_canvas->Refresh();
            m_executed = false;
        }
    }
}

void AddWireCommand::Redo()
{
    if (!m_executed) {
        Execute();  // Execute does the adding again
    }
}

// DeleteWireCommand implementation
DeleteWireCommand::DeleteWireCommand(CircuitCanvas* canvas, void* wire)
    : m_canvas(canvas), m_wire(wire), m_executed(false)
{
    // Store wire data for potential restoration
    Wire* w = static_cast<Wire*>(wire);
    m_startPin = w->GetStartPin();
    m_endPin = w->GetEndPin();
}

void DeleteWireCommand::Execute()
{
    if (!m_executed) {
        std::vector<Wire*>& wires = m_canvas->GetWires();
        auto it = std::find(wires.begin(), wires.end(), static_cast<Wire*>(m_wire));
        if (it != wires.end()) {
            wires.erase(it);
            m_canvas->Refresh();
            m_executed = true;
        }
    }
}

void DeleteWireCommand::Undo()
{
    if (m_executed) {
        Pin* start = static_cast<Pin*>(m_startPin);
        Pin* end = static_cast<Pin*>(m_endPin);
        Wire* wire = new SimpleWire(start, end);
        m_canvas->AddWire(wire);
        m_executed = false;
    }
}

void DeleteWireCommand::Redo()
{
    if (!m_executed) {
        Execute();  // Execute does the deletion again
    }
}

// UndoRedoManager implementation
UndoRedoManager::UndoRedoManager()
{
}

UndoRedoManager::~UndoRedoManager()
{
    Clear();
}

void UndoRedoManager::PushCommand(std::unique_ptr<UndoCommand> command)
{
    command->Execute();  // Execute the command
    
    m_undoStack.push_back(std::move(command));
    
    // Limit the undo stack size
    if (m_undoStack.size() > MAX_UNDO_LEVELS) {
        m_undoStack.erase(m_undoStack.begin());
    }
    
    // Clear redo stack when a new command is added
    m_redoStack.clear();
}

void UndoRedoManager::Undo()
{
    if (!m_undoStack.empty()) {
        std::unique_ptr<UndoCommand> command = std::move(m_undoStack.back());
        m_undoStack.pop_back();
        
        command->Undo();
        
        m_redoStack.push_back(std::move(command));
    }
}

void UndoRedoManager::Redo()
{
    if (!m_redoStack.empty()) {
        std::unique_ptr<UndoCommand> command = std::move(m_redoStack.back());
        m_redoStack.pop_back();
        
        command->Redo();
        
        m_undoStack.push_back(std::move(command));
    }
}

void UndoRedoManager::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

wxString UndoRedoManager::GetUndoActionName() const
{
    if (!m_undoStack.empty()) {
        return m_undoStack.back()->GetName();
    }
    return "Undo";
}

wxString UndoRedoManager::GetRedoActionName() const
{
    if (!m_redoStack.empty()) {
        return m_redoStack.back()->GetName();
    }
    return "Redo";
}