#!/bin/bash

# run_cadc_program.sh - Run a CADC program with specified parameters
# Similar to run_4004_program.sh but for the CADC system

PROGRAM_FILE=""
LOAD_ADDR="0x00"
TICKS=1000
VERBOSE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--program)
            PROGRAM_FILE="$2"
            shift 2
            ;;
        -a|--address)
            LOAD_ADDR="$2"
            shift 2
            ;;
        -t|--ticks)
            TICKS="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo "Run a CADC program"
            echo ""
            echo "Options:"
            echo "  -p, --program FILE    CADC program file to load"
            echo "  -a, --address ADDR    Load address (default: 0x00)"
            echo "  -t, --ticks NUM       Number of ticks to run (default: 1000)"
            echo "  -v, --verbose         Show verbose output"
            echo "  -h, --help            Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Check if we're in the right directory
if [ ! -f "./build/ProtoVM" ]; then
    echo "Error: ProtoVM executable not found in ./build/"
    echo "Please run this script from the ProtoVM project root directory."
    exit 1
fi

# Check if program file exists
if [ -n "$PROGRAM_FILE" ] && [ ! -f "$PROGRAM_FILE" ]; then
    echo "Error: Program file '$PROGRAM_FILE' not found"
    exit 1
fi

echo "Running CADC program..."
echo "======================="

if [ -n "$PROGRAM_FILE" ]; then
    if [ "$VERBOSE" = true ]; then
        ./build/ProtoVM --load-binary "$PROGRAM_FILE" "$LOAD_ADDR" -t "$TICKS" -v
    else
        ./build/ProtoVM --load-binary "$PROGRAM_FILE" "$LOAD_ADDR" -t "$TICKS"
    fi
else
    if [ "$VERBOSE" = true ]; then
        ./build/ProtoVM cadc -t "$TICKS" -v
    else
        ./build/ProtoVM cadc -t "$TICKS"
    fi
fi

echo ""
echo "CADC program execution completed!"