#include "io.h"
#include <cpuid.h>
#include "terminal.h"

// Vendor strings from CPUs.
#define CPUID_VENDOR_AMD "AuthenticAMD"
#define CPUID_VENDOR_AMD_OLD "AMDisbetter!" // Early engineering samples of AMD K5 processor
#define CPUID_VENDOR_INTEL "GenuineIntel"
#define CPUID_VENDOR_VIA "VIA VIA VIA "
#define CPUID_VENDOR_TRANSMETA "GenuineTMx86"
#define CPUID_VENDOR_TRANSMETA_OLD "TransmetaCPU"
#define CPUID_VENDOR_CYRIX "CyrixInstead"
#define CPUID_VENDOR_CENTAUR "CentaurHauls"
#define CPUID_VENDOR_NEXGEN "NexGenDriven"
#define CPUID_VENDOR_UMC "UMC UMC UMC "
#define CPUID_VENDOR_SIS "SiS SiS SiS "
#define CPUID_VENDOR_NSC "Geode by NSC"
#define CPUID_VENDOR_RISE "RiseRiseRise"
#define CPUID_VENDOR_VORTEX "Vortex86 SoC"
#define CPUID_VENDOR_AO486 "MiSTer AO486"
#define CPUID_VENDOR_AO486_OLD "GenuineAO486"
#define CPUID_VENDOR_ZHAOXIN "  Shanghai  "
#define CPUID_VENDOR_HYGON "HygonGenuine"
#define CPUID_VENDOR_ELBRUS "E2K MACHINE "

// Vendor strings from hypervisors.
#define CPUID_VENDOR_QEMU "TCGTCGTCGTCG"
#define CPUID_VENDOR_KVM " KVMKVMKVM  "
#define CPUID_VENDOR_VMWARE "VMwareVMware"
#define CPUID_VENDOR_VIRTUALBOX "VBoxVBoxVBox"
#define CPUID_VENDOR_XEN "XenVMMXenVMM"
#define CPUID_VENDOR_HYPERV "Microsoft Hv"
#define CPUID_VENDOR_PARALLELS " prl hyperv "
#define CPUID_VENDOR_PARALLELS_ALT " lrpepyh vr " // Sometimes Parallels incorrectly encodes "prl hyperv" as "lrpepyh vr" due to an endianness mismatch.
#define CPUID_VENDOR_BHYVE "bhyve bhyve "
#define CPUID_VENDOR_QNX " QNXQVMBSQG "

enum
{
    CPUID_FEAT_ECX_SSE3 = 1 << 0,
    CPUID_FEAT_ECX_PCLMUL = 1 << 1,
    CPUID_FEAT_ECX_DTES64 = 1 << 2,
    CPUID_FEAT_ECX_MONITOR = 1 << 3,
    CPUID_FEAT_ECX_DS_CPL = 1 << 4,
    CPUID_FEAT_ECX_VMX = 1 << 5,
    CPUID_FEAT_ECX_SMX = 1 << 6,
    CPUID_FEAT_ECX_EST = 1 << 7,
    CPUID_FEAT_ECX_TM2 = 1 << 8,
    CPUID_FEAT_ECX_SSSE3 = 1 << 9,
    CPUID_FEAT_ECX_CID = 1 << 10,
    CPUID_FEAT_ECX_SDBG = 1 << 11,
    CPUID_FEAT_ECX_FMA = 1 << 12,
    CPUID_FEAT_ECX_CX16 = 1 << 13,
    CPUID_FEAT_ECX_XTPR = 1 << 14,
    CPUID_FEAT_ECX_PDCM = 1 << 15,
    CPUID_FEAT_ECX_PCID = 1 << 17,
    CPUID_FEAT_ECX_DCA = 1 << 18,
    CPUID_FEAT_ECX_SSE4_1 = 1 << 19,
    CPUID_FEAT_ECX_SSE4_2 = 1 << 20,
    CPUID_FEAT_ECX_X2APIC = 1 << 21,
    CPUID_FEAT_ECX_MOVBE = 1 << 22,
    CPUID_FEAT_ECX_POPCNT = 1 << 23,
    CPUID_FEAT_ECX_TSC = 1 << 24,
    CPUID_FEAT_ECX_AES = 1 << 25,
    CPUID_FEAT_ECX_XSAVE = 1 << 26,
    CPUID_FEAT_ECX_OSXSAVE = 1 << 27,
    CPUID_FEAT_ECX_AVX = 1 << 28,
    CPUID_FEAT_ECX_F16C = 1 << 29,
    CPUID_FEAT_ECX_RDRAND = 1 << 30,
    // CPUID_FEAT_ECX_HYPERVISOR = 1 << 31,
    CPUID_FEAT_EDX_FPU = 1 << 0,
    CPUID_FEAT_EDX_VME = 1 << 1,
    CPUID_FEAT_EDX_DE = 1 << 2,
    CPUID_FEAT_EDX_PSE = 1 << 3,
    CPUID_FEAT_EDX_TSC = 1 << 4,
    CPUID_FEAT_EDX_MSR = 1 << 5,
    CPUID_FEAT_EDX_PAE = 1 << 6,
    CPUID_FEAT_EDX_MCE = 1 << 7,
    CPUID_FEAT_EDX_CX8 = 1 << 8,
    CPUID_FEAT_EDX_APIC = 1 << 9,
    CPUID_FEAT_EDX_SEP = 1 << 11,
    CPUID_FEAT_EDX_MTRR = 1 << 12,
    CPUID_FEAT_EDX_PGE = 1 << 13,
    CPUID_FEAT_EDX_MCA = 1 << 14,
    CPUID_FEAT_EDX_CMOV = 1 << 15,
    CPUID_FEAT_EDX_PAT = 1 << 16,
    CPUID_FEAT_EDX_PSE36 = 1 << 17,
    CPUID_FEAT_EDX_PSN = 1 << 18,
    CPUID_FEAT_EDX_CLFLUSH = 1 << 19,
    CPUID_FEAT_EDX_DS = 1 << 21,
    CPUID_FEAT_EDX_ACPI = 1 << 22,
    CPUID_FEAT_EDX_MMX = 1 << 23,
    CPUID_FEAT_EDX_FXSR = 1 << 24,
    CPUID_FEAT_EDX_SSE = 1 << 25,
    CPUID_FEAT_EDX_SSE2 = 1 << 26,
    CPUID_FEAT_EDX_SS = 1 << 27,
    CPUID_FEAT_EDX_HTT = 1 << 28,
    CPUID_FEAT_EDX_TM = 1 << 29,
    CPUID_FEAT_EDX_IA64 = 1 << 30,
    // CPUID_FEAT_EDX_PBE = 1 << 31
};

uint8_t inb(uint16_t p)
{
    uint8_t r;
    asm volatile("inb %%dx, %%al" : "=a"(r) : "d"(p));
    return r;
}

uint16_t inw(uint16_t p)
{
    uint16_t r;
    asm volatile("inw %%dx, %%ax" : "=a"(r) : "d"(p));
    return r;
}

uint32_t inl(uint16_t p)
{
    uint32_t r;
    asm volatile("inl %%dx, %%eax" : "=a"(r) : "d"(p));
    return r;
}

void outb(uint16_t portid, uint8_t value)
{
    asm volatile("outb %%al, %%dx" ::"d"(portid), "a"(value));
}

void outw(uint16_t portid, uint16_t value)
{
    asm volatile("outw %%ax, %%dx" ::"d"(portid), "a"(value));
}

void outl(uint16_t portid, uint32_t value)
{
    asm volatile("outl %%eax, %%dx" ::"d"(portid), "a"(value));
}

int cpu_get_model(void)
{
    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);
    return ebx;
}

static int check_apic(void)
{
    unsigned int eax, unused, edx;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_APIC;
}

int cpuid_string(int code, int where[4])
{
    asm volatile("cpuid" : "=a"(*where), "=b"(*(where + 0)),
                           "=d"(*(where + 1)), "=c"(*(where + 2)) : "a"(code));
    return (int)where[0];
}

static inline void cpuid(uint32_t reg, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "0"(reg));
}

char *cpu_string()
{
    static char s[16] = "CPUID_ERROR!";
    cpuid_string(0, (int *)(s));
    return s;
}

void cpu_print_info()
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t largest_standard_function;
    char vendor[13];
    cpuid(0, &largest_standard_function, (uint32_t *)(vendor + 0), (uint32_t *)(vendor + 8), (uint32_t *)(vendor + 4));
    vendor[12] = '\0';

    if (largest_standard_function >= 0x01)
    {
        cpuid(0x01, &eax, &ebx, &ecx, &edx);

        kprintf("Features:");

        if (edx & CPUID_FEAT_EDX_PSE)
            kprintf(" PSE");
        if (edx & CPUID_FEAT_EDX_PAE)
            kprintf(" PAE");
        if (edx & CPUID_FEAT_EDX_APIC)
            kprintf(" APIC");
        if (edx & CPUID_FEAT_EDX_MTRR)
            kprintf(" MTRR");

        kprintf("\n");

        kprintf("Instructions:");

        if (edx & CPUID_FEAT_EDX_TSC)
            kprintf(" TSC");
        if (edx & CPUID_FEAT_EDX_MSR)
            kprintf(" MSR");
        if (edx & CPUID_FEAT_EDX_SSE)
            kprintf(" SSE");
        if (edx & CPUID_FEAT_EDX_SSE2)
            kprintf(" SSE2");
        if (ecx & CPUID_FEAT_ECX_SSE3)
            kprintf(" SSE3");
        if (ecx & CPUID_FEAT_ECX_SSSE3)
            kprintf(" SSSE3");
        if (ecx & bit_SSE4_1)
            kprintf(" SSE41");
        if (ecx & bit_SSE4_2)
            kprintf(" SSE42");
        if (ecx & bit_AVX)
            kprintf(" AVX");
        if (ecx & bit_F16C)
            kprintf(" F16C");
        if (ecx & bit_RDRND)
            kprintf(" RDRAND");

        kprintf("\n");
    }

    // Extended Function 0x00 - Largest Extended Function

    uint32_t largestExtendedFunc;
    cpuid(0x80000000, &largestExtendedFunc, &ebx, &ecx, &edx);

    // Extended Function 0x01 - Extended Feature Bits

    if (largestExtendedFunc >= 0x80000001)
    {
        cpuid(0x80000001, &eax, &ebx, &ecx, &edx);

        if (edx & CPUID_FEAT_EDX_IA64)
        {
            kprintf("64-bit Architecture\n");
        }
    }

    // Extended Function 0x02-0x04 - Processor Name / Brand String
    if (largestExtendedFunc >= 0x80000004)
    {
        char name[48];
        cpuid(0x80000002, (uint32_t *)(name + 0), (uint32_t *)(name + 4), (uint32_t *)(name + 8), (uint32_t *)(name + 12));
        cpuid(0x80000003, (uint32_t *)(name + 16), (uint32_t *)(name + 20), (uint32_t *)(name + 24), (uint32_t *)(name + 28));
        cpuid(0x80000004, (uint32_t *)(name + 32), (uint32_t *)(name + 36), (uint32_t *)(name + 40), (uint32_t *)(name + 44));

        // Processor name is right justified with leading spaces
        const char *p = name;
        while (*p == ' ')
        {
            ++p;
        }

        kprintf("CPU Name: %s\n", p);
    }
}