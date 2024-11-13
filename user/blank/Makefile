$(shell mkdir -p ./build)
all:
	nasm -f elf ./blank.asm -o ./build/blank.o
	i686-elf-gcc -g -T ./linker.ld -o ../../rootfs/bin/blank.bin -ffreestanding -Og -nostdlib -fpic ./build/blank.o

clean:
	rm -rf ./build ../../rootfs/bin/blank.bin