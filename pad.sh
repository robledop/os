#/bin/bash

size=$(stat -c %s $1)
padding=$((512 - (size % 512)))
if [ $padding -eq 512 ]; then padding=0; fi
dd if=/dev/zero bs=1 count=$padding >> $1 