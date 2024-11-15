#include <debug.h>
#include <string.h>
#include <x86.h>

void print_registers();
typedef struct stack_frame {
    struct stack_frame *ebp;
    uint32_t eip;
} stack_frame_t;


static struct elf32_shdr *symtab_section_header = NULL;
static struct elf32_shdr *elf_section_headers;

void stack_trace(void)
{
    kprintf(KBOLD KWHT "Stack trace:\n" KRESET);

    const stack_frame_t *stack = __builtin_frame_address(0);
    // struct stack_frame *stack;
    // asm volatile("movl %%ebp, %0" : "=r"(stack));
    int max = 10;
    while (stack && stack->eip != 0 && max-- > 0) {
        auto const symbol = debug_function_symbol_lookup(stack->eip);
        kprintf("\t%p [%s + %d]\n",
                stack->eip,
                (symbol.name == NULL) ? "[unknown]" : symbol.name,
                stack->eip - symbol.address);
        stack = stack->ebp;
    }
}

/// @brief Prints the call stack, as a list of function addresses
/// gdb or addr2line can be used to translate these into function names
void debug_stats(void)
{
    print_registers();
    stack_trace();
}

struct symbol debug_function_symbol_lookup(const elf32_addr address)
{
    if (!symtab_section_header) {
        return (struct symbol){0, NULL};
    }

    auto const symbols_table = (struct elf32_sym *)symtab_section_header->sh_addr;
    if (!symbols_table) {
        return (struct symbol){0, NULL};
    }
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

void assert(const char *snippet, const char *file, int line, const char *message, ...)
{
    kprintf(KBOLD KWHT "assert failed %s:%d %s" KRESET, file, line, snippet);

    if (*message) {
        va_list arg;
        va_start(arg, message);
        const char *data = va_arg(arg, char *);
        kprintf(data, arg);
        panic(message);
    }
    panic("Assertion failed");
}

void print_registers()
{
    kprintf(KBOLD KWHT "Registers:\n" KRESET);
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip;
    asm volatile("movl %%eax, %0" : "=r"(eax));
    asm volatile("movl %%ebx, %0" : "=r"(ebx));
    asm volatile("movl %%ecx, %0" : "=r"(ecx));
    asm volatile("movl %%edx, %0" : "=r"(edx));
    asm volatile("movl %%esi, %0" : "=r"(esi));
    asm volatile("movl %%edi, %0" : "=r"(edi));
    asm volatile("movl %%ebp, %0" : "=r"(ebp));
    asm volatile("movl %%esp, %0" : "=r"(esp));
    asm volatile("1: movl $1b, %0" : "=r"(eip));
    kprintf("\tEAX: %p", eax);
    kprintf("\tEBX: %p", ebx);
    kprintf("\tECX: %p", ecx);
    kprintf("\tEDX: %p\n", edx);
    kprintf("\tESI: %p", esi);
    kprintf("\tEDI: %p", edi);
    kprintf("\tEBP: %p", ebp);
    kprintf("\tESP: %p\n", esp);
    kprintf("\tEIP: %p", eip);
    auto eflags = read_eflags();

    kprintf("\tEFLAGS: %p ", eflags);

    if (eflags & EFLAGS_ALL) {
        kprintf("( ");
    }
    if (eflags & EFLAGS_CF) {
        kprintf("CF ");
    }
    if (eflags & EFLAGS_PF) {
        kprintf("PF ");
    }
    if (eflags & EFLAGS_AF) {
        kprintf("AF ");
    }
    if (eflags & EFLAGS_ZF) {
        kprintf("ZF ");
    }
    if (eflags & EFLAGS_SF) {
        kprintf("SF ");
    }
    if (eflags & EFLAGS_TF) {
        kprintf("TF ");
    }
    if (eflags & EFLAGS_IF) {
        kprintf("IF ");
    }
    if (eflags & EFLAGS_DF) {
        kprintf("DF ");
    }
    if (eflags & EFLAGS_OF) {
        kprintf("OF ");
    }
    if (eflags & EFLAGS_IOPL) {
        kprintf("IOPL ");
    }
    if (eflags & EFLAGS_NT) {
        kprintf("NT ");
    }
    if (eflags & EFLAGS_RF) {
        kprintf("RF ");
    }
    if (eflags & EFLAGS_VM) {
        kprintf("VM ");
    }
    if (eflags & EFLAGS_AC) {
        kprintf("AC ");
    }
    if (eflags & EFLAGS_VIF) {
        kprintf("VIF ");
    }
    if (eflags & EFLAGS_VIP) {
        kprintf("VIP ");
    }
    if (eflags & EFLAGS_ID) {
        kprintf("ID ");
    }
    if (eflags & EFLAGS_AI) {
        kprintf("AI ");
    }

    if (eflags & EFLAGS_ALL) {
        kprintf(")");
    }
    kprintf("\n");
}
