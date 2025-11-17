#ifndef COMPONENTLIBRARY_H
#define COMPONENTLIBRARY_H

#include <wx/wx.h>
#include <vector>
#include <map>
#include <functional>
#include <memory>

// Forward declarations
class Component;

struct ComponentInfo {
    wxString name;
    wxString displayName;
    wxString category;
    wxString description;
    std::function<Component*()> createFunc;
};

class ComponentLibrary {
public:
    static ComponentLibrary& GetInstance();
    
    void RegisterComponent(const wxString& name, const wxString& displayName, 
                          const wxString& category, const wxString& description,
                          std::function<Component*()> createFunc);
    
    std::vector<ComponentInfo> GetComponents() const;
    std::vector<ComponentInfo> Search(const wxString& query) const;
    std::vector<wxString> GetCategories() const;
    std::vector<ComponentInfo> GetByCategory(const wxString& category) const;
    
    Component* CreateComponent(const wxString& name) const;

private:
    ComponentLibrary() = default;  // Private constructor for singleton
    std::map<wxString, ComponentInfo> m_components;
};

#endif // COMPONENTLIBRARY_H