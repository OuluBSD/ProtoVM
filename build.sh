#!/bin/bash

if [ "$1" = "--clean" ]; then
    umk ./src,$HOME/Dev/ai-upp/uppsrc ProtoVM ~/.config/u++/theide/CLANG.bm -dsaH2 +DEBUG_FULL,USEMALLOC build/ProtoVM
else
    umk ./src,$HOME/Dev/ai-upp/uppsrc ProtoVM ~/.config/u++/theide/CLANG.bm -dsH2 +DEBUG_FULL,USEMALLOC build/ProtoVM
fi
