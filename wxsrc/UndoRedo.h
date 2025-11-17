#ifndef UNDOREDO_H
#define UNDOREDO_H

#include <vector>
#include <memory>

// Forward declarations
class CircuitCanvas;

// Base class for undo/redo commands
class UndoCommand {
public:
    virtual ~UndoCommand() = default;
    virtual void Execute() = 0;    // Perform the command
    virtual void Undo() = 0;       // Undo the command
    virtual void Redo() = 0;       // Redo the command
    virtual wxString GetName() const = 0;  // Get the command name for display
};

// Specific command implementations
class AddComponentCommand : public UndoCommand {
public:
    AddComponentCommand(CircuitCanvas* canvas, int x, int y, const wxString& type);
    virtual void Execute() override;
    virtual void Undo() override;
    virtual void Redo() override;
    virtual wxString GetName() const override { return "Add Component"; }

private:
    CircuitCanvas* m_canvas;
    int m_x, m_y;
    wxString m_type;
    void* m_component;  // Store component as void* to avoid including Component header
    bool m_executed;
};

class DeleteComponentCommand : public UndoCommand {
public:
    DeleteComponentCommand(CircuitCanvas* canvas, void* component);
    virtual void Execute() override;
    virtual void Undo() override;
    virtual void Redo() override;
    virtual wxString GetName() const override { return "Delete Component"; }

private:
    CircuitCanvas* m_canvas;
    void* m_component;
    bool m_executed;
    // Store component data for potential restoration
    int m_x, m_y;
    wxString m_name;
    wxString m_type;
};

class MoveComponentCommand : public UndoCommand {
public:
    MoveComponentCommand(CircuitCanvas* canvas, void* component, int oldX, int oldY, int newX, int newY);
    virtual void Execute() override;
    virtual void Undo() override;
    virtual void Redo() override;
    virtual wxString GetName() const override { return "Move Component"; }

private:
    CircuitCanvas* m_canvas;
    void* m_component;
    int m_oldX, m_oldY;
    int m_newX, m_newY;
    bool m_executed;
};

class AddWireCommand : public UndoCommand {
public:
    AddWireCommand(CircuitCanvas* canvas, void* startPin, void* endPin);
    virtual void Execute() override;
    virtual void Undo() override;
    virtual void Redo() override;
    virtual wxString GetName() const override { return "Add Wire"; }

private:
    CircuitCanvas* m_canvas;
    void* m_startPin;
    void* m_endPin;
    void* m_wire;  // The created wire
    bool m_executed;
};

class DeleteWireCommand : public UndoCommand {
public:
    DeleteWireCommand(CircuitCanvas* canvas, void* wire);
    virtual void Execute() override;
    virtual void Undo() override;
    virtual void Redo() override;
    virtual wxString GetName() const override { return "Delete Wire"; }

private:
    CircuitCanvas* m_canvas;
    void* m_wire;
    void* m_startPin;
    void* m_endPin;
    bool m_executed;
};

// Undo/Redo manager class
class UndoRedoManager {
public:
    UndoRedoManager();
    ~UndoRedoManager();
    
    void PushCommand(std::unique_ptr<UndoCommand> command);
    void Undo();
    void Redo();
    
    bool CanUndo() const { return m_undoStack.size() > 0; }
    bool CanRedo() const { return m_redoStack.size() > 0; }
    
    wxString GetUndoActionName() const;
    wxString GetRedoActionName() const;
    
    void Clear();  // Clear the undo/redo stacks

private:
    std::vector<std::unique_ptr<UndoCommand>> m_undoStack;
    std::vector<std::unique_ptr<UndoCommand>> m_redoStack;
    static const int MAX_UNDO_LEVELS = 50;  // Limit undo levels to prevent excessive memory usage
};

#endif // UNDOREDO_H