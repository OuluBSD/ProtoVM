#!/bin/bash

# Main build script for ProtoVM
# This script can build either the original U++ version or the new wxWidgets GUI

echo "ProtoVM Build System"
echo "==================="

if [ "$1" = "gui" ] || [ "$1" = "wx" ]; then
    echo "Building ProtoVM GUI (wxWidgets version)..."
    if [ -f "wxsrc/build.sh" ]; then
        cd wxsrc
        ./build.sh "${@:2}"  # Pass remaining arguments to the wx build script
    else
        echo "Error: wxsrc/build.sh not found"
        exit 1
    fi
elif [ "$1" = "original" ] || [ "$1" = "upp" ]; then
    echo "Building original ProtoVM (U++ version)..."
    ./build.sh "${@:2}"  # Pass remaining arguments to the original build script
else
    echo "Usage: $0 [option]"
    echo "Options:"
    echo "  gui, wx     - Build the wxWidgets GUI version"
    echo "  original, upp - Build the original U++ version (default)"
    echo "  all         - Build both versions"
    echo ""
    echo "For GUI build: $0 gui [--run]"
    echo "For original:  $0 original [--clean]"
    echo "For both:      $0 all"
    
    if [ $# -eq 0 ]; then
        echo ""
        echo "Defaulting to original U++ build..."
        ./build.sh
    fi
fi