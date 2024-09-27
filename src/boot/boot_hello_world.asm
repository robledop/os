  ; Hello world boot loader
  ORG 0x7c00
  BITS 16

start:
  mov si, message
  call print

  ; infinite loop
  jmp $

print:
  mov bx, 0 ; sets the page number for the Teletype output BIOS interrupt

.loop:
  lodsb ; gets the address in the si register and loads its contents into the the al register. It starts with the letter 'H' from "Hello world". It then increments the si register
  cmp al, 0 ; message is a null terminated string, so we are checking if we reached its end here
  je .done ; if we reached the end go to .done
  call print_char ; otherwise call print_char and loop
  jmp .loop

.done:
  ret

print_char:
  ; Video - Teletype output BIOS interrupt
  ; https://www.ctyme.com/intr/rb-0106.htm
  mov ah, 0eh
  int 0x10
  ret

message: db 'Hello world!', 0
  ; pad the end with 510 bytes
  times 510-($ - $$) db 0
  dw 0xAA55

  ; nasm -f bin ./boot.asm -o ./boot.bin to compile this
  ; qemu-system-x86_64 -hda ./boot.bin to run in on qemu
