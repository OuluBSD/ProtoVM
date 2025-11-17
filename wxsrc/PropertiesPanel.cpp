#include "PropertiesPanel.h"
#include "CircuitCanvas.h"
#include <wx/propgrid/manager.h>

wxBEGIN_EVENT_TABLE(PropertiesPanel, wxPanel)
    EVT_PG_CHANGED(wxID_ANY, PropertiesPanel::OnPropertyGridChange)
wxEND_EVENT_TABLE()

PropertiesPanel::PropertiesPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    // Create the property grid
    m_propertyGrid = new wxPropertyGrid(
        this, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxPG_SPLITTER_AUTO_CENTER | wxPG_TOOLTIPS
    );
    
    // Set up the sizer
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_propertyGrid, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
    
    // Initially clear the properties
    ClearProperties();
}

void PropertiesPanel::UpdateProperties(Component* component)
{
    if (!component) {
        ClearProperties();
        return;
    }
    
    // Clear existing properties
    m_propertyGrid->Clear();
    
    // Add general properties
    m_propertyGrid->Append(new wxPropertyCategory("General"));
    m_propertyGrid->Append(new wxStringProperty("Name", "name", component->GetName()));
    wxPoint pos = component->GetPosition();
    wxString posStr = wxString::Format("%d, %d", pos.x, pos.y);
    m_propertyGrid->Append(new wxStringProperty("Position", "position", posStr));
    
    // Add component-specific properties
    m_propertyGrid->Append(new wxPropertyCategory("Component"));
    
    // For now, just add a type property - in a full implementation we'd have specific properties
    // based on the component type (NAND, NOR, etc.)
    m_propertyGrid->Append(new wxStringProperty("Type", "type", component->GetName()));
    
    // Add pin information if available
    m_propertyGrid->Append(new wxPropertyCategory("Pins"));
    
    // Add input pins
    for (const Pin& pin : component->GetInputPins()) {
        wxString pinName = "Input: " + pin.GetName();
        wxString pinPos = wxString::Format("%d, %d", pin.GetPosition().x, pin.GetPosition().y);
        m_propertyGrid->Append(new wxStringProperty(pinName, pin.GetName() + "_input", pinPos));
    }
    
    // Add output pins
    for (const Pin& pin : component->GetOutputPins()) {
        wxString pinName = "Output: " + pin.GetName();
        wxString pinPos = wxString::Format("%d, %d", pin.GetPosition().x, pin.GetPosition().y);
        m_propertyGrid->Append(new wxStringProperty(pinName, pin.GetName() + "_output", pinPos));
    }
    
    // Additional properties can be added based on component type
    if (component->GetName() == "NAND") {
        // Add NAND-specific properties
        m_propertyGrid->Append(new wxPropertyCategory("NAND Gate"));
        m_propertyGrid->Append(new wxBoolProperty("Enable Animation", "nand_animation", true));
    }
    else if (component->GetName() == "NOR") {
        // Add NOR-specific properties
        m_propertyGrid->Append(new wxPropertyCategory("NOR Gate"));
        m_propertyGrid->Append(new wxBoolProperty("Enable Animation", "nor_animation", true));
    }
    else if (component->GetName() == "NOT") {
        // Add NOT-specific properties
        m_propertyGrid->Append(new wxPropertyCategory("NOT Gate"));
        m_propertyGrid->Append(new wxBoolProperty("Enable Animation", "not_animation", true));
    }
    else if (component->GetName() == "BUF") {
        // Add Buffer-specific properties
        m_propertyGrid->Append(new wxPropertyCategory("Buffer"));
        m_propertyGrid->Append(new wxBoolProperty("Enable Animation", "buf_animation", true));
    }
}

void PropertiesPanel::ClearProperties()
{
    m_propertyGrid->Clear();
    m_propertyGrid->Append(new wxStringProperty("Info", "info", "Select a component to view properties"));
}

void PropertiesPanel::OnPropertyGridChange(wxPropertyGridEvent& event)
{
    wxString propertyName = event.GetPropertyName();
    wxVariant value = event.GetPropertyValue();
    
    // In a real implementation, we would update the selected component based on property changes
    // For now, we'll just log the change
    wxLogMessage("Property %s changed to %s", propertyName, value.MakeString());
    
    // Example: if position property changes, update the component position
    if (propertyName == "position") {
        // Parse the position string and update the component
        wxString posStr = value.MakeString();
        long x, y;
        wxStringTokenizer tkz(posStr, wxT(", "));
        if (tkz.HasMoreTokens()) {
            tkz.GetNextToken().ToLong(&x);
        }
        if (tkz.HasMoreTokens()) {
            tkz.GetNextToken().ToLong(&y);
        }
        
        // In a real implementation, we would update the selected component's position
        // and potentially issue a move command for undo/redo
    }
}