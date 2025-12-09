cmake_minimum_required(VERSION 3.10)
project(ProtoVMTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create the diff engine test
add_executable(diff_engine_test diff_engine_test.cpp)
target_include_directories(diff_engine_test PRIVATE
    ../src/ProtoVMCLI
    ../src/ProtoVM
)

# Create the scheduling test
add_executable(scheduling_test unit/scheduling_test.cpp)
target_include_directories(scheduling_test PRIVATE
    ../src/ProtoVMCLI
    ../src/ProtoVM
)

# Create the CDC analysis test
add_executable(cdc_analysis_test unit/cdc_analysis_test.cpp)
target_include_directories(cdc_analysis_test PRIVATE
    ../src/ProtoVMCLI
    ../src/ProtoVM
)