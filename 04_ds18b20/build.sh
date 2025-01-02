#!/usr/bin/env bash

DIRECTORY="build"

if [[ ! -d "$DIRECTORY" ]]; then
    mkdir "$DIRECTORY"
fi

cd $DIRECTORY || return
cmake ../CMakeLists.txt
make
cp compile_commands.json ../
