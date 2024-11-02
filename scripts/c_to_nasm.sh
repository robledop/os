#!/bin/bash

# Translates a C header to a NASM header
# The C header can only make use of:
#   - #define
#   - #ifndef
#   - #endif
# It replaces #pragma once with #ifndef, #define and #endif, but it may not cover all edge cases

# INPUT: A list of the required NASM headers

# The first argument is the directory where the C headers are located
HEADER_DIR=$1
shift # discard the first argument that was already stored in HEADER_DIR so that the remaining arguments are the NASM headers
# and can be iterated over

for NASM_HEADER in "$@"
do
    C_HEADER="${HEADER_DIR}/${NASM_HEADER%%.asm}.h"
    if [ "$C_HEADER" -nt "$NASM_HEADER" ]; then
        sed 's/\/\*/;/' "$C_HEADER" | # change start of block comments
        sed 's/\*\///'            | # change end of block comments
        sed 's/\/\//;/'           | # change start of line comments
        sed "s/'//g"               | # remove single quotes
        # replace #pragma once with #ifndef, #define
        sed 's/^#pragma once/#ifndef '"_${NASM_HEADER%%.asm}"'\n#define '"_${NASM_HEADER%%.asm}"'\n/' |
        sed 's/^#/%/'             > "${HEADER_DIR}/$NASM_HEADER" # change preprocessor directives and write to NASM header
    fi
done

# Add %endif at the end of the file if it is not present
sed -i -e '$!b' -e '/%endif$/b' -e '$a%endif' "${HEADER_DIR}/$NASM_HEADER"