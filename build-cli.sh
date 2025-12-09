#!/bin/bash

if [ "$1" = "--clean" ]; then
    umk ./src,$HOME/Topside/uppsrc ProtoVMCLI ~/.config/u++/theide/CLANG.bm -dsaH2 +DEBUG_FULL,USEMALLOC build/ProtoVMCLI
else
    umk ./src,$HOME/Topside/uppsrc ProtoVMCLI ~/.config/u++/theide/CLANG.bm -dsH2 +DEBUG_FULL,USEMALLOC build/ProtoVMCLI
fi
