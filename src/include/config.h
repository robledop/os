#ifndef CONFIG_H
#define CONFIG_H

// config.asm is generated from config.h using the script c_to_nasm.sh
// This file must only contain #ifndef, #define and #endif

#define TOTAL_INTERRUPTS 512

#define HEAP_SIZE_BYTES 104857600 // 100MB
#define HEAP_BLOCK_SIZE 4096

// https://wiki.osdev.org/Memory_Map_(x86)
#define HEAP_ADDRESS 0x01000000
#define HEAP_TABLE_ADDRESS 0x00007E00

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define MAX_PATH_LENGTH 108
#define MAX_FILE_SYSTEMS 10
#define MAX_FILE_DESCRIPTORS 512

#define MAX_FMT_STR 10240

#define PROGRAM_VIRTUAL_ADDRESS 0x400000

// Must be aligned to 4096 bytes page size
#define USER_STACK_SIZE (1024 * 512)

#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR 0x1B
#define USER_DATA_SELECTOR 0x23
#define TSS_SELECTOR 0x28

#define USER_STACK_TOP 0x3FF000
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

#define MAX_PROGRAM_ALLOCATIONS 1024
#define MAX_PROCESSES 256

#define MAX_SYSCALLS 1024
#define KEYBOARD_BUFFER_SIZE 1024

// Multitasking enabled
// #define MULTITASKING

#define KERNEL_LOAD_ADDRESS 0x200000

#endif