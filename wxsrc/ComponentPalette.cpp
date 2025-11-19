#include "ComponentPalette.h"
#include "ComponentLibrary.h"
#include "CircuitCanvas.h"
#include <wx/imaglist.h>

wxBEGIN_EVENT_TABLE(ComponentPalette, wxPanel)
    EVT_TEXT(wxID_ANY, ComponentPalette::OnSearchText)
    EVT_CHOICE(wxID_ANY, ComponentPalette::OnCategorySelect)
    EVT_LIST_ITEM_ACTIVATED(wxID_ANY, ComponentPalette::OnComponentSelect)
wxEND_EVENT_TABLE()

ComponentPalette::ComponentPalette(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    CreateControls();
    PopulateComponents();
}

void ComponentPalette::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Search control
    m_searchCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_searchCtrl->SetHint("Search components...");
    mainSizer->Add(m_searchCtrl, 0, wxEXPAND | wxALL, 5);
    
    // Category selector
    m_categoryChoice = new wxChoice(this, wxID_ANY);
    m_categoryChoice->Append("All Components");
    mainSizer->Add(m_categoryChoice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Component list
    m_componentList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                                     wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_SORT_ASCENDING);
    m_componentList->AppendColumn("Component", wxLIST_FORMAT_LEFT, 150);
    m_componentList->AppendColumn("Category", wxLIST_FORMAT_LEFT, 100);
    mainSizer->Add(m_componentList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Add button (for adding component to canvas)
    m_addButton = new wxButton(this, wxID_ANY, "Add to Canvas");
    mainSizer->Add(m_addButton, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    SetSizer(mainSizer);
}

void ComponentPalette::PopulateComponents()
{
    // Clear the category choice and populate with categories
    m_categoryChoice->Clear();
    m_categoryChoice->Append("All Components");
    
    ComponentLibrary& lib = ComponentLibrary::GetInstance();
    std::vector<wxString> categories = lib.GetCategories();
    
    for (const wxString& category : categories) {
        m_categoryChoice->Append(category);
    }
    
    m_categoryChoice->SetSelection(0);  // Select "All Components"
    
    // Clear and populate the component list
    m_componentList->DeleteAllItems();
    
    std::vector<ComponentInfo> components = lib.GetComponents();
    for (size_t i = 0; i < components.size(); ++i) {
        long index = m_componentList->InsertItem(i, components[i].displayName);
        m_componentList->SetItem(index, 1, components[i].category);
        // Store the component name as client data for later retrieval
        m_componentList->SetItemPtrData(index, reinterpret_cast<wxUIntPtr>(new wxString(components[i].name)));
    }
}

void ComponentPalette::OnSearchText(wxCommandEvent& event)
{
    wxString query = m_searchCtrl->GetValue();
    
    ComponentLibrary& lib = ComponentLibrary::GetInstance();
    std::vector<ComponentInfo> components = lib.Search(query);
    
    // Clear and repopulate the component list based on search results
    m_componentList->DeleteAllItems();
    
    for (size_t i = 0; i < components.size(); ++i) {
        long index = m_componentList->InsertItem(i, components[i].displayName);
        m_componentList->SetItem(index, 1, components[i].category);
        // Store the component name as client data for later retrieval
        m_componentList->SetItemPtrData(index, reinterpret_cast<wxUIntPtr>(new wxString(components[i].name)));
    }
}

void ComponentPalette::OnCategorySelect(wxCommandEvent& event)
{
    wxString selectedCategory = m_categoryChoice->GetStringSelection();
    
    ComponentLibrary& lib = ComponentLibrary::GetInstance();
    std::vector<ComponentInfo> components;
    
    if (selectedCategory == "All Components") {
        components = lib.GetComponents();
    } else {
        components = lib.GetByCategory(selectedCategory);
    }
    
    // Clear and repopulate the component list based on selected category
    m_componentList->DeleteAllItems();
    
    for (size_t i = 0; i < components.size(); ++i) {
        long index = m_componentList->InsertItem(i, components[i].displayName);
        m_componentList->SetItem(index, 1, components[i].category);
        // Store the component name as client data for later retrieval
        m_componentList->SetItemPtrData(index, reinterpret_cast<wxUIntPtr>(new wxString(components[i].name)));
    }
}

void ComponentPalette::OnComponentSelect(wxListEvent& event)
{
    long selectedIndex = event.GetIndex();
    if (selectedIndex == -1) return;
    
    wxString* componentNamePtr = reinterpret_cast<wxString*>(m_componentList->GetItemData(selectedIndex));
    if (componentNamePtr) {
        // In a real implementation, we would add the component to the canvas
        // For now, we'll just log the selection
        wxLogMessage("Selected component: %s", *componentNamePtr);
    }
}

