$(shell mkdir -p ./build)
FILES = ./build/shell.o 
INCLUDES = -I./ -I./include -I../stdlib/include
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

all: ./build/shell.o 
	i686-elf-gcc -g -T ./linker.ld -o ./shell.elf -ffreestanding -O0 -nostdlib -fpic -g $(FILES) ../stdlib/stdlib.elf

./build/shell.o: ./src/shell.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c ./src/shell.c -o ./build/shell.o


clean:
	rm -rf ./build ./shell.elf