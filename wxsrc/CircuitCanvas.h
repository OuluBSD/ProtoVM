#ifndef CIRCUITCANVAS_H
#define CIRCUITCANVAS_H

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/timer.h>
#include <vector>
#include <algorithm>  // For std::find
#include <functional>
#include <memory>     // For std::unique_ptr
#include <map>        // For std::map

// Forward declarations
class Component;
class Wire;
class CircuitData;      // Forward declaration for CircuitData
class UndoCommand;      // Forward declaration for UndoCommand
class UndoRedoManager;  // Forward declaration for UndoRedoManager
class Pin;              // Forward declaration for Pin class

class CircuitCanvas : public wxPanel
{
public:
    CircuitCanvas(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~CircuitCanvas();
    
    void AddComponent(Component* component);
    void AddWire(Wire* wire);
    
    std::vector<Component*>& GetComponents() { return m_components; }
    std::vector<Wire*>& GetWires() { return m_wires; }
    
    // Serialization methods
    void SerializeToData(CircuitData& data) const;
    void DeserializeFromData(const CircuitData& data);
    
    // Animation methods
    void StartAnimation();
    void StopAnimation();
    void UpdateAnimation(float deltaTime);
    void UpdateWireStates();  // Update wire states based on component outputs

    // Undo/Redo methods
    void PushUndoCommand(std::unique_ptr<UndoCommand> command);
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();

    // Selection methods
    Component* GetSelectedComponent() const { return m_selectedComponent; }
    void SetSelectionChangedCallback(std::function<void(Component*)> callback) { m_selectionChangedCallback = callback; }

    // Grid and snapping methods
    void SetGridEnabled(bool enabled) { m_gridEnabled = enabled; }
    bool IsGridEnabled() const { return m_gridEnabled; }
    void SetGridSpacing(int spacing) { m_gridSpacing = spacing; }
    int GetGridSpacing() const { return m_gridSpacing; }
    void SetSnapToGrid(bool snap) { m_snapToGrid = snap; }
    bool GetSnapToGrid() const { return m_snapToGrid; }
    wxPoint SnapToGrid(const wxPoint& point) const;

    // Zoom and pan methods
    void SetZoomFactor(double factor);
    double GetZoomFactor() const { return m_zoomFactor; }
    void ZoomIn();
    void ZoomOut();
    void ResetZoom();
    void Pan(int dx, int dy);
    wxPoint LogicalToPhysical(const wxPoint& logicalPoint) const;
    wxPoint PhysicalToLogical(const wxPoint& physicalPoint) const;

    // Selection methods
    void SelectComponent(Component* comp, bool additive = false);  // Select a single component
    void SelectAllComponents();
    void ClearSelection();
    void DeleteSelectedComponents();
    std::vector<Component*>& GetSelectedComponents() { return m_selectedComponents; }
    bool IsComponentSelected(Component* comp) const;

    // Simulation methods
    void SetSimulationController(class SimulationController* simController) { m_simulationController = simController; }
    class SimulationController* GetSimulationController() const { return m_simulationController; }

    // Wire creation mode methods
    void ToggleWireCreationMode(bool enabled) { m_wireCreationMode = enabled; }
    bool IsInWireCreationMode() const { return m_wireCreationMode; }

protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);
    void OnMouseMotion(wxMouseEvent& event);
    void OnMouseRightDown(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);

private:
    std::vector<Component*> m_components;
    std::vector<Wire*> m_wires;
    
    // For dragging and selection
    bool m_dragging;
    wxPoint m_lastMousePos;
    wxPoint m_originalComponentPos;  // Original position for the first selected component during drag
    Component* m_selectedComponent;  // Primary selected component
    std::vector<Component*> m_selectedComponents;  // All selected components
    
    // Store original positions during drag operations for multi-selection
    std::map<Component*, wxPoint> m_originalPositions;
    
    // For wire creation
    bool m_wireCreationMode;
    Pin* m_startPin;
    wxPoint m_currentWireEndPoint;
    
    // For animation
    wxTimer* m_animationTimer;
    
    // For undo/redo
    class UndoRedoManager* m_undoRedoManager;

    // Selection callback
    std::function<void(Component*)> m_selectionChangedCallback;

    // Grid and snapping
    bool m_gridEnabled;
    bool m_snapToGrid;
    int m_gridSpacing;
    
    // Zoom and pan
    double m_zoomFactor;
    wxPoint m_panOffset;
    wxPoint m_lastMousePosition;
    bool m_panning;
    
    // Simulation
    class SimulationController* m_simulationController;

    // Event handlers
    void OnAnimationTimer(wxTimerEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnMiddleMouseDown(wxMouseEvent& event);
    void OnMiddleMouseUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};

// Pin class to represent connection points on components
class Pin
{
public:
    Pin(int x, int y, const wxString& name, bool isInput) 
        : m_pos(x, y), m_name(name), m_isInput(isInput), m_connected(false) {}
    
    wxPoint GetPosition() const { return m_pos; }
    void SetPosition(const wxPoint& pos) { m_pos = pos; }
    wxString GetName() const { return m_name; }
    bool IsInput() const { return m_isInput; }
    bool IsConnected() const { return m_connected; }
    void SetConnected(bool connected) { m_connected = connected; }

private:
    wxPoint m_pos;
    wxString m_name;
    bool m_isInput;
    bool m_connected;
};

// Base class for all circuit components
class Component
{
public:
    Component(int x, int y, const wxString& name) : m_pos(x, y), m_name(name), m_selected(false) {}
    virtual ~Component() {}
    
    virtual void Draw(wxDC& dc) = 0;
    virtual bool Contains(const wxPoint& pos) const = 0;
    virtual void Move(int dx, int dy) { m_pos.x += dx; m_pos.y += dy; }
    virtual wxRect GetBounds() const = 0;
    virtual std::vector<Pin>& GetInputPins() = 0;
    virtual std::vector<Pin>& GetOutputPins() = 0;
    
    wxPoint GetPosition() const { return m_pos; }
    void SetPosition(const wxPoint& pos) { m_pos = pos; }
    wxString GetName() const { return m_name; }
    void SetName(const wxString& name) { m_name = name; }
    bool IsSelected() const { return m_selected; }
    void SetSelected(bool selected) { m_selected = selected; }

protected:
    wxPoint m_pos;
    wxString m_name;
    bool m_selected;
};

// Class for wires connecting components
class Wire
{
public:
    Wire(Pin* start, Pin* end)
        : m_startPin(start), m_endPin(end), m_active(false), m_propagationPosition(0.0f), m_animationActive(false) {}
    virtual ~Wire() {}
    
    virtual void Draw(wxDC& dc) = 0;
    
    Pin* GetStartPin() const { return m_startPin; }
    Pin* GetEndPin() const { return m_endPin; }
    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }
    
    // Animation methods
    void SetAnimationActive(bool active) { m_animationActive = active; }
    bool IsAnimationActive() const { return m_animationActive; }
    void UpdateAnimation(float deltaTime);  // Update animation based on time passed
    float GetPropagationPosition() const { return m_propagationPosition; }
    void ResetPropagation() { m_propagationPosition = 0.0f; }

private:
    Pin* m_startPin;
    Pin* m_endPin;
    bool m_active;  // Whether the wire is carrying a signal
    float m_propagationPosition;  // Position of signal propagation [0.0-1.0]
    bool m_animationActive;  // Whether animation is currently running
};

// Concrete implementation of a NAND gate component
class NANDGateComponent : public Component
{
public:
    NANDGateComponent(int x, int y);
    
    virtual void Draw(wxDC& dc) override;
    virtual bool Contains(const wxPoint& pos) const override;
    virtual wxRect GetBounds() const override;
    virtual std::vector<Pin>& GetInputPins() override;
    virtual std::vector<Pin>& GetOutputPins() override;

private:
    wxRect GetBodyRect() const;
    std::vector<Pin> m_inputPins;
    std::vector<Pin> m_outputPins;
};

// Concrete implementation of a NOR gate component
class NORGateComponent : public Component
{
public:
    NORGateComponent(int x, int y);
    
    virtual void Draw(wxDC& dc) override;
    virtual bool Contains(const wxPoint& pos) const override;
    virtual wxRect GetBounds() const override;
    virtual std::vector<Pin>& GetInputPins() override;
    virtual std::vector<Pin>& GetOutputPins() override;

private:
    wxRect GetBodyRect() const;
    std::vector<Pin> m_inputPins;
    std::vector<Pin> m_outputPins;
};

// Concrete implementation of a NOT gate (inverter) component
class NOTGateComponent : public Component
{
public:
    NOTGateComponent(int x, int y);
    
    virtual void Draw(wxDC& dc) override;
    virtual bool Contains(const wxPoint& pos) const override;
    virtual wxRect GetBounds() const override;
    virtual std::vector<Pin>& GetInputPins() override;
    virtual std::vector<Pin>& GetOutputPins() override;

private:
    wxRect GetBodyRect() const;
    std::vector<Pin> m_inputPins;
    std::vector<Pin> m_outputPins;
};

// Concrete implementation of a buffer component
class BufferComponent : public Component
{
public:
    BufferComponent(int x, int y);
    
    virtual void Draw(wxDC& dc) override;
    virtual bool Contains(const wxPoint& pos) const override;
    virtual wxRect GetBounds() const override;
    virtual std::vector<Pin>& GetInputPins() override;
    virtual std::vector<Pin>& GetOutputPins() override;

private:
    wxRect GetBodyRect() const;
    std::vector<Pin> m_inputPins;
    std::vector<Pin> m_outputPins;
};

// Wire implementation with visual feedback
class SimpleWire : public Wire
{
public:
    SimpleWire(Pin* start, Pin* end);
    
    virtual void Draw(wxDC& dc) override;
};

#endif // CIRCUITCANVAS_H