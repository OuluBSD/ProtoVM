#include "MainFrame.h"
#include "CircuitCanvas.h"
#include "PropertiesPanel.h"
#include "ComponentPalette.h"
#include "SimulationController.h"
#include "SimulationBridge.h"
#include "SimulationInterface.h"
#include "CircuitData.h"
#include "CircuitSerializer.h"
#include <wx/artprov.h>

// Event table for MainFrame
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_EXIT, MainFrame::OnExit)
    EVT_MENU(wxID_NEW, MainFrame::OnNewProject)
    EVT_MENU(wxID_OPEN, MainFrame::OnOpenProject)
    EVT_MENU(wxID_SAVE, MainFrame::OnSaveProject)
    EVT_MENU(wxID_SAVEAS, MainFrame::OnSaveProjectAs)
    EVT_MENU(wxID_UNDO, MainFrame::OnUndo)
    EVT_MENU(wxID_REDO, MainFrame::OnRedo)
    EVT_MENU(wxID_ANY, MainFrame::OnToggleGrid)
    EVT_MENU(wxID_ANY, MainFrame::OnToggleSnapToGrid)
    EVT_MENU(wxID_ANY, MainFrame::OnZoomIn)
    EVT_MENU(wxID_ANY, MainFrame::OnZoomOut)
    EVT_MENU(wxID_ANY, MainFrame::OnZoomReset)
    EVT_MENU(wxID_ANY, MainFrame::OnToggleWireMode)
    EVT_MENU(wxID_ANY, MainFrame::OnStartSimulation)
    EVT_MENU(wxID_ANY, MainFrame::OnPauseSimulation)
    EVT_MENU(wxID_ANY, MainFrame::OnStopSimulation)
    EVT_MENU(wxID_ANY, MainFrame::OnStepSimulation)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 800)), 
      m_canvas(nullptr), m_propertiesPanel(nullptr), m_componentPalette(nullptr), m_simulationController(nullptr), m_simulationBridge(nullptr)
{
    CreateMenus();
    CreateToolbar();
    CreateStatusBar();
    CreateLayout();

    // Create and initialize the simulation controller
    m_simulationController = new SimulationController();
    if (m_canvas) {
        m_canvas->SetSimulationController(m_simulationController);
        m_simulationController->SetCanvas(m_canvas);
        
        // Set up the update callback to refresh the canvas during simulation
        m_simulationController->SetUpdateCallback([this]() {
            if (m_canvas) {
                m_canvas->Refresh();
            }
        });
    }

    // Set up the selection callback to update the properties panel
    if (m_canvas && m_propertiesPanel) {
        m_canvas->SetSelectionChangedCallback([this](Component* comp) {
            m_propertiesPanel->UpdateProperties(comp);
        });
    }

    // Initialize the bridge between GUI and simulation engine
    InitializeSimulationBridge();
    Centre();
}

void MainFrame::CreateMenus()
{
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_NEW, "&New Project\tCtrl+N", "Create a new circuit project");
    fileMenu->Append(wxID_OPEN, "&Open Project\tCtrl+O", "Open an existing circuit project");
    fileMenu->Append(wxID_SAVE, "&Save Project\tCtrl+S", "Save the current circuit project");
    fileMenu->Append(wxID_SAVEAS, "Save Project &As\tCtrl+Shift+S", "Save the current circuit project with a new name");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Quit this program");

    wxMenu* editMenu = new wxMenu;
    editMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z", "Undo last action");
    editMenu->Append(wxID_REDO, "&Redo\tCtrl+Y", "Redo last action");

    wxMenu* viewMenu = new wxMenu;
    wxMenuItem* gridItem = new wxMenuItem(viewMenu, wxID_ANY, "&Grid\tCtrl+G", "Toggle grid visibility", wxITEM_CHECK);
    wxMenuItem* snapItem = new wxMenuItem(viewMenu, wxID_ANY, "&Snap to Grid\tCtrl+Shift+G", "Toggle snap to grid", wxITEM_CHECK);
    wxMenuItem* zoomInItem = new wxMenuItem(viewMenu, wxID_ANY, "Zoom &In\tCtrl++", "Zoom in");
    wxMenuItem* zoomOutItem = new wxMenuItem(viewMenu, wxID_ANY, "Zoom &Out\tCtrl+-", "Zoom out");
    wxMenuItem* zoomResetItem = new wxMenuItem(viewMenu, wxID_ANY, "Reset &Zoom\tCtrl+0", "Reset zoom to 100%");
    viewMenu->Append(gridItem);
    viewMenu->Append(snapItem);
    viewMenu->AppendSeparator();
    viewMenu->Append(zoomInItem);
    viewMenu->Append(zoomOutItem);
    viewMenu->Append(zoomResetItem);
    // Set initial state
    gridItem->Check(true);  // Grid is on by default
    snapItem->Check(true);  // Snap to grid is on by default

    wxMenu* toolsMenu = new wxMenu;
    wxMenuItem* wireModeItem = new wxMenuItem(toolsMenu, wxID_ANY, "&Wire Mode\tW", "Toggle wire creation mode");
    toolsMenu->Append(wireModeItem);

    wxMenu* simulateMenu = new wxMenu;
    wxMenuItem* startSimItem = new wxMenuItem(simulateMenu, wxID_ANY, "&Start Simulation\tF5", "Start circuit simulation");
    wxMenuItem* pauseSimItem = new wxMenuItem(simulateMenu, wxID_ANY, "&Pause Simulation\tF6", "Pause circuit simulation");
    wxMenuItem* stopSimItem = new wxMenuItem(simulateMenu, wxID_ANY, "S&top Simulation\tShift+F5", "Stop circuit simulation");
    wxMenuItem* stepSimItem = new wxMenuItem(simulateMenu, wxID_ANY, "Step S&imulation\tF7", "Run one simulation step");
    simulateMenu->Append(startSimItem);
    simulateMenu->Append(pauseSimItem);
    simulateMenu->Append(stopSimItem);
    simulateMenu->AppendSeparator();
    simulateMenu->Append(stepSimItem);

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(editMenu, "&Edit");
    menuBar->Append(viewMenu, "&View");
    menuBar->Append(toolsMenu, "&Tools");
    menuBar->Append(simulateMenu, "&Simulate");

    SetMenuBar(menuBar);
}

void MainFrame::CreateToolbar()
{
    wxToolBar* toolbar = CreateToolBar();
    toolbar->AddTool(wxID_NEW, "New", wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR), "Create a new project");
    toolbar->AddTool(wxID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR), "Open a project");
    toolbar->AddTool(wxID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), "Save the project");
    toolbar->AddSeparator();
    
    // Add simulation controls - using specific IDs to bind to events
    toolbar->AddTool(1000, "Start", wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR), "Start simulation");
    toolbar->AddTool(1001, "Stop", wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR), "Stop simulation");
    
    toolbar->Realize();
    
    // Bind toolbar events
    Bind(wxEVT_TOOL, &MainFrame::OnStartSimulation, this, 1000);
    Bind(wxEVT_TOOL, &MainFrame::OnStopSimulation, this, 1001);
}

void MainFrame::CreateStatusBar()
{
    wxFrame::CreateStatusBar(2);
    SetStatusText("Ready", 0);
    SetStatusText("No project loaded", 1);
}

void MainFrame::CreateLayout()
{
    // Create the main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create component library panel with search
    m_componentPalette = new ComponentPalette(this, wxID_ANY);
    m_componentPalette->SetMinSize(wxSize(250, -1)); // Give more space for the component library
    m_componentPalette->SetBackgroundColour(wxColour(240, 240, 240));

    // Create the splitter window for palette and canvas
    wxSplitterWindow* horizontalSplitter = new wxSplitterWindow(this, wxID_ANY);
    horizontalSplitter->SetMinimumPaneSize(100); // Minimum size for panels

    // Create the circuit canvas
    m_canvas = new CircuitCanvas(horizontalSplitter, wxID_ANY);
    
    // Add some sample components to demonstrate the drawing system
    m_canvas->AddComponent(new NANDGateComponent(100, 100));
    m_canvas->AddComponent(new NOTGateComponent(300, 100));
    m_canvas->AddComponent(new BufferComponent(100, 250));
    m_canvas->AddComponent(new NORGateComponent(300, 250));

    // Create properties panel
    m_propertiesPanel = new PropertiesPanel(horizontalSplitter, wxID_ANY);

    // Add panels to horizontal splitter (canvas and properties)
    wxSplitterWindow* canvasSplitter = new wxSplitterWindow(horizontalSplitter, wxID_ANY);
    canvasSplitter->SetMinimumPaneSize(100);
    canvasSplitter->SplitVertically(m_canvas, m_propertiesPanel);
    canvasSplitter->SetSashGravity(0.8); // Give more space to canvas

    // Split horizontally between component palette and the other two panels
    horizontalSplitter->SplitVertically(m_componentPalette, canvasSplitter);
    horizontalSplitter->SetSashGravity(0.15); // Give about 15% space to the component palette

    // Add the splitter to the main sizer
    mainSizer->Add(horizontalSplitter, 1, wxEXPAND);

    // Set the main sizer for the frame
    SetSizer(mainSizer);
}

void MainFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void MainFrame::OnNewProject(wxCommandEvent& event)
{
    // Clear the canvas
    if (m_canvas)
    {
        // In a real implementation, we would properly clear the canvas
        // For now, we'll just recreate the sample components
        auto components = m_canvas->GetComponents();
        auto wires = m_canvas->GetWires();
        
        // Delete all components and wires
        for (auto* comp : components) {
            delete comp;
        }
        for (auto* wire : wires) {
            delete wire;
        }
        
        components.clear();
        wires.clear();
        
        // Add sample components back
        m_canvas->AddComponent(new NANDGateComponent(100, 100));
        m_canvas->AddComponent(new NOTGateComponent(300, 100));
        m_canvas->AddComponent(new BufferComponent(100, 250));
        m_canvas->AddComponent(new NORGateComponent(300, 250));
        
        m_canvas->Refresh();
    }
    
    m_currentFilePath.Clear();
    SetStatusText("New project created", 0);
    SetStatusText("No project loaded", 1);
}

void MainFrame::OnOpenProject(wxCommandEvent& event)
{
    if (!m_canvas) return;
    
    wxFileDialog openFileDialog(this, _("Open Circuit Project"), "", "",
                               "Circuit files (*.circuit)|*.circuit|All files (*.*)|*.*",
                               wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (openFileDialog.ShowModal() == wxID_OK)
    {
        wxString filepath = openFileDialog.GetPath();
        
        // Load the circuit data from file
        CircuitData circuitData;
        if (CircuitSerializer::LoadCircuit(filepath, circuitData))
        {
            // Deserialize the data to the canvas
            m_canvas->DeserializeFromData(circuitData);
            
            m_currentFilePath = filepath;
            SetStatusText("Project loaded: " + filepath, 0);
            wxString filename = wxFileName(filepath).GetFullName();
            SetStatusText("Project: " + filename, 1);
            
            // Update the window title to show the loaded file
            SetTitle("ProtoVM Circuit Designer - " + filename);
        }
        else
        {
            wxMessageBox("Failed to load the circuit file!", "Error", wxOK | wxICON_ERROR, this);
        }
    }
}

void MainFrame::OnSaveProject(wxCommandEvent& event)
{
    if (!m_canvas) return;
    
    if (m_currentFilePath.IsEmpty())
    {
        // If no file is currently open, prompt for a new file
        OnSaveProjectAs(event);
        return;
    }
    
    // Serialize the canvas to circuit data
    CircuitData circuitData;
    m_canvas->SerializeToData(circuitData);
    
    // Save the circuit data to file
    if (CircuitSerializer::SaveCircuit(circuitData, m_currentFilePath))
    {
        SetStatusText("Project saved: " + m_currentFilePath, 0);
    }
    else
    {
        wxMessageBox("Failed to save the circuit file!", "Error", wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnSaveProjectAs(wxCommandEvent& event)
{
    if (!m_canvas) return;
    
    wxFileDialog saveFileDialog(this, _("Save Circuit Project"), "", "",
                               "Circuit files (*.circuit)|*.circuit|All files (*.*)|*.*",
                               wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (saveFileDialog.ShowModal() == wxID_OK)
    {
        wxString filepath = saveFileDialog.GetPath();
        
        // Make sure the file has the correct extension
        if (!filepath.EndsWith(".circuit"))
            filepath += ".circuit";
        
        // Serialize the canvas to circuit data
        CircuitData circuitData;
        m_canvas->SerializeToData(circuitData);
        
        // Save the circuit data to file
        if (CircuitSerializer::SaveCircuit(circuitData, filepath))
        {
            m_currentFilePath = filepath;
            SetStatusText("Project saved: " + filepath, 0);
            wxString filename = wxFileName(filepath).GetFullName();
            SetStatusText("Project: " + filename, 1);
            
            // Update the window title to show the saved file
            SetTitle("ProtoVM Circuit Designer - " + filename);
        }
        else
        {
            wxMessageBox("Failed to save the circuit file!", "Error", wxOK | wxICON_ERROR, this);
        }
    }
}

void MainFrame::OnToggleWireMode(wxCommandEvent& event)
{
    if (m_canvas)
    {
        // Toggle wire creation mode on the canvas
        bool wasInWireMode = m_canvas->IsInWireCreationMode();
        m_canvas->ToggleWireCreationMode(!wasInWireMode);
        
        // Update status and menu item
        if (m_canvas->IsInWireCreationMode()) {
            SetStatusText("Wire mode activated. Right-click on a pin to start a connection, then left-click on another pin to complete it.", 0);
        } else {
            SetStatusText("Wire mode deactivated", 0);
        }
    }
}

void MainFrame::OnStartSimulation(wxCommandEvent& event)
{
    if (m_simulationController)
    {
        // Initialize the simulation bridge if not already done
        if (!m_simulationBridge) {
            // Create a mock simulation engine for the bridge
            // In a real implementation, this would connect to the actual ProtoVM engine
            void* dummyEngine = nullptr;  // Placeholder - would be actual engine in real implementation
            
            // Create the simulation bridge
            m_simulationBridge = new SimulationBridge(reinterpret_cast<ISimulationEngine*>(dummyEngine), m_canvas);
            
            // Connect the bridge to the simulation controller
            m_simulationController->SetSimulationBridge(m_simulationBridge);
            
            // Initialize the bridge with current circuit
            m_simulationBridge->InitializeSimulation();
        }
        
        m_simulationController->StartSimulation();
        SetStatusText("Simulation started", 0);
    }
}

void MainFrame::OnPauseSimulation(wxCommandEvent& event)
{
    if (m_simulationController)
    {
        m_simulationController->PauseSimulation();
        SetStatusText("Simulation paused", 0);
    }
}

void MainFrame::OnStopSimulation(wxCommandEvent& event)
{
    if (m_simulationController)
    {
        m_simulationController->StopSimulation();
        SetStatusText("Simulation stopped", 0);
    }
    
    // Reset the simulation bridge to initial state
    if (m_simulationBridge) {
        m_simulationBridge->ResetSimulation();
    }
}

void MainFrame::OnStepSimulation(wxCommandEvent& event)
{
    if (m_simulationController)
    {
        m_simulationController->StepSimulation();
        SetStatusText("Single simulation step executed", 0);
        
        // If we have a simulation bridge, run the bridge's step as well
        if (m_simulationBridge) {
            m_simulationBridge->RunSimulationStep();
        }
    }
}

void MainFrame::OnUndo(wxCommandEvent& event)
{
    if (m_canvas)
    {
        m_canvas->Undo();
    }
}

void MainFrame::OnRedo(wxCommandEvent& event)
{
    if (m_canvas)
    {
        m_canvas->Redo();
    }
}

void MainFrame::OnToggleGrid(wxCommandEvent& event)
{
    if (m_canvas)
    {
        bool gridEnabled = !m_canvas->IsGridEnabled();
        m_canvas->SetGridEnabled(gridEnabled);
        m_canvas->Refresh();
        
        // Update menu item check state
        wxMenuItem* item = static_cast<wxMenuItem*>(event.GetEventObject());
        if (item) {
            item->Check(gridEnabled);
        }
    }
}

void MainFrame::OnToggleSnapToGrid(wxCommandEvent& event)
{
    if (m_canvas)
    {
        bool snapEnabled = !m_canvas->GetSnapToGrid();
        m_canvas->SetSnapToGrid(snapEnabled);
        
        // Update menu item check state
        wxMenuItem* item = static_cast<wxMenuItem*>(event.GetEventObject());
        if (item) {
            item->Check(snapEnabled);
        }
    }
}

void MainFrame::OnZoomIn(wxCommandEvent& event)
{
    if (m_canvas)
    {
        m_canvas->ZoomIn();
    }
}

void MainFrame::OnZoomOut(wxCommandEvent& event)
{
    if (m_canvas)
    {
        m_canvas->ZoomOut();
    }
}

void MainFrame::OnZoomReset(wxCommandEvent& event)
{
    if (m_canvas)
    {
        m_canvas->ResetZoom();
    }
}
void MainFrame::InitializeSimulationBridge()
{
    if (m_canvas && m_simulationController) {
        // For now, we'll create a mock Machine for the bridge
        // In a real implementation, we'd connect to the actual ProtoVM Machine

        // Since we can't directly include Machine here due to header complexity,
        // we'll implement the bridge initialization when the user starts simulation
        m_simulationController->SetUpdateCallback([this]() {
            if (m_canvas) {
                m_canvas->Refresh();
            }
        });
    }
}
