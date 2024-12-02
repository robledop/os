#include <rand.h>
#include <stddef.h>
#include <x86.h>

#define ENTROPY_POOL_SIZE 64
static uint32_t entropy_pool[ENTROPY_POOL_SIZE];
static uint32_t entropy_pool_index = 0;

// https://wiki.osdev.org/Random_Number_Generator
static unsigned long int next = 1; // NB: "unsigned long int" is assumed to be 32 bits wide

uint32_t rand(void) // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed)
{
    next = seed;
}

uint32_t rdrand()
{
    uint32_t value;
    asm __volatile("rdrand %0" : "=r"(value));
    return value;
}

void add_entropy(const uint8_t byte)
{
    entropy_pool[entropy_pool_index] ^= byte;
    entropy_pool_index = (entropy_pool_index + 1) % ENTROPY_POOL_SIZE;
}

uint32_t get_random(void)
{
    add_entropy(rdtsc());

    uint32_t hash = 0;
    for (size_t i = 0; i < ENTROPY_POOL_SIZE; i++) {
        hash ^= entropy_pool[i];
    }

    return hash;
}