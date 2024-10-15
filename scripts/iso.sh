#!/bin/sh
make grub
grub-mkrescue -o myos.iso ./rootfs
