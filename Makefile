$(shell mkdir -p ./bin)
SRC_DIRS := $(shell find ./src -type d ! -path './src/include' ! -path './src/grub' ! -path './src/boot')
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

all: ./bin/boot.bin ./bin/kernel.bin
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
	sudo umount /mnt/d
	rm -rf ./hello.txt ./file2.txt

./bin/kernel.bin: $(FILES)
	i686-elf-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
	i686-elf-gcc $(FLAGS) -T ./src/linker.ld -o ./bin/kernel.bin ./build/kernelfull.o

./bin/boot.bin: ./src/boot/boot.asm
	# nasm -felf32 ./src/boot/boot.asm -o ./bin/boot.bin
	nasm -f bin -g ./src/boot/boot.asm -o ./bin/boot.bin

# Pattern rules for .asm and .c files
./build/%.asm.o: ./src/%.asm
	nasm -f elf -g $< -o $@

./build/%.o: ./src/%.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

# NOT FUNCTIONAL YET
grub: ./bin/kernel.bin
	i686-elf-ld -g -relocatable $(FILES) -o ./build/kernelfull.o
	i686-elf-gcc $(FLAGS) -T ./src/linker.ld -o ./bin/myos.bin ./build/kernelfull.o
	grub-file --is-x86-multiboot ./bin/kernel.bin
	sudo mount -t vfat ./disk.img /mnt/d
	sudo cp ./bin/kernel.bin /mnt/d/boot/myos.kernel
	sudo umount -q /mnt/d

clean:
	rm -rf ./bin ./build ./mnt
