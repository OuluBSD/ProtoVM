#!/bin/bash

# Build script for ProtoVM GUI application
set -e  # Exit on any error

echo "ProtoVM GUI Build Script"

# Check for wxWidgets installation using different possible package names
WX_CONFIG=""
for pkg in wxwidgets wxWidgets-3.2 wxWidgets-3.0 wxgtk3.2 wxgtk3.0; do
    if pkg-config --exists "$pkg"; then
        WX_CONFIG="$pkg"
        break
    fi
done

if [ -z "$WX_CONFIG" ]; then
    echo "Error: wxWidgets is not installed or not found in pkg-config"
    echo "Please install wxWidgets development libraries:"
    echo "  Ubuntu/Debian: sudo apt-get install libwxgtk3.2-dev"
    echo "  Or try: sudo apt-get install libwxgtk3.0-gtk3-dev"
    echo "  CentOS/RHEL: sudo yum install wxGTK3-devel"
    echo "  macOS: brew install wxwidgets"
    exit 1
fi

echo "Found wxWidgets package: $WX_CONFIG"

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