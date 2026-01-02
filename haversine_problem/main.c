#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>

#include "sax_json.h"
#define PI 3.141592

const double EARTH_RADIUS_KM = 6371.0;
#define ATOF_MULRECIP

static double fast_atof(const char *s, size_t len);

#ifdef ATOF_MULRECIP
#define MAX_FRAC 15
static const double inv_pow10[MAX_FRAC+1] = {
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

static double fast_atof(const char *s, size_t len){
    const char *p = s;
    const char *end = s + len;
    if (p == end) return 0.0;
    // sign
    int sign = 1;
    if(*p =='-'){
        sign = -1;
        ++p;
    }else if(*p == '+'){
        ++p;
    }

    // integer part
    uint64_t int_part = 0;
    while (p< end && *p >= '0' && *p <= '9'){
        int_part = int_part * 10 + (uint64_t)(*p -'0');
        ++p;
    }

    // fraction
    uint64_t frac_part = 0;
    int frac_digits = 0;
    int extra_digit = -1;
    if(p<end && *p == '.'){
        ++p;
        while (p < end && *p >= '0' && *p <= '9'){
            if(frac_digits < MAX_FRAC){
                frac_part = frac_part * 10 + (uint64_t)(*p - '0');
                ++frac_digits;
            }else if (frac_digits == MAX_FRAC){
                extra_digit = *p - '0';
                frac_digits++;
            }
            ++p;
        }
    }

    // rounding
    if(frac_digits > MAX_FRAC && extra_digit >= 0 && extra_digit >= 5){
        uint64_t denom = 1;
        for(int i=0;i<MAX_FRAC;++i){
            denom *= 10ULL;
        }
        frac_part += 1ULL;
        if(frac_part >= (uint64_t)denom){
            frac_part -= (uint64_t)denom;
            int_part += 1ULL;
        }
    }

    // build the value
    double value = (double)int_part;
    if(frac_digits > 0){
        int used_frac_digits = frac_digits > MAX_FRAC ? MAX_FRAC : frac_digits;
        value += (double)frac_part * inv_pow10[used_frac_digits];
    }
    
    return sign < 0 ? -value : value;
}
#endif


typedef struct
{
    double x0, y0, x1, y1;
    unsigned seen;
} pair_t;

typedef struct
{
    double sum;
    double c;
    size_t count;
} acc_t;

static double deg2rad(double degrees)
{
    double result = (degrees * PI) / 180;
    return result;
}

// The Haversine function
static double compute(pair_t *p, long R)
{
    double dY = deg2rad(p->y1 - p->y0);
    double dX = deg2rad(p->x1 - p->x0);
    double y0 = deg2rad(p->y0);
    double y1 = deg2rad(p->y1);
    double rootTerm = (pow(sin(dY / 2), 2)) + cos(y0) * cos(y1) * (pow(sin(dX / 2), 2));
    double result = 2 * R * asin(sqrt(rootTerm));

    return result;
}

static void acc_init(acc_t *a)
{
    a->sum = 0.0;
    a->c = 0.0;
    a->count = 0;
}

static void acc_add(acc_t *a, double value)
{
    // Kahan add
    double y = value - a->c;
    double t = a->sum + y;
    a->c = (t - a->sum) - y;
    a->sum = t;
    a->count++;
}

static double acc_average(const acc_t *a)
{
    if (a->count == 0)
        return 0.0;
    return a->sum / (double)a->count;
}

typedef struct {
    pair_t current;
    char last_key[16];
    acc_t acc;
} handler_ud_t;


void on_key(void *ud, const char *key){
    handler_ud_t *h = ud;
    strncpy(h->last_key, key, sizeof(h->last_key)-1);
    h->last_key[sizeof(h->last_key)-1] = '\0';
}

void on_number(void *ud, const char *num_text, size_t len){
    handler_ud_t *h = ud;
    // strtod
    // char *end = NULL;
    // double v = strtod(num_text, &end);
    // =====
    double v = fast_atof(num_text, len);

    if(strcmp(h->last_key, "x0") == 0){
        h->current.x0 =v;
        h->current.seen += 1;
    }else if(strcmp(h->last_key, "y0") == 0){
        h->current.y0 =v;
        h->current.seen += 2;
    }else if (strcmp(h->last_key, "x1") == 0){
        h->current.x1 =v;
        h->current.seen += 4;
    }else if (strcmp(h->last_key, "y1") == 0){
        h->current.y1 =v;
        h->current.seen += 8;
    }
}

void on_end_object(void *ud){
    handler_ud_t *h = ud;

    if (h->current.seen == 15){
        double val = compute(&h->current, EARTH_RADIUS_KM);
        acc_add(&h->acc, val);
    }

    // Blank current for next object
    memset(&h->current, 0, sizeof(h->current));
    h->last_key[0] = '\0';
}

void on_error(void *ud, const char *msg, size_t pos)
{
    (void)ud;
    fprintf(stderr, "Parser error at byte %zu: %s\n", pos, msg);
}



int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    const char *num = "-123.4567895";
    char *end = NULL;
    double n_strtod = strtod(num,&end);
    double n_atof = fast_atof(num, strlen(num));
    printf("checking: %s\n", num);
    printf("strtod: %.15f\n", n_strtod);
    printf("n_atof %.15f\n", n_atof);

    const char *path = argv[1];
    json_sax_handler_t h = {
        .error = on_error,
        .end_object = on_end_object,
        .number = on_number,
        .key = on_key
    };

    handler_ud_t ud;
    memset(&ud, 0, sizeof(ud));
    memset(&ud.current, 0, sizeof(pair_t));
    acc_init(&ud.acc);

    time_t startTime = time(NULL);
    if (!parse_file_with_sax(path, &h, &ud))
    {
        return EXIT_FAILURE;
    }

    double avg = acc_average(&ud.acc);
    time_t endTime = time(NULL);

    // === DISPLAY RESULT ===

    printf("Result %.6f\n", avg);
    printf("Throughput = %.4f haversines/second\n", (double)((double)ud.acc.count / (double)(endTime - startTime)));
    printf("Total = %ld seconds\n", endTime - startTime);

    return EXIT_SUCCESS;
}
