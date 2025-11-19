#ifndef COMPONENTPALETTE_H
#define COMPONENTPALETTE_H

#include <wx/wx.h>
#include <wx/clrpicker.h>
#include <wx/listctrl.h>
#include <wx/dnd.h>
#include <vector>

class ComponentPalette : public wxPanel
{
public:
    ComponentPalette(wxWindow* parent, wxWindowID id = wxID_ANY);
    
private:
    void CreateControls();
    void PopulateComponents();
    void OnSearchText(wxCommandEvent& event);
    void OnCategorySelect(wxCommandEvent& event);
    void OnComponentSelect(wxListEvent& event);
    void OnDragInit(wxListEvent& event);
    
    wxTextCtrl* m_searchCtrl;
    wxChoice* m_categoryChoice;
    wxListCtrl* m_componentList;
    wxButton* m_addButton;
    
    wxDECLARE_EVENT_TABLE();
};

#endif // COMPONENTPALETTE_H