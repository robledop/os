#include <debug.h>
#include <kernel_heap.h>
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
    printf(KBWHT "Stack trace:\n" KRESET);

    const stack_frame_t *stack = __builtin_frame_address(0);
    int max                    = 10;
    while (stack && stack->eip != 0 && max-- > 0) {
        auto const symbol = debug_function_symbol_lookup(stack->eip);
        printf("\t%#lx " KBWHT "[" KRESET KCYN "%s" KWHT " + %#lx" KBWHT "]\n" KRESET,
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
    kernel_heap_print_stats();
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
    printf(KBWHT "assert failed %s:%d %s" KRESET, file, line, snippet);

    if (*message) {
        va_list arg;
        va_start(arg, message);
        const char *data = va_arg(arg, char *);
        printf(data, arg);
        panic(message);
    }
    panic("Assertion failed");
}

void print_registers()
{
    printf(KBWHT "Registers:\n" KRESET);
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
    printf("\tEAX: %#010lx", eax);
    printf("\tEBX: %#010lx", ebx);
    printf("\tECX: %#010lx", ecx);
    printf("\tEDX: %#010lx\n", edx);
    printf("\tESI: %#010lx", esi);
    printf("\tEDI: %#010lx", edi);
    printf("\tEBP: %#010lx", ebp);
    printf("\tESP: %#010lx\n", esp);
    printf("\tEIP: %#010lx", eip);
    auto eflags = read_eflags();

    printf("\tEFLAGS: %#010lx ", eflags);

    if (eflags & EFLAGS_ALL) {
        printf("( ");
    }
    if (eflags & EFLAGS_CF) {
        printf("CF ");
    }
    if (eflags & EFLAGS_PF) {
        printf("PF ");
    }
    if (eflags & EFLAGS_AF) {
        printf("AF ");
    }
    if (eflags & EFLAGS_ZF) {
        printf("ZF ");
    }
    if (eflags & EFLAGS_SF) {
        printf("SF ");
    }
    if (eflags & EFLAGS_TF) {
        printf("TF ");
    }
    if (eflags & EFLAGS_IF) {
        printf("IF ");
    }
    if (eflags & EFLAGS_DF) {
        printf("DF ");
    }
    if (eflags & EFLAGS_OF) {
        printf("OF ");
    }
    if (eflags & EFLAGS_IOPL) {
        printf("IOPL ");
    }
    if (eflags & EFLAGS_NT) {
        printf("NT ");
    }
    if (eflags & EFLAGS_RF) {
        printf("RF ");
    }
    if (eflags & EFLAGS_VM) {
        printf("VM ");
    }
    if (eflags & EFLAGS_AC) {
        printf("AC ");
    }
    if (eflags & EFLAGS_VIF) {
        printf("VIF ");
    }
    if (eflags & EFLAGS_VIP) {
        printf("VIP ");
    }
    if (eflags & EFLAGS_ID) {
        printf("ID ");
    }
    if (eflags & EFLAGS_AI) {
        printf("AI ");
    }

    if (eflags & EFLAGS_ALL) {
        printf(")");
    }
    printf("\n");
}
