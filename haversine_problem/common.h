#ifndef COMMON_H
#define COMMON_H
#include <stdbool.h>
#include <inttypes.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef double f64;
typedef unsigned char u8;

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#endif