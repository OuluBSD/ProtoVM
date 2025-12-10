#!/bin/bash

# Build script for ProtoVM with analog functionality

echo "Building ProtoVM with analog functionality..."

# Check if we're in the ProtoVM directory
if [ ! -f "build-cli.sh" ]; then
    echo "Error: build-cli.sh not found. Please run this script from the ProtoVM root directory."
    exit 1
fi

# Build the CLI version
echo "Building CLI..."
if [ "$1" = "--clean" ]; then
    echo "Performing clean build..."
    if ./build-cli.sh --clean; then
        echo "CLI build successful!"
    else
        echo "CLI build failed!"
        exit 1
    fi
else
    if ./build-cli.sh; then
        echo "CLI build successful!"
    else
        echo "CLI build failed!"
        exit 1
    fi
fi

echo "Build completed successfully!"