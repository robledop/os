  ; Boot loader
  ORG 0x7c00
  BITS 16

  CODE_SEG equ gdt_code - gdt_start
  DATA_SEG equ gdt_data - gdt_start

  ; BIOS Paramater Block
  ; https://wiki.osdev.org/FAT#Boot_Record
  jmp short start
  nop

  ; FAT16 Header
OEMID            db 'OSDEV   '   ; OEM Identifier
ByterPerSector   dw 0x200        ; 512 bytes per sector
SectorPerCluster db 0x80         ; 128 sectors per cluster
ReservedSectors  dw 200          ; 200 reserved sectors
FATCopies        db 2            ; 2 FAT copies
RootDirEntries   dw 0x40         ; 64 root directory entries
NumSectors       dw 0x00         ; 0 sectors
MediaType        db 0xf8         ; Media type
SectorsPerFAT    dw 0x100        ; 256 sectors per FAT
SectorsPerTrack  dw 0x20         ; 32 sectors per track
NumberOfHeads    dw 0x40         ; 64 heads
HiddenSectors    dd 0x00         ; 0 hidden sectors
SectorsBig       dd 0x773594      

  ; Extended BPB (DOS 4.0)
DriveNumber      db 0x80         ; Drive number
WinNTBit         db 0x00         ; Windows NT bit
Signature        db 0x29         ; Signature
VolumeID         dd 0xD105       ; Volume ID
VolumeIDString   db 'NO NAME    '; Volume ID String
SystemIDString   db 'FAT16   '   ; System ID String


start:
  jmp 0x0:step2
  cli ; clear interrupts

step2:
  mov ax, 0x00
  mov ds, ax ; set data segment
  mov es, ax ; set extra segment
  mov ss, ax ; set stack segment
  mov sp, 0x7c00 ; set stack pointer


  sti ; Enables interrupts

  mov ax, 0x0003 ; VGA text mode, 720x400 pixels, 9x16 font, 80x25 characters
  int 0x10
  ;mov ax, 0x1112 ; replace 9x16 font with 9x8 font, so now it's 80x50 characters
  ;mov bl, 0x00
  ;int 0x10

  ; https://wiki.osdev.org/Protected_Mode
.load_protected:
  cli
  lgdt[gdt_descriptor]
  mov eax, cr0
  or eax, 0x1
  mov cr0, eax
  jmp CODE_SEG:load32
  jmp $

  ; GDT https://wiki.osdev.org/Global_Descriptor_Table
gdt_start:
gdt_null:
  dd 0x0
  dd 0x0

  ; offset 0x8
gdt_code:   ; CS should point to this
  dw 0xFFFF ; Segment limit first 0-15 bits
  dw 0      ; Base first 0-15 bits
  db 0      ; Base 16-23 bits
  db 0x9a   ; Access byte
  db 11001111b ; High 4 bit flags and the low 4 bit flags
  db 0      ; Base 24-31 bits

  ; offset 0x10
gdt_data:   ; DS, SS, ES, FS, GS
  dw 0xFFFF ; Segment limit first 0-15 bits
  dw 0      ; Base first 0-15 bits
  db 0      ; Base 16-23 bits
  db 0x92   ; Access byte
  db 11001111b ; High 4 bit flags and the low 4 bit flags
  db 0      ; Base 24-31 bits

gdt_end:

gdt_descriptor:
  dw gdt_end - gdt_start - 1
  dd gdt_start

  [BITS 32]
load32:
  mov eax, 1
  mov ecx, 100
  mov edi, 0x0100000
  call ata_lba_read
  jmp CODE_SEG:0x0100000

ata_lba_read:
  mov ebx, eax ; Backup the LBA
  ; Send the highest 8 bits of the LBA to hard disk controller
  shr eax, 24
  or eax, 0xE0  ; Select the master drive
  mov dx, 0x1F6
  out dx, al

  ; Send the total sectors to read
  mov eax, ecx
  mov dx, 0x1F2
  out dx, al

  ; Send more bits of the LBA
  mov eax, ebx ; Restore the backup LBA
  mov dx, 0x1F3
  out dx, al

  ; Send more bits of the LBA
  mov dx, 0x1F4
  mov eax, ebx ; Restore the backup LBA
  shr eax, 8
  out dx, al

  ; Send uppper 16 bits of the LBA
  mov dx, 0x1F5
  mov eax, ebx ; Restore the backup LBA
  shr eax, 16
  out dx, al
  
  mov dx, 0x1F7
  mov al, 0x20
  out dx, al

  ; Read all sectors into memory
.next_sector:
  push ecx

  ; Checking if we need to read
.try_again:
  mov dx, 0x1F7
  in al, dx
  test al, 8
  jz .try_again

  ; We need to read 256 words at a time
  mov ecx, 256
  mov dx, 0x1F0
  rep insw
  pop ecx
  loop .next_sector

  ret ; ata_lba_read 

  ; pad the end with 510 bytes
  times 510-($ - $$) db 0
  dw 0xAA55

  ; nasm -f bin ./boot.asm -o ./boot.bin to compile this
  ; qemu-system-x86_64 -hda ./boot.bin to run in on qemu

  ; To burn this into an USB drive
  ; sudo fdisk -l
  ; To list all drives and find the USB stick like /dev/sdb
  ; sudo dd if=./boot.bin of=/dev/sdb
