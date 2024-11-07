$(shell mkdir -p ./bin)
$(shell mkdir -p ./rootfs/bin)
CC=i686-elf-gcc
AS=nasm
LD=i686-elf-ld
SRC_DIRS := $(shell find ./kernel -type d ! -path './kernel/boot')
BUILD_DIRS := $(patsubst ./kernel/%,./build/%,$(SRC_DIRS))
$(shell mkdir -p $(BUILD_DIRS))
ASM_FILES := $(wildcard $(addsuffix /*.asm, $(SRC_DIRS)))
C_FILES := $(wildcard $(addsuffix /*.c, $(SRC_DIRS)))
ASM_OBJS := $(ASM_FILES:./kernel/%.asm=./build/%.asm.o)
C_OBJS := $(C_FILES:./kernel/%.c=./build/%.o)
FILES := $(ASM_OBJS) $(C_OBJS)
INCLUDES = -I ./include
AS_INCLUDES = -I ./include
AS_HEADERS = config.asm
DEBUG_FLAGS = -g
STAGE2_FLAGS = -ffreestanding \
	-O0 \
	-nostdlib \
	-falign-jumps \
	-falign-functions \
	-falign-labels \
	-falign-loops \
	-fstrength-reduce \
	-fno-omit-frame-pointer \
	-Wno-unused-function \
	-Wno-unused-variable \
	-fno-inline-functions \
	-fno-builtin \
	-Werror \
	-Wno-unused-label \
	-Wno-cpp \
	-Wno-unused-parameter \
	-nostartfiles \
	-nodefaultlibs \
	-save-temps \
	-Wextra \
	-std=gnu23 \
	-pedantic \
	-Wall

FLAGS = -ffreestanding \
	-O0 \
	-nostdlib \
	-falign-jumps \
	-falign-functions \
	-falign-labels \
	-falign-loops \
	-fstrength-reduce \
	-fno-omit-frame-pointer \
	-Wno-unused-function \
	-Wno-unused-variable \
	-fno-inline-functions \
	-fno-builtin \
	-Wno-unused-label \
	-Wno-cpp \
	-Wno-unused-parameter \
	-nostartfiles \
	-nodefaultlibs \
	-save-temps \
	-std=gnu23 \
	-fstack-protector \
	-fsanitize=undefined \

#	-Werror \
#	-Wextra \
#	-Wall
#	-pedantic \
#	-masm=intel \
	# -pedantic-errors \
	# -fstack-protector \
	# -fsanitize=undefined \

FLAGS += -D__KERNEL__

ifeq ($(filter grub,$(MAKECMDGOALS)),grub)
    FLAGS += -DGRUB
endif
ifeq ($(filter qemu_grub_debug,$(MAKECMDGOALS)),qemu_grub_debug)
    FLAGS += -DGRUB
endif

.PHONY: all
# Build that uses my own 2-stage bootloader
all: ./bin/boot.bin ./bin/kernel.bin apps FORCE
	./scripts/c_to_nasm.sh ./include $(AS_HEADERS)
	rm -rf ./bin/disk.img
	dd if=/dev/zero of=./bin/disk.img bs=512 count=65536
	mkfs.vfat -R 512 -c -F 16 -S 512 ./bin/disk.img
	dd if=./bin/boot.bin of=./bin/disk.img bs=512 count=1 seek=0 conv=notrunc
	dd if=./bin/stage2.bin of=./bin/disk.img bs=512 count=1 seek=1 conv=notrunc
	dd if=./bin/kernel.bin of=./bin/disk.img bs=512 count=510 seek=2 conv=notrunc
	sudo mount -t vfat ./bin/disk.img /mnt/d
	./scripts/generate-files.sh
	sudo cp -r ./rootfs/. /mnt/d/
	sudo umount /mnt/d

# stat --format=%n:%s ./kernel.bin

# This is used by the build with my own 2-stage bootloader. GRUB is filtered out.
./bin/kernel.bin: $(filter-out ./build/grub/%, $(FILES)) FORCE
	$(LD) $(DEBUG_FLAGS) -relocatable $(filter-out ./build/grub/%, $(FILES)) -o ./build/kernelfull.o
	$(CC) $(FLAGS) $(DEBUG_FLAGS) -T ./kernel/linker.ld -o ./bin/kernel.bin ./build/kernelfull.o
	./scripts/pad.sh ./bin/kernel.bin 512

# My bootloader
./bin/boot.bin: ./kernel/boot/boot.asm FORCE
	$(shell mkdir -p ./build/boot)
	$(AS) $(AS_INCLUDES) -f bin $(DEBUG_FLAGS) ./kernel/boot/boot.asm -o ./bin/boot.bin
	$(AS) $(AS_INCLUDES) -f elf $(DEBUG_FLAGS) ./kernel/boot/stage2.asm -o ./build/boot/stage2.asm.o
	$(CC) $(STAGE2_FLAGS) $(DEBUG_FLAGS) -I./include -c ./kernel/boot/stage2.c -o ./build/boot/stage2.o
	$(LD) $(DEBUG_FLAGS) -relocatable ./build/boot/stage2.asm.o ./build/boot/stage2.o -o ./build/stage2full.o
	$(CC) $(STAGE2_FLAGS) $(DEBUG_FLAGS) -T ./kernel/boot/linker.ld -o ./bin/stage2.bin ./build/stage2full.o
	./scripts/pad.sh ./bin/stage2.bin 512

./build/%.asm.o: ./kernel/%.asm .asm_headers FORCE
	$(AS) $(AS_INCLUDES) -f elf $(DEBUG_FLAGS) $< -o $@

.PHONY: .asm_headers
.asm_headers: FORCE
	./scripts/c_to_nasm.sh ./include $(AS_HEADERS)

./build/%.o: ./kernel/%.c FORCE
	$(CC) $(INCLUDES) $(FLAGS) $(DEBUG_FLAGS) -c $< -o $@

.PHONY: grub
grub: ./bin/kernel-grub.bin apps ./bin/boot.bin FORCE
	grub-file --is-x86-multiboot ./rootfs/boot/myos.bin
	./scripts/create-grub-image.sh
	# VBoxManage convertdd ./disk.img ./disk.vdi

# The GRUB build does not include kernel.asm. It also does not include anything in the boot directory,
# but that is already filtered in SRC_DIRS
./bin/kernel-grub.bin: $(filter-out ./build/main/%.asm.o, $(FILES)) FORCE
	$(LD) $(DEBUG_FLAGS) -relocatable $(filter-out ./build/main/%.asm.o, $(FILES)) -o ./build/kernelfull.o
	$(CC) $(FLAGS) $(DEBUG_FLAGS) -T ./kernel/grub/linker.ld -o ./rootfs/boot/myos.bin ./build/kernelfull.o

# Generates an .iso file. The kernel does not yet have a CD-ROM driver.
.PHONY: iso
iso: grub FORCE
	rm -rf ./myos.iso
	grub-mkrescue -o ./myos.iso ./rootfs

.PHONY: qemu_debug
qemu_debug: all FORCE
	qemu-system-i386 -S -gdb tcp::1234 -boot d -hda ./bin/disk.img -m 512 -daemonize -serial file:serial.log -d int -D qemu.log

.PHONY: qemu
qemu: all FORCE
	qemu-system-i386 -boot d -hda ./bin/disk.img -m 512 -serial stdio

.PHONY: qemu_grub_debug
qemu_grub_debug: grub FORCE
	#./scripts/create_tap.sh
	qemu-system-i386 -S -gdb tcp::1234 -boot d -hda ./disk.img -m 512 -daemonize -serial file:serial.log -display gtk,zoom-to-fit=on -d int -D qemu.log -netdev tap,id=net0,ifname=tap0,script=no,downscript=no -device e1000,netdev=net0 -object filter-dump,id=f1,netdev=net0,file=dump.dat

.PHONY: qemu_grub
qemu_grub: grub FORCE
	qemu-system-i386 -boot d -hda ./disk.img -m 512 -serial stdio -display gtk,zoom-to-fit=on

.PHONY: qemu_iso
qemu_iso: iso FORCE
	qemu-system-i386 -boot d -cdrom ./myos.iso -m 512 -daemonize -serial file:serial.log -display gtk,zoom-to-fit=on -d int -D qemu.log

.PHONY: apps
apps: FORCE
	cd ./user && $(MAKE) all

.PHONY: apps_clean
apps_clean:
	cd ./user && $(MAKE) clean

.PHONY: clean
clean: apps_clean
	echo "Cleaning..."
	rm -rf ./bin ./build ./mnt ./disk.img ./disk1.vdi ./rootfs/testdir ./rootfs/boot/myos.bin ./myos.iso ./serial.log ./qemu.log ./bochslog.txt ./include/config.asm

.PHONY: test
test: grub
	# TODO: Add test cases
	qemu-system-i386 -boot d -hda ./disk.img -m 128 -serial stdio -display none -d int -D qemu.log

# Force rebuild of all files
.PHONY: FORCE
FORCE: