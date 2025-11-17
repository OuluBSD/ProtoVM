#ifndef PROPERTIESPANEL_H
#define PROPERTIESPANEL_H

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

// Forward declarations
class Component;

class PropertiesPanel : public wxPanel
{
public:
    PropertiesPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    
    // Update the properties panel based on the selected component
    void UpdateProperties(Component* component);
    
    // Clear the properties panel
    void ClearProperties();

private:
    wxPropertyGrid* m_propertyGrid;
    
    // Event handlers
    void OnPropertyGridChange(wxPropertyGridEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};

#endif // PROPERTIESPANEL_H