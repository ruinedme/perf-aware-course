#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>

#define PROFILER 1

#include "common.h"
#include "profiler.c"
#include "_math.c"
const f64 EARTH_RADIUS_KM = 6372.8;

// typedef struct
// {
//     // f64 x0, y0, x1, y1;
//     f64 values[4];
//     unsigned seen;
// } pair_t;

typedef struct
{
    f64 sum;
    f64 c;
    size_t count;
} acc_t;

static void acc_init(acc_t *a)
{
    a->sum = 0.0;
    a->c = 0.0;
    a->count = 0;
}

static void acc_add(acc_t *a, f64 value)
{
    // Kahan add
    f64 y = value - a->c;
    f64 t = a->sum + y;
    a->c = (t - a->sum) - y;
    a->sum = t;
    a->count++;
}

static f64 acc_average(const acc_t *a)
{
    if (a->count == 0)
        return 0.0;
    return a->sum / (f64)a->count;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }
    begin_profile();

    const char *path = argv[1];
    FILE *f = NULL;
    f = fopen(path, "rb");
    if (!f)
    {
        perror("fopen");
        return 1;
    }

    size_t file_size = 320000000; // 40 million doubles
    size_t count = file_size / sizeof(f64);
    f64 *buf = (f64*)malloc(file_size);
    if(!buf){
        perror("malloc");
        return 1;
    }
    
    size_t n = fread(buf,sizeof(f64), count,f);
    if (n != count){
        perror("fread");
        return 1;
    }
    fclose(f);

    acc_t acc;
    acc_init(&acc);

    size_t offset = 0;
    f64 a = 0.0;
    while(offset < count){
        f64 x0 = buf[offset++];
        f64 y0 = buf[offset++];
        f64 x1 = buf[offset++];
        f64 y1 = buf[offset++];
        pair_t pair = {
            .seen = 0,
            .values = {x0,y0,x1,y1}
        };
        f64 value = haversine_distance(&pair, EARTH_RADIUS_KM);
        acc_add(&acc, value);
        a += value;
    }
    f64 avg = acc_average(&acc);
    free(buf);
    end_and_print_profile();
    printf("Result acc %.16f\n", avg);
    printf("Result   a %.16f\n", a / acc.count);
    return 0;
}