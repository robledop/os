#!/bin/sh
set -e

mkdir -p bin/isodir
mkdir -p bin/isodir/boot
mkdir -p bin/isodir/boot/grub

cp ./bin/myos.bin bin/isodir/boot/myos.kernel
cat > bin/isodir/boot/grub/grub.cfg << EOF
menuentry "myos" {
	multiboot /boot/myos.kernel
}
set timeout=0
EOF
grub-mkrescue -o bin/myos.iso bin/isodir
