#!/bin/bash

if [ "$1" = "--clean" ]; then
    umk ./src,$HOME/Topside/uppsrc ProtoVM ~/.config/u++/theide/CLANG.bm -dsaH2 +DEBUG_FULL build/ProtoVM
else
    umk ./src,$HOME/Topside/uppsrc ProtoVM ~/.config/u++/theide/CLANG.bm -dsH2 +DEBUG_FULL build/ProtoVM
fi
