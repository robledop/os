#!/bin/bash

rm -rf ./disk.img
dd if=/dev/zero of=./disk.img bs=512 count=65536
mkfs.vfat -c -F 16 ./disk.img
sudo mount -t vfat ./disk.img /mnt/d
ld=$(losetup -j ./disk.img | grep -oP '/dev/loop[0-9]+') # Find a free loop device
sudo grub-install --root-directory=/mnt/d --force --no-floppy --modules="normal part_msdos multiboot" "$ld"
./scripts/generate-files.sh
sudo cp -r ./rootfs/. /mnt/d/
sudo umount -q /mnt/d