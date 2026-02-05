#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <time.h>
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>

#define PROFILER 1

#include "common.h"
#include "profiler.c"
#include "_math.c"
#include "sax_json.c"

#define MAX_FRAC 15
const f64 EARTH_RADIUS_KM = 6372.8;

// static f64 fast_atof(const char *s, size_t len);
static const f64 inv_pow10[MAX_FRAC + 1] = {
    1.0,
    0.1,
    0.01,
    0.001,
    0.0001,
    0.00001,
    0.000001,
    0.0000001,
    0.00000001,
    0.000000001,
    0.0000000001,
    0.00000000001,
    0.000000000001,
    0.0000000000001,
    0.00000000000001,
    0.000000000000001,
};

static f64 fast_atof(const char *s, size_t len)
{
    TIME_FUNCTION(_s);
    const char *p = s;
    const char *end = s + len;
    if (p == end)
        return 0.0;
    // sign
    int sign = 1;
    if (*p == '-')
    {
        sign = -1;
        ++p;
    }
    else if (*p == '+')
    {
        ++p;
    }

    // integer part
    uint64_t int_part = 0;
    while (p < end && *p >= '0' && *p <= '9')
    {
        int_part = int_part * 10 + (uint64_t)(*p - '0');
        ++p;
    }

    // fraction
    uint64_t frac_part = 0;
    int frac_digits = 0;
    int extra_digit = -1;
    if (p < end && *p == '.')
    {
        ++p;
        while (p < end && *p >= '0' && *p <= '9')
        {
            if (frac_digits < MAX_FRAC)
            {
                frac_part = frac_part * 10 + (uint64_t)(*p - '0');
                ++frac_digits;
            }
            else if (frac_digits == MAX_FRAC)
            {
                extra_digit = *p - '0';
                frac_digits++;
            }
            ++p;
        }
    }

    // rounding
    if (frac_digits > MAX_FRAC && extra_digit >= 0 && extra_digit >= 5)
    {
        uint64_t denom = 1;
        for (int i = 0; i < MAX_FRAC; ++i)
        {
            denom *= 10ULL;
        }
        frac_part += 1ULL;
        if (frac_part >= (uint64_t)denom)
        {
            frac_part -= (uint64_t)denom;
            int_part += 1ULL;
        }
    }

    // build the value
    f64 value = (f64)int_part;
    if (frac_digits > 0)
    {
        int used_frac_digits = frac_digits > MAX_FRAC ? MAX_FRAC : frac_digits;
        value += (f64)frac_part * inv_pow10[used_frac_digits];
    }

    f64 result = sign < 0 ? -value : value;
    RETURN_VAL(_s, result);
    // return result;
}

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

typedef struct
{
    pair_t current;
    acc_t acc;
} handler_ud_t;

void on_number(void *ud, const char *num_text, size_t len)
{
    TIME_FUNCTION(_s);
    handler_ud_t *h = ud;
    f64 v = fast_atof(num_text, len);
    h->current.values[h->current.seen++] = v;
    END_SCOPE(_s);
}

void on_end_object(void *ud)
{
    TIME_BANDWIDTH(_s, __func__, sizeof(pair_t) + sizeof(f64));
    handler_ud_t *h = ud;

    if (h->current.seen > 0)
    {
        f64 val = haversine_distance(&h->current, EARTH_RADIUS_KM);
        acc_add(&h->acc, val);
        h->current.seen = 0;
    }

    // Blank current for next object
    // memset(&h->current, 0, sizeof(h->current));
    RETURN_VOID(_s);
}

void on_error(void *ud, const char *msg, size_t pos)
{
    (void)ud;
    fprintf(stderr, "Parser error at byte %zu: %s\n", pos, msg);
}

int main(int argc, char *argv[])
{
    begin_profile();
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    json_sax_handler_t h = {
        .error = on_error,
        .end_object = on_end_object,
        .number = on_number};

    handler_ud_t ud;
    memset(&ud, 0, sizeof(ud));
    memset(&ud.current, 0, sizeof(pair_t));
    acc_init(&ud.acc);

    if (!parse_file_with_sax(path, &h, &ud))
    {
        return EXIT_FAILURE;
    }

    f64 avg = acc_average(&ud.acc);

    // === DISPLAY RESULT ===
    end_and_print_profile();
    printf("Result %.16f\n", avg);
    return EXIT_SUCCESS;
}
