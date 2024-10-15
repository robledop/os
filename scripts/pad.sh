#/bin/bash

# Usage: pad.sh <file> <block size>
# Example: pad.sh file 512
# This script will pad the file with zeros to the nearest block size

size=$(stat -c %s $1)
padding=$((512 - (size % $2)))
if [ $padding -eq $2 ]; then padding=0; fi
dd if=/dev/zero bs=1 count=$padding >> $1 