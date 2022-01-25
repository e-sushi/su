#!/bin/sh

INCLUDES="-I../src -I/mnt/c/src/graphviz/include"
SOURCE="../src/su-main.cpp"
LIBS="-L/mnt/c/src/graphviz/lib -lcgraph -lcdt -lgvc -lstdc++"

COMPILE_FLAGS="-std=gnu++17 -w -lm -fpermissive"
OUTPUT="su.exe"

DEFINES_DEBUG="-D SU_INTERNAL=1 -D SU_SLOW=1 -D _CRT_SECURE_NO_WARNINGS"

OUT_DIR="../build/debug/"

BUILD_ARG=$1


clear
date
gcc $SOURCE $INCLUDES $LIBS $COMPILE_FLAGS $DEFINES_DEBUG -o $OUT_DIR$OUTPUT -g
