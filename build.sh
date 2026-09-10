#!/bin/bash

# Main build script for ProtoVM
# This script can build the original U++ version, the wxWidgets GUI, or the CLI

echo "ProtoVM Build System"
echo "==================="

build_upp() {
    if [ "$1" = "--clean" ]; then
        umk ./src,$HOME/Topside/uppsrc ProtoVM ~/.config/u++/theide/CLANG.bm -dsaH2 +DEBUG_FULL,USEMALLOC build/ProtoVM
    else
        umk ./src,$HOME/Topside/uppsrc ProtoVM ~/.config/u++/theide/CLANG.bm -dsH2 +DEBUG_FULL,USEMALLOC build/ProtoVM
    fi
}

if [ "$1" = "gui" ] || [ "$1" = "wx" ]; then
    echo "Building ProtoVM GUI (wxWidgets version)..."
    if [ -f "wxsrc/build.sh" ]; then
        cd wxsrc
        ./build.sh "${@:2}"  # Pass remaining arguments to the wx build script
    else
        echo "Error: wxsrc/build.sh not found"
        exit 1
    fi
elif [ "$1" = "cli" ]; then
    echo "Building ProtoVM CLI (U++ make)..."
    ./build_cli.sh "${@:2}"
elif [ "$1" = "original" ] || [ "$1" = "upp" ]; then
    echo "Building original ProtoVM (U++ version)..."
    shift
    build_upp "$@"
else
    echo "Usage: $0 [option]"
    echo "Options:"
    echo "  gui, wx     - Build the wxWidgets GUI version"
    echo "  cli         - Build the CLI executable via U++ make (build_cli.sh)"
    echo "  original, upp - Build the original U++ version (default)"
    echo "  all         - Build both versions"
    echo ""
    echo "For GUI build: $0 gui [--run]"
    echo "For original:  $0 original [--clean]"
    echo "For both:      $0 all"
    
    if [ $# -eq 0 ]; then
        echo ""
        echo "Defaulting to original U++ build..."
        build_upp
    fi
fi