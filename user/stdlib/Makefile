$(shell mkdir -p build)
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
	-Og \
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

all: $(FILES) ./build/memory.o ./build/string.o
	ar rcs libstdlib.a $(FILES) ./build/memory.o ./build/string.o

./build/%.asm.o: ./src/%.asm
	nasm -f elf -g $< -o $@

./build/%.o: ./src/%.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

# Files shared with the kernel
./build/memory.o: ../../kernel/memory/memory.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

./build/string.o: ../../kernel/string/string.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c $< -o $@

clean:
	rm -rf ./build  ./libstdlib.a