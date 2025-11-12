#!/bin/bash

# Script to run the 4004 binary creation utility from the project root
# Change to the script's directory parent (project root) to run properly
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "Running 4004 binary creation utility..."
echo "Usage: ./bin/create_4004_binary [options]"
echo ""

# Run the binary creation utility
./bin/create_4004_binary "$@"

exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo "4004 binary creation completed successfully."
else
    echo "4004 binary creation failed with exit code $exit_code."
fi

exit $exit_code