#!/bin/bash

mkdir -p "./rootfs/testdir"

# Generate files
echo "Generating files..."

for (( i = 0; i < 100; i++ )); do
  touch ./rootfs/testdir/"file$i.txt"
done