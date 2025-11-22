# CMakeLists.txt for ProtoVM GUI Tests
cmake_minimum_required(VERSION 3.10)
project(ProtoVMGUITests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)

# Include wxWidgets headers
include(${wxWidgets_USE_FILE})

# We can't use wxWidgets-dependent GUI widgets in unit tests directly,
# so we'll create tests that verify the underlying logic only
# For this, we need to compile some of the core logic without wxWidgets UI dependencies

# Define test sources
set(GUI_TEST_SOURCES
    logic_test.cpp
    simulation_test.cpp
    advanced_logic_test.cpp
    canvas_component_test.cpp
    project_test.cpp
    selection_test.cpp
    undo_redo_test.cpp
    properties_test.cpp
)

# Create executable for GUI tests
add_executable(gui_tests ${GUI_TEST_SOURCES})

# Link against required libraries
target_link_libraries(gui_tests ${wxWidgets_LIBRARIES})
target_include_directories(gui_tests PRIVATE 
    ${PROJECT_SOURCE_DIR}/../../src/ProtoVM
    ${wxWidgets_INCLUDE_DIRS}
)

# We may need to define a headless mode for testing
set_target_properties(gui_tests PROPERTIES
    COMPILE_FLAGS "-DHEADLESS_TEST"
)