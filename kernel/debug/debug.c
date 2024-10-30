#include <debug.h>
#include <string.h>

typedef struct stack_frame {
    struct stack_frame *ebp;
    uint32_t eip;
} stack_frame_t;


static struct elf32_shdr *symtab_section_header = NULL;
static struct elf32_shdr *elf_section_headers;

/// @brief Prints the call stack, as a list of function addresses
/// gdb or addr2line can be used to translate these into function names
void debug_callstack(void)
{
    stack_frame_t *stack;
    asm volatile("movl %%ebp, %0" : "=r"(stack));
    kprintf("Stack trace:\n");
    while (stack->eip != 0) {
        auto symbol = debug_symbol_lookup(stack->eip);
        kprintf("\t<%p> [%s + %d]\n",
                stack->eip,
                (symbol.name == NULL) ? "[unknown]" : symbol.name,
                stack->eip - symbol.address);
        stack = stack->ebp;
    }
}

struct symbol debug_symbol_lookup(const elf32_addr address)
{
    if (!symtab_section_header) {
        return (struct symbol){0, NULL};
    }

    auto const symbols_table      = (struct elf32_sym *)symtab_section_header->sh_addr;
    const uint32_t symtab_entries = symtab_section_header->sh_size / sizeof(struct elf32_sym);

    const struct elf32_shdr *strtab_sh = &elf_section_headers[symtab_section_header->sh_link];
    auto const strtab_addr             = (char *)strtab_sh->sh_addr;

    const struct elf32_sym *closest_func = NULL;
    for (uint32_t i = 0; i < symtab_entries; i++) {
        const struct elf32_sym *sym = &symbols_table[i];
        if (sym->st_value == address) {
            char *symbol_name          = strtab_addr + sym->st_name;
            const struct symbol symbol = {
                .address = sym->st_value,
                .name    = symbol_name,
            };
            return symbol;
        }

        if (sym->st_value < address && (!closest_func || sym->st_value > closest_func->st_value)) {
            closest_func = sym;
        }
    }
    if (closest_func) {
        char *symbol_name          = strtab_addr + closest_func->st_name;
        const struct symbol symbol = {
            .address = closest_func->st_value,
            .name    = symbol_name,
        };
        return symbol;
    }

    return (struct symbol){0, NULL};
}

void init_symbols(const multiboot_info_t *mbd)
{
    if (!(mbd->flags & MULTIBOOT_INFO_ELF_SHDR)) {
        return;
    }

    const uint32_t num    = mbd->u.elf_sec.num;
    elf_section_headers   = (struct elf32_shdr *)mbd->u.elf_sec.addr;
    const char *sh_strtab = (char *)elf_section_headers[mbd->u.elf_sec.shndx].sh_addr;

    for (uint32_t i = 0; i < num; i++) {
        struct elf32_shdr *sh    = &elf_section_headers[i];
        const char *section_name = sh_strtab + sh->sh_name;

        if (starts_with(".symtab", section_name)) {
            symtab_section_header = sh;
            break;
        }
    }
}
