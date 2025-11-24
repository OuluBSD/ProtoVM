cmake_minimum_required(VERSION 3.10)
project(ProtoVM_GUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use wx-config to get compiler and linker flags
find_program(WX_CONFIG_PROGRAM NAMES wx-config)
if(NOT WX_CONFIG_PROGRAM)
    message(FATAL_ERROR "wx-config not found. Please install wxWidgets development libraries.")
endif()

# Execute wx-config to get flags
execute_process(
    COMMAND ${WX_CONFIG_PROGRAM} --cxxflags
    OUTPUT_VARIABLE WX_CXX_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${WX_CONFIG_PROGRAM} --libs core,base,propgrid,adv
    OUTPUT_VARIABLE WX_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add the flags to the compilation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WX_CXX_FLAGS}")

# Add all source files (GUI components only)
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
    wxsrc/SimulationBridge.cpp
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
    wxsrc/SimulationBridge.h
)

# Create executable
add_executable(protovm_gui ${SOURCES} ${HEADERS})

# Add include directories
target_include_directories(protovm_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/wxsrc
)

# Separate the libraries for target_link_libraries
separate_arguments(WX_LIBS_LIST NATIVE_COMMAND "${WX_LIBS}")

# Link the libraries
target_link_libraries(protovm_gui ${WX_LIBS_LIST})