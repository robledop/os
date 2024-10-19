#include "elfloader.h"
#include "config.h"
#include "file.h"
#include "kernel.h"
#include "kernel_heap.h"
#include "serial.h"
#include "status.h"
#include "string.h"

constexpr char elf_signature[] = {0x7f, 'E', 'L', 'F'};

static bool elf_valid_signature(const void *buffer)
{
    return memcmp(buffer, (void *)elf_signature, sizeof(elf_signature)) == 0;
}

static bool elf_valid_class(const struct elf_header *header)
{
    // We only support 32 bit binaries.
    return header->e_ident[EI_CLASS] == ELFCLASSNONE || header->e_ident[EI_CLASS] == ELFCLASS32;
}

static bool elf_valid_encoding(const struct elf_header *header)
{
    return header->e_ident[EI_DATA] == ELFDATANONE || header->e_ident[EI_DATA] == ELFDATA2LSB;
}

static bool elf_is_executable(const struct elf_header *header)
{
    return header->e_type == ET_EXEC && header->e_entry >= PROGRAM_VIRTUAL_ADDRESS;
}

static bool elf_has_program_header(const struct elf_header *header)
{
    return header->e_phoff != 0;
}

void *elf_memory(const struct elf_file *file)
{
    return file->elf_memory;
}

struct elf_header *elf_header(const struct elf_file *file)
{
    return file->elf_memory;
}

struct elf32_shdr *elf_sheader(struct elf_header *header)
{
    return (struct elf32_shdr *)((int)header + header->e_shoff);
}

struct elf32_phdr *elf_pheader(struct elf_header *header)
{
    if (header->e_phoff == 0) {
        return nullptr;
    }

    return (struct elf32_phdr *)((int)header + header->e_phoff);
}

struct elf32_phdr *elf_program_header(struct elf_header *header, const int index)
{
    return &elf_pheader(header)[index];
}

struct elf32_shdr *elf_section(struct elf_header *header, const int index)
{
    return &elf_sheader(header)[index];
}

void *elf_phdr_phys_address(const struct elf_file *file, const struct elf32_phdr *phdr)
{
    return (char *)elf_memory(file) + phdr->p_offset;
}

char *elf_str_table(struct elf_header *header)
{
    return (char *)header + elf_section(header, header->e_shstrndx)->sh_offset;
}

void *elf_virtual_base(const struct elf_file *file)
{
    return file->virtual_base_address;
}

void *elf_virtual_end(const struct elf_file *file)
{
    return file->virtual_end_address;
}

void *elf_phys_base(const struct elf_file *file)
{
    return file->physical_base_address;
}

void *elf_phys_end(const struct elf_file *file)
{
    return file->physical_end_address;
}

int elf_validate_loaded(const struct elf_header *header)
{
    if (header == nullptr) {
        return -EINFORMAT;
    }
    return (elf_valid_signature(header) && elf_valid_class(header) && elf_valid_encoding(header) &&
            elf_has_program_header(header))
        ? ALL_OK
        : -EINFORMAT;
}

int elf_process_phdr_pt_load(struct elf_file *elf_file, const struct elf32_phdr *phdr)
{
    if (elf_file->virtual_base_address >= (void *)phdr->p_vaddr || elf_file->virtual_base_address == nullptr) {
        elf_file->virtual_base_address  = (void *)phdr->p_vaddr;
        elf_file->physical_base_address = (char *)elf_memory(elf_file) + phdr->p_offset;
    }

    const unsigned int end_virtual_address = phdr->p_vaddr + phdr->p_filesz;
    if (elf_file->virtual_end_address <= (void *)(end_virtual_address) || elf_file->virtual_end_address == nullptr) {
        elf_file->virtual_end_address  = (void *)end_virtual_address;
        elf_file->physical_end_address = (char *)elf_memory(elf_file) + phdr->p_offset + phdr->p_filesz;
    }
    return 0;
}

int elf_process_pheader(struct elf_file *elf_file, const struct elf32_phdr *phdr)
{
    int res = 0;
    switch (phdr->p_type) {
    case PT_LOAD:
        res = elf_process_phdr_pt_load(elf_file, phdr);
        break;
    default:
        panic("Invalid program header type\n");
        break;
    }
    return res;
}

int elf_process_pheaders(struct elf_file *elf_file)
{
    int res                   = 0;
    struct elf_header *header = elf_header(elf_file);
    for (int i = 0; i < header->e_phnum; i++) {
        const struct elf32_phdr *phdr = elf_program_header(header, i);
        res                           = elf_process_pheader(elf_file, phdr);
        if (res < 0) {
            warningf("Failed to process program header %d\n", i);
            break;
        }
    }
    return res;
}

int elf_process_loaded(struct elf_file *elf_file)
{
    int res                         = 0;
    const struct elf_header *header = elf_header(elf_file);
    if (header == nullptr) {
        return -EINFORMAT;
    }

    res = elf_validate_loaded(header);
    if (res < 0) {
        warningf("Failed to validate loaded ELF file\n");
        goto out;
    }

    res = elf_process_pheaders(elf_file);
    if (res < 0) {
        warningf("Failed to process program headers for ELF file\n");
        goto out;
    }

out:
    return res;
}

int elf_load(const char *filename, struct elf_file **file_out)
{
    dbgprintf("Loading ELF file %s\n", filename);
    struct elf_file *elf_file = kzalloc(sizeof(struct elf_file));
    int fd                    = 0;

    int res = fopen(filename, "r");
    if (res <= 0) {
        warningf("Failed to open file %s\n", filename);
        res = -EIO;
        goto out;
    }

    fd = res;
    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res < 0) {
        warningf("Failed to get file stat for %s\n", filename);
        goto out;
    }

    elf_file->elf_memory = kzalloc(stat.size);
    res                  = fread(elf_file->elf_memory, stat.size, 1, fd);
    if (res < 0) {
        warningf("Failed to read file %s\n", filename);
        goto out;
    }

    res = elf_process_loaded(elf_file);
    if (res < 0) {
        warningf("Failed to process loaded ELF file %s\n", filename);
        goto out;
    }
    elf_file->in_memory_size = stat.size;

    *file_out = elf_file;

out:
    fclose(fd);
    return res;
}

void elf_close(struct elf_file *file)
{
    if (!file) {
        warningf("Invalid ELF file\n");
        return;
    }

    kfree(file->elf_memory);
    kfree(file);
}