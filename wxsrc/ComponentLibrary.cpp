#include "ComponentLibrary.h"
#include "CircuitCanvas.h"
#include <algorithm>

ComponentLibrary& ComponentLibrary::GetInstance()
{
    static ComponentLibrary instance;
    return instance;
}

void ComponentLibrary::RegisterComponent(const wxString& name, const wxString& displayName, 
                                       const wxString& category, const wxString& description,
                                       std::function<Component*()> createFunc)
{
    ComponentInfo info;
    info.name = name;
    info.displayName = displayName;
    info.category = category;
    info.description = description;
    info.createFunc = createFunc;
    
    m_components[name] = info;
}

std::vector<ComponentInfo> ComponentLibrary::GetComponents() const
{
    std::vector<ComponentInfo> result;
    for (const auto& pair : m_components) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<ComponentInfo> ComponentLibrary::Search(const wxString& query) const
{
    std::vector<ComponentInfo> result;
    
    if (query.IsEmpty()) {
        return GetComponents();
    }
    
    wxString lowerQuery = query.Lower();
    
    for (const auto& pair : m_components) {
        const ComponentInfo& info = pair.second;
        if (info.name.Lower().Contains(lowerQuery) || 
            info.displayName.Lower().Contains(lowerQuery) ||
            info.category.Lower().Contains(lowerQuery) ||
            info.description.Lower().Contains(lowerQuery)) {
            result.push_back(info);
        }
    }
    
    return result;
}

std::vector<wxString> ComponentLibrary::GetCategories() const
{
    std::vector<wxString> categories;
    std::map<wxString, bool> seen;  // To ensure uniqueness
    
    for (const auto& pair : m_components) {
        const wxString& category = pair.second.category;
        if (seen.find(category) == seen.end()) {
            categories.push_back(category);
            seen[category] = true;
        }
    }
    
    return categories;
}

std::vector<ComponentInfo> ComponentLibrary::GetByCategory(const wxString& category) const
{
    std::vector<ComponentInfo> result;
    
    for (const auto& pair : m_components) {
        if (pair.second.category == category) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

Component* ComponentLibrary::CreateComponent(const wxString& name) const
{
    auto it = m_components.find(name);
    if (it != m_components.end()) {
        return it->second.createFunc();
    }
    return nullptr;
}

// Register default components
static bool RegisterDefaultComponents() {
    ComponentLibrary& lib = ComponentLibrary::GetInstance();
    
    lib.RegisterComponent("NAND", "NAND Gate", "Logic Gates", "NAND logic gate with 2 inputs",
                         []() { return new NANDGateComponent(0, 0); });
    
    lib.RegisterComponent("NOR", "NOR Gate", "Logic Gates", "NOR logic gate with 2 inputs",
                         []() { return new NORGateComponent(0, 0); });
    
    lib.RegisterComponent("NOT", "NOT Gate", "Logic Gates", "NOT logic gate (inverter)",
                         []() { return new NOTGateComponent(0, 0); });
    
    lib.RegisterComponent("BUF", "Buffer", "Logic Gates", "Non-inverting buffer",
                         []() { return new BufferComponent(0, 0); });
    
    lib.RegisterComponent("AND", "AND Gate", "Logic Gates", "AND logic gate with 2 inputs",
                         []() { 
                             // Create a custom AND gate component
                             BufferComponent* buffer = new BufferComponent(0, 0);
                             buffer->SetName("AND");
                             return buffer; 
                         });
    
    return true;
}

// Static initialization
static bool registered = RegisterDefaultComponents();