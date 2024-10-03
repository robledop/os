$(shell mkdir -p ./build)
all:
	nasm -f elf ./blank.asm -o ./build/blank.o
	i686-elf-gcc -g -T ./linker.ld -o ./blank.elf -ffreestanding -O0 -nostdlib -fpic ./build/blank.o

clean:
	rm -rf ./build ./blank.elf