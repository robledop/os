#!/usr/bin/env bash

make grub
qemu-system-i386 -S -gdb tcp::1234 -boot d -drive file=disk.img,format=raw -m 64 -display gtk,zoom-to-fit=on  &

gdb rootfs/boot/myos.bin  \
    -ex 'target remote localhost:1234' \
    -ex 'layout src' \
    -ex 'layout regs' \
    -ex 'break kernel_main' \
    -ex 'continue'
