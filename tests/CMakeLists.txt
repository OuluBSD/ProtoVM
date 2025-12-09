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