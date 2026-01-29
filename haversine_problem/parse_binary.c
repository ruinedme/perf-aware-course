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

#define PROFILER 0

#include "common.h"
#include "profiler.c"
const f64 EARTH_RADIUS_KM = 6372.8;

typedef struct
{
    // f64 x0, y0, x1, y1;
    f64 values[4];
    unsigned seen;
} pair_t;

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

static f64 deg2rad(f64 degrees)
{
    f64 result = 0.017453292519943295 * degrees;
    return result;
}

// The Haversine function
f64 haversine_distance(pair_t *p, f64 R)
{
    START_SCOPE(_s, __func__);
    f64 x0 = p->values[0];
    f64 y0 = p->values[1];
    f64 x1 = p->values[2];
    f64 y1 = p->values[3];

    f64 dY = deg2rad(y1 - y0);
    f64 dX = deg2rad(x1 - x0);
    y0 = deg2rad(y0);
    y1 = deg2rad(y1);
    f64 rootTerm = (pow(sin(dY / 2.0), 2.0)) + cos(y0) * cos(y1) * (pow(sin(dX / 2.0), 2.0));
    f64 result = 2.0 * R * asin(sqrt(rootTerm));

    RETURN_VAL(_s, result);
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
    }
    f64 avg = acc_average(&acc);
    free(buf);
    end_and_print_profile();
    printf("Result %.16f\n", avg);
    return 0;
}