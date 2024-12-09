$(shell mkdir -p build)
$(shell mkdir -p ../../rootfs/lib)
SRC_DIRS := $(shell find ./src -type d)
BUILD_DIRS := $(patsubst ./src/%,./build/%,$(SRC_DIRS))
$(shell mkdir -p $(BUILD_DIRS))
ASM_FILES := $(wildcard $(addsuffix /*.asm, $(SRC_DIRS)))
C_FILES := $(wildcard $(addsuffix /*.c, $(SRC_DIRS)))
ASM_OBJS := $(ASM_FILES:./src/%.asm=./build/%.asm.o)
C_OBJS := $(C_FILES:./src/%.c=./build/%.o)
FILES := $(ASM_OBJS) $(C_OBJS)
INCLUDES = -I../../include
FLAGS = -g \
	-O0 \
	-ffreestanding \
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
	-Wno-unused-label \
	-Wno-cpp \
	-Wno-unused-parameter \
	-nostartfiles \
	-nodefaultlibs \
	-save-temps \
	-Iinc \
	-Werror \
	-Wextra \
	-Wall

FLAGS += -D__USER__

all: $(FILES) ./build/memory.o ./build/string.o ./build/printf.o ./build/math.o ./build/time.o ./build/assert.o ./build/errno.o
	ar rcs libc.a $(FILES) ./build/memory.o ./build/string.o ./build/printf.o ./build/math.o ./build/time.o ./build/assert.o ./build/errno.o
	cp libc.a ../../rootfs/lib

./build/%.asm.o: ./src/%.asm
	nasm -f elf -g $< -o $@

./build/%.o: ./src/%.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

# Files shared with the kernel
./build/memory.o: ../../kernel/lib/memory.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

./build/string.o: ../../kernel/lib/string.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

./build/printf.o: ../../kernel/lib/printf.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

./build/math.o: ../../kernel/lib/math.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

./build/time.o: ../../kernel/lib/time.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

./build/assert.o: ../../kernel/lib/assert.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

./build/errno.o: ../../kernel/lib/errno.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

clean:
	rm -rf ./build  ./libc.a ../../rootfs/lib
