#!/bin/bash

# Build script for ProtoVM GUI application
set -e  # Exit on any error

echo "ProtoVM GUI Build Script"

# Check if wxWidgets is installed
if ! pkg-config --exists wxwidgets; then
    echo "Error: wxWidgets is not installed or not found in pkg-config"
    echo "Please install wxWidgets development libraries:"
    echo "  Ubuntu/Debian: sudo apt-get install libwxgtk3.0-gtk3-dev libwxgtk3.0-0v5"
    echo "  CentOS/RHEL: sudo yum install wxGTK3-devel wxGTK3"
    echo "  macOS: brew install wxwidgets"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure the project with CMake
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo "Building the project..."
make -j$(nproc)

echo "Build completed successfully!"
echo "The executable is located at: build/protovm_gui"

# Optionally run the application
if [ "$1" == "--run" ]; then
    echo "Running ProtoVM GUI..."
    ./protovm_gui
fi