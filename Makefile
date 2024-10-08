$(shell mkdir -p ./bin)
SRC_DIRS := $(shell find ./src -type d ! -path './src/include' ! -path './src/boot')
BUILD_DIRS := $(patsubst ./src/%,./build/%,$(SRC_DIRS))
$(shell mkdir -p $(BUILD_DIRS))
ASM_FILES := $(wildcard $(addsuffix /*.asm, $(SRC_DIRS)))
C_FILES := $(wildcard $(addsuffix /*.c, $(SRC_DIRS)))
ASM_OBJS := $(ASM_FILES:./src/%.asm=./build/%.asm.o)
C_OBJS := $(C_FILES:./src/%.c=./build/%.o)
FILES := $(ASM_OBJS) $(C_OBJS)
INCLUDES = -I ./src/include
STAGE2_FLAGS = -g \
	-ffreestanding \
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
	-Wextra \
	-std=gnu23 \
	-pedantic \
	-Wall

FLAGS = -g \
	-ffreestanding \
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
	-Wextra \
	-std=gnu23 \
	-pedantic \
	-fstack-protector \
	-fsanitize=undefined \
	-Wall

	# -pedantic-errors \
	# -fstack-protector \
	# -fsanitize=undefined \

ifeq ($(filter grub,$(MAKECMDGOALS)),grub)
    FLAGS += -DGRUB
endif
ifeq ($(filter qemu_grub,$(MAKECMDGOALS)),qemu_grub)
    FLAGS += -DGRUB
endif


all: ./bin/boot.bin ./bin/kernel.bin apps
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/stage2.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp -r ./rootfs/. /mnt/d/
	sudo umount /mnt/d
# stat --format=%n:%s ./kernel.bin

./bin/kernel.bin: $(filter-out ./build/src/grub/%, $(FILES))
	i686-elf-ld -g -relocatable $(filter-out ./build/grub/%, $(FILES)) -o ./build/kernelfull.o
	i686-elf-gcc $(FLAGS) -T ./src/linker.ld -o ./bin/kernel.bin ./build/kernelfull.o
	./pad.sh ./bin/kernel.bin 512

./bin/boot.bin: ./src/boot/boot.asm 
	$(shell mkdir -p ./build/boot)
	nasm -f bin -g ./src/boot/boot.asm -o ./bin/boot.bin
	nasm -f elf -g ./src/boot/stage2.asm -o ./build/boot/stage2.asm.o
	i686-elf-gcc -g $(STAGE2_FLAGS) -c ./src/boot/stage2.c -o ./build/boot/stage2.o
	i686-elf-ld -g -relocatable ./build/boot/stage2.asm.o ./build/boot/stage2.o -o ./build/stage2full.o
	i686-elf-gcc $(STAGE2_FLAGS) -T ./src/boot/linker.ld -o ./bin/stage2.bin ./build/stage2full.o

	./pad.sh ./bin/stage2.bin 512

./build/%.asm.o: ./src/%.asm
	nasm -f elf -g $< -o $@

./build/%.o: ./src/%.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -c $< -o $@

grub: ./bin/kernel-grub.bin apps
	grub-file --is-x86-multiboot ./rootfs/boot/myos.kernel
	rm -rf ./disk.img
	dd if=/dev/zero of=./disk.img bs=512 count=131072
	mkfs.vfat -c -F 16 -S 512 ./disk.img
	sudo mount -t vfat ./disk.img /mnt/d
	sudo grub-install --root-directory=/mnt/d --force --no-floppy --modules="normal part_msdos multiboot" /dev/loop1
	sudo cp -r ./rootfs/. /mnt/d/
	sudo umount -q /mnt/d
	# VBoxManage convertdd ./disk.img ./disk.vdi

./bin/kernel-grub.bin: $(filter-out ./build/kernel/%.asm.o, $(FILES))
	i686-elf-ld -g -relocatable $(filter-out ./build/kernel/%.asm.o, $(FILES)) -o ./build/kernelfull.o 
	i686-elf-gcc $(FLAGS) -T ./src/grub/linker.ld -o ./rootfs/boot/myos.kernel ./build/kernelfull.o

qemu: all
	# qemu-system-i386 -boot d -hda ./bin/os.bin -m 512 -serial stdio -display gtk,zoom-to-fit=on
	qemu-system-i386 -S -gdb tcp::1234 -boot d -hda ./bin/os.bin -m 512 -daemonize -serial file:serial.log -display gtk,zoom-to-fit=on

qemu_grub: grub 
	# qemu-system-i386 -hda ./disk.img -m 512 -serial stdio -display gtk,zoom-to-fit=on
	qemu-system-i386 -S -gdb tcp::1234 -boot d -hda ./disk.img -m 512 -daemonize -serial file:serial.log -display gtk,zoom-to-fit=on

apps:
	cd ./user && $(MAKE) all

apps_clean:
	cd ./user && $(MAKE) clean

clean: apps_clean
	rm -rf ./bin ./build ./mnt ./disk.img ./disk.vdi ./rootfs/boot/myos.kernel
