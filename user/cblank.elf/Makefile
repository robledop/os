FILES = ./build/cblank.o 
INCLUDES = -I./
$(shell mkdir -p ./build)
all: ./build/cblank.o 
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c ./cblank.c -o ./build/cblank.o
	i686-elf-gcc -g -T ./linker.ld -o ./cblank.elf -ffreestanding -O0 -nostdlib -fpic -g $(FILES) ../stdlib/stdlib.elf

./build/cblank.o: ./cblank.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu23 -c ./cblank.c -o ./build/cblank.o


clean:
	rm -rf ./build ./cblank.elf