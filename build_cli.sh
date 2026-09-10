#!/usr/bin/env bash
set -euo pipefail

# Wrapper to invoke the U++ CLI build; delegates to the existing hyphenated script.
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_SCRIPT="${SCRIPT_DIR}/build-cli.sh"

if [ ! -x "$TARGET_SCRIPT" ]; then
    echo "Error: $TARGET_SCRIPT is missing or not executable." >&2
    exit 1
fi

exec "$TARGET_SCRIPT" "$@"
