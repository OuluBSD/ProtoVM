#!/bin/bash

if [ "$1" = "--clean" ]; then
    umk ./src,$HOME/Topside/uppsrc ProtoVM ~/.config/u++/theide/CLANG.bm -vdbsa +DEBUG_FULL build/ProtoVM
else
    umk ./src,$HOME/Topside/uppsrc ProtoVM ~/.config/u++/theide/CLANG.bm -vdbs +DEBUG_FULL build/ProtoVM
fi
