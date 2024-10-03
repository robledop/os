FILES = ./build/cblank.o 
INCLUDES = -I./ -I../stdlib/include
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

$(shell mkdir -p ./build)
all: ./build/cblank.o 
	i686-elf-gcc -g -T ./linker.ld -o ./cblank.elf -ffreestanding -O0 -nostdlib -fpic -g $(FILES) ../stdlib/stdlib.elf

./build/cblank.o: ./cblank.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu99 -c ./cblank.c -o ./build/cblank.o


clean:
	rm -rf ./build ./cblank.elf