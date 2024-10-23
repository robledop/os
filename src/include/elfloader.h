#pragma once


#include "elf.h"
#include "config.h"

struct elf_file
{
    char filename[MAX_PATH_LENGTH];

    uint32_t in_memory_size;

    // The physical memory address that this elf file is loaded at
    void *elf_memory;
    void *virtual_base_address;
    void *virtual_end_address;
    void *physical_base_address;
    // The physical end address of this binary
    void *physical_end_address;
};

int elf_load(const char *filename, struct elf_file **file_out);
void elf_close(struct elf_file *file);
void *elf_virtual_base(const struct elf_file *file);
void *elf_virtual_end(const struct elf_file *file);
void *elf_phys_base(const struct elf_file *file);
void *elf_phys_end(const struct elf_file *file);

struct elf_header *elf_header(const struct elf_file *file);
struct elf32_shdr *elf_sheader(struct elf_header *header);
void *elf_memory(const struct elf_file *file);
struct elf32_phdr *elf_pheader(struct elf_header *header);
struct elf32_phdr *elf_program_header(struct elf_header *header, int index);
struct elf32_shdr *elf_section(struct elf_header *header, int index);
void *elf_phdr_phys_address(const struct elf_file *file, const struct elf32_phdr *phdr);
int elf_process_loaded(struct elf_file *elf_file);
