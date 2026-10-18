#!/bin/bash

# Main build script for ProtoVM
# This script can build the original U++ version, the wxWidgets GUI, or the CLI

# Locate the U++ install. Override with UPP_DIR if needed. Prefer an existing
# install (Dev is often symlinked and paths differ per host), falling back to
# $HOME/Dev/ai-upp.
if [ -z "$UPP_DIR" ]; then
    for candidate in /yhteinen/active/sblo/Dev/ai-upp "$HOME/Dev/ai-upp"; do
        if [ -d "$candidate" ]; then
            UPP_DIR="$candidate"
            break
        fi
    done
fi
UPP_DIR="${UPP_DIR:-$HOME/Dev/ai-upp}"
UPP_SRC="$UPP_DIR/uppsrc"
UPP_BIN="$UPP_DIR/bin"
UMK="$UPP_BIN/umk"
ASSEMBLY="./src,./uppsrc,$UPP_SRC"

echo "ProtoVM Build System"
echo "==================="

ensure_umk() {
    if [ -x "$UMK" ]; then
        return 0
    fi
    local bootstrap="$UPP_DIR/stdsrc/build/bootstrap.sh"
    if [ ! -f "$bootstrap" ]; then
        echo "Error: umk not found at $UMK and no bootstrap script at $bootstrap" >&2
        echo "Set UPP_DIR to your U++ install directory." >&2
        exit 1
    fi
    echo "umk not found at $UMK; bootstrapping U++ toolchain..."
    "$bootstrap" || { echo "Error: U++ bootstrap failed" >&2; exit 1; }
    if [ ! -x "$UMK" ]; then
        echo "Error: bootstrap completed but umk still missing at $UMK" >&2
        exit 1
    fi
}

build_upp() {
    local package="$1"
    ensure_umk
    local clean_flag=""
    shift
    while [ $# -gt 0 ]; do
        case "$1" in
            --clean) clean_flag="-a" ;;
        esac
        shift
    done
    mkdir -p build
    $UMK "$ASSEMBLY" "$package" ~/.config/u++/theide/CLANG.bm -dsH2 +DEBUG_FULL,USEMALLOC $UPP_FLAGS build/"$package" $clean_flag
}

# Main configuration flag passed to umk. GUI applications need +GUI; console
# targets (e.g. the headless self-test) must not set it.
UPP_FLAGS="+GUI"

if [ "$1" = "gui" ] || [ "$1" = "wx" ]; then
    echo "Building ProtoVM GUI (wxWidgets version)..."
    if [ -f "wxsrc/build.sh" ]; then
        cd wxsrc
        ./build.sh "${@:2}"
    else
        echo "Error: wxsrc/build.sh not found"
        exit 1
    fi
elif [ "$1" = "cli" ]; then
    echo "Building ProtoVM CLI (U++ make)..."
    ./build_cli.sh "${@:2}"
elif [ "$1" = "logic-analyzer" ] || [ "$1" = "la" ]; then
    echo "Building Logic Analyzer (U++ version)..."
    shift
    build_upp LogicAnalyzer "$@"
elif [ "$1" = "la-test" ] || [ "$1" = "logic-analyzer-test" ]; then
    echo "Building Logic Analyzer Self-Test (U++ version)..."
    shift
    UPP_FLAGS=""
    build_upp LogicAnalyzerModelTest "$@"
    echo ""
    echo "Running self-test..."
    ./build/LogicAnalyzerModelTest
elif [ "$1" = "original" ] || [ "$1" = "upp" ]; then
    echo "Building original ProtoVM (U++ version)..."
    shift
    build_upp ProtoVM "$@"
else
    echo "Usage: $0 [option]"
    echo "Options:"
    echo "  gui, wx           - Build the wxWidgets GUI version"
    echo "  cli               - Build the CLI executable via U++ make (build_cli.sh)"
    echo "  logic-analyzer, la - Build the Logic Analyzer U++ application"
    echo "  la-test           - Build and run Logic Analyzer self-test"
    echo "  original, upp     - Build the original U++ version (default)"
    echo "  all               - Build both versions"
    echo ""
    echo "For GUI build:       $0 gui [--run]"
    echo "For Logic Analyzer:  $0 logic-analyzer [--clean]"
    echo "For Self-Test:       $0 la-test [--clean]"
    echo "For original:        $0 original [--clean]"
    echo "For both:            $0 all"
    
    if [ $# -eq 0 ]; then
        echo ""
        echo "Defaulting to original U++ build..."
        build_upp ProtoVM
    fi
fi