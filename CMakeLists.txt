cmake_minimum_required(VERSION 3.10)
project(ProtoVM_GUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets package
find_package(wxWidgets REQUIRED COMPONENTS core base)

# Include wxWidgets use file
include(${wxWidgets_USE_FILE})

# Add all source files
set(SOURCES
    wxsrc/main.cpp
    wxsrc/ProtoVMApp.cpp
    wxsrc/MainFrame.cpp
    wxsrc/CircuitCanvas.cpp
    wxsrc/CircuitSerializer.cpp
    wxsrc/UndoRedo.cpp
    wxsrc/PropertiesPanel.cpp
    wxsrc/ComponentLibrary.cpp
    wxsrc/ComponentPalette.cpp
    wxsrc/SimulationController.cpp
)

set(HEADERS
    wxsrc/ProtoVMApp.h
    wxsrc/MainFrame.h
    wxsrc/CircuitCanvas.h
    wxsrc/CircuitData.h
    wxsrc/CircuitSerializer.h
    wxsrc/UndoRedo.h
    wxsrc/PropertiesPanel.h
    wxsrc/ComponentLibrary.h
    wxsrc/ComponentPalette.h
    wxsrc/SimulationController.h
)

# Create executable
add_executable(protovm_gui ${SOURCES} ${HEADERS})

# Find and include property grid library
find_package(wxWidgets COMPONENTS core base propgrid adv REQUIRED)

# Link wxWidgets libraries
target_link_libraries(protovm_gui ${wxWidgets_LIBRARIES})

# Include directories
target_include_directories(protovm_gui PRIVATE ${wxWidgets_INCLUDE_DIRS})