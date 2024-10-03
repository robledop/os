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
FLAGS = -g \
	-ffreestanding \
	-O0 \
	-nostdlib \
	-falign-jumps \
	-falign-functions \
	-falign-labels \
	-falign-loops \
	-fstrength-reduce \
	-fomit-frame-pointer \
	-finline-functions \
	-Wno-unused-function \
	-fno-builtin \
	-Werror \
	-Wno-unused-label \
	-Wno-cpp \
	-Wno-unused-parameter \
	-nostartfiles \
	-nodefaultlibs \
	-Iinc \
	-Wall

all: ./bin/boot.bin ./bin/kernel.bin apps
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin
	sudo mount -t vfat ./bin/os.bin /mnt/d
	echo "Hello, World!" > hello.txt
	sudo cp ./hello.txt /mnt/d
	echo "This is a test file." > file2.txt
	sudo cp ./file2.txt /mnt/d
	sudo mkdir /mnt/d/test
	sudo cp ./hello.txt /mnt/d/test
	sudo cp ./user/blank.bin/blank.bin /mnt/d
	sudo cp ./user/blank.elf/blank.elf /mnt/d
	sudo cp ./user/cblank.elf/cblank.elf /mnt/d
	sudo umount /mnt/d
	rm -rf ./hello.txt ./file2.txt

./bin/kernel.bin: $(filter-out ./build/src/grub/%, $(FILES))
	i686-elf-ld -g -relocatable $(filter-out ./build/grub/%, $(FILES)) -o ./build/kernelfull.o
	i686-elf-gcc $(FLAGS) -T ./src/linker.ld -o ./bin/kernel.bin ./build/kernelfull.o

./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin -g ./src/boot/boot.asm -o ./bin/boot.bin

./build/%.asm.o: ./src/%.asm
	nasm -f elf -g $< -o $@

./build/%.o: ./src/%.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

grub: ./bin/kernel-grub.bin
	grub-file --is-x86-multiboot ./bin/kernel-grub.bin
	sudo mount -t vfat ./disk.img /mnt/d
	sudo cp ./bin/kernel-grub.bin /mnt/d/boot/myos.kernel
	sudo cp ./user/blank.bin/blank.bin /mnt/d
	sudo cp ./user/blank.elf/blank.elf /mnt/d
	sudo cp ./user/cblank.elf/cblank.elf /mnt/d
	sudo umount -q /mnt/d

./bin/kernel-grub.bin: $(filter-out ./build/kernel/%.asm.o, $(FILES))
	i686-elf-ld -g -relocatable $(filter-out ./build/kernel/%.asm.o, $(FILES)) -o ./build/kernelfull.o
	i686-elf-gcc $(FLAGS) -T ./src/grub/linker.ld -o ./bin/kernel-grub.bin ./build/kernelfull.o

qemu: all
	qemu-system-i386 -boot d -hda ./bin/os.bin -m 512 -serial stdio -display gtk,zoom-to-fit=on

qemu_grub: grub clean
	qemu-system-i386 -hda ./disk.img -m 512 -serial stdio -display gtk,zoom-to-fit=on

apps:
	cd ./user/stdlib && $(MAKE) all
	cd ./user/blank.bin && $(MAKE) all
	cd ./user/blank.elf && $(MAKE) all
	cd ./user/cblank.elf && $(MAKE) all

apps_clean:
	cd ./user/stdlib && $(MAKE) clean
	cd ./user/blank.bin && $(MAKE) clean
	cd ./user/blank.elf && $(MAKE) clean
	cd ./user/cblank.elf && $(MAKE) clean

clean: apps_clean
	rm -rf ./bin ./build ./mnt
