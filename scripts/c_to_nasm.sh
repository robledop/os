#!/bin/bash

# Translates a C header to a NASM header
# The C header can only make use of:
#   - #define
#   - #ifndef
#   - #endif

# INPUT: A list of the required NASM headers

# The first argument is the directory where the C headers are located
HEADER_DIR=$1
shift

for NASM_HEADER in "$@"
do
    C_HEADER="${HEADER_DIR}/${NASM_HEADER%%.asm}.h"
    if [ "$C_HEADER" -nt "$NASM_HEADER" ]; then
        sed 's/\/\*/;/' "$C_HEADER" | # change start of block comments
        sed 's/\*\///'            | # change end of block comments
        sed 's/\/\//;/'           | # change start of line comments
        sed 's/^#/%/'             > "${HEADER_DIR}/$NASM_HEADER" # change preprocessor directives and write to NASM header
    fi
done
