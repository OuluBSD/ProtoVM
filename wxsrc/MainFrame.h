#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/aui/aui.h>

class CircuitCanvas;  // Forward declaration
class PropertiesPanel;  // Forward declaration
class ComponentPalette;  // Forward declaration

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title);

private:
    void CreateMenus();
    void CreateToolbar();
    void CreateStatusBar();
    void CreateLayout();

    // Event handlers
    void OnExit(wxCommandEvent& event);
    void OnNewProject(wxCommandEvent& event);
    void OnOpenProject(wxCommandEvent& event);
    void OnSaveProject(wxCommandEvent& event);
    void OnSaveProjectAs(wxCommandEvent& event);
    void OnToggleWireMode(wxCommandEvent& event);
    void OnStartSimulation(wxCommandEvent& event);
    void OnStopSimulation(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnToggleGrid(wxCommandEvent& event);
    void OnToggleSnapToGrid(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);
    void OnZoomReset(wxCommandEvent& event);
    void OnStartSimulation(wxCommandEvent& event);
    void OnPauseSimulation(wxCommandEvent& event);
    void OnStopSimulation(wxCommandEvent& event);
    void OnStepSimulation(wxCommandEvent& event);

    CircuitCanvas* m_canvas;
    PropertiesPanel* m_propertiesPanel;
    ComponentPalette* m_componentPalette;
    class SimulationController* m_simulationController;
    wxString m_currentFilePath;  // Path of currently loaded project file

    wxDECLARE_EVENT_TABLE();
};

#endif // MAINFRAME_H