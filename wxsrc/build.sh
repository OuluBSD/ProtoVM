#!/bin/bash

# Build script for ProtoVM GUI application
set -e  # Exit on any error

echo "ProtoVM GUI Build Script"

# Check for wxWidgets installation using wx-config
if ! command -v wx-config &> /dev/null; then
    echo "Error: wx-config is not found. wxWidgets is not installed properly."
    echo "Please install wxWidgets development libraries:"
    echo "  Ubuntu/Debian: sudo apt-get install libwxgtk3.2-dev"
    echo "  Or try: sudo apt-get install libwxgtk3.0-gtk3-dev"
    echo "  CentOS/RHEL: sudo yum install wxGTK3-devel"
    echo "  macOS: brew install wxwidgets"
    exit 1
fi

# Get wxWidgets version to confirm installation
WX_VERSION=$(wx-config --version)
if [ -z "$WX_VERSION" ]; then
    echo "Error: wx-config exists but cannot retrieve version information"
    exit 1
fi

echo "Found wxWidgets version: $WX_VERSION"

# Get the project root directory (parent of wxsrc)
PROJECT_ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$PROJECT_ROOT"

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