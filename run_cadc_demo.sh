#!/bin/bash

# run_cadc_demo.sh - Run the CADC demonstration program
# Similar to run_4004_demo.sh but for the CADC system

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [options]"
    echo "Run the CADC demonstration program"
    echo ""
    echo "Options:"
    echo "  -v, --verbose    Show verbose output"
    echo "  -h, --help       Show this help message"
    exit 0
fi

# Check if we're in the right directory
if [ ! -f "./build/ProtoVM" ]; then
    echo "Error: ProtoVM executable not found in ./build/"
    echo "Please run this script from the ProtoVM project root directory."
    exit 1
fi

echo "Running CADC demonstration..."
echo "============================="

if [ "$1" = "-v" ] || [ "$1" = "--verbose" ]; then
    ./build/ProtoVM cadc -v
else
    ./build/ProtoVM cadc
fi

echo ""
echo "CADC demonstration completed!"