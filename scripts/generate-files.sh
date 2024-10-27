#!/bin/bash

mkdir -p "./rootfs/testdir"
echo "Generating test files..."

for (( i = 0; i < 100; i++ )); do
  touch ./rootfs/testdir/"file$i.txt"
done