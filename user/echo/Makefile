mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

# By default, the output file is the name of the directory
OUTPUT = $(current_dir)

$(shell mkdir -p build)
SRC_DIRS := $(shell find ./src -type d)
BUILD_DIRS := $(patsubst ./src/%,./build/%,$(SRC_DIRS))
$(shell mkdir -p $(BUILD_DIRS))
ASM_FILES := $(wildcard $(addsuffix /*.asm, $(SRC_DIRS)))
C_FILES := $(wildcard $(addsuffix /*.c, $(SRC_DIRS)))
ASM_OBJS := $(ASM_FILES:./src/%.asm=./build/%.asm.o)
C_OBJS := $(C_FILES:./src/%.c=./build/%.o)
FILES := $(ASM_OBJS) $(C_OBJS)
INCLUDES = -I./ -I../../include
FLAGS = -g \
	-ffreestanding \
	-Og \
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
	-save-temps \
	-Iinc \
	-Wall

FLAGS += -D__USER__

all: $(FILES)
	i686-elf-gcc -g -T ./linker.ld -o ../../rootfs/bin/$(OUTPUT) -ffreestanding -O0 -nostdlib -fpic -g $(FILES) ../libc/libc.a

./build/%.asm.o: ./src/%.asm
	nasm -f elf -g $< -o $@

./build/%.o: ./src/%.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

clean:
	rm -rf ./build ../../rootfs/bin/$(OUTPUT)