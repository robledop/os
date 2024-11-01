#pragma once

// #include <stdint.h>
// #include <stddef.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long long size_t;
typedef unsigned int uintptr_t;
typedef unsigned char uchar;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
#define bool _Bool
#define NULL ((void *)0)
#define true 1
#define false 0

#define offsetof(TYPE, MEMBER) ((uint32_t) &((TYPE *) 0)->MEMBER)
