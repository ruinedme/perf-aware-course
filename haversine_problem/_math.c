#if _MSC_VER
#include <intrin.h>
#else
#include <x86itrin.h>
#endif
#include <signal.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include "common.h"
// #include "profiler.c"

const static f64 PI = 3.14159265358979323846264338327950288419716939937510582097494459230781640628;
const static f64 Sqrt2 = 1.4142135623730951454746218587388284504413604736328125;
static const f64 PI_2 = 1.5707963267948965579989817342720925807952880859375;
// static const f64 TWO_PI = PI * 2.0;

typedef struct
{
    // f64 x0, y0, x1, y1;
    f64 values[4];
    unsigned seen;
} pair_t;

static f64 deg2rad(f64 degrees)
{
    f64 result = 0.017453292519943295 * degrees;
    return result;
}

static inline f64 Square(f64 x)
{
    return x * x;
}

static inline f64 sqrt_w(f64 x)
{
    __m128d xmm = _mm_set_sd(x);
    __m128d ans = _mm_sqrt_sd(xmm, xmm);
    double result = _mm_cvtsd_f64(ans);
    return result;
}

static f64 sin_w(f64 OrigX)
{
    f64 PosX = fabs(OrigX);
    f64 X = (PosX > PI_2) ? (PI - PosX) : PosX;
    
    f64 X2 = X*X;
    
    f64 R = 0x1.883c1c5deffbep-49;
    R = fma(R, X2, -0x1.ae43dc9bf8ba7p-41);
    R = fma(R, X2, 0x1.6123ce513b09fp-33);
    R = fma(R, X2, -0x1.ae6454d960ac4p-26);
    R = fma(R, X2, 0x1.71de3a52aab96p-19);
    R = fma(R, X2, -0x1.a01a01a014eb6p-13);
    R = fma(R, X2, 0x1.11111111110c9p-7);
    R = fma(R, X2, -0x1.5555555555555p-3);
    R = fma(R, X2, 0x1p0);
    R *= X;
    
    f64 Result = (OrigX < 0) ? -R : R;
    
    return Result;
}

static f64 cos_w(f64 x)
{
    f64 r = sin_w(x + PI_2);
    return r;
    // return cos(x);
}

static f64 asin_w(f64 OrigX)
{
    bool needsTransform = (OrigX > 0.7071067811865475244); // 1/sqrt(2)
    f64 X = needsTransform ? sqrt_w(1.0 - OrigX * OrigX) : OrigX;
    f64 X2 = Square(X);
    
    f64 R = 0x1.dfc53682725cap-1;
    R = fma(R, X2, -0x1.bec6daf74ed61p1);
    R = fma(R, X2, 0x1.8bf4dadaf548cp2);
    R = fma(R, X2, -0x1.b06f523e74f33p2);
    R = fma(R, X2, 0x1.4537ddde2d76dp2);
    R = fma(R, X2, -0x1.6067d334b4792p1);
    R = fma(R, X2, 0x1.1fb54da575b22p0);
    R = fma(R, X2, -0x1.57380bcd2890ep-2);
    R = fma(R, X2, 0x1.69b370aad086ep-4);
    R = fma(R, X2, -0x1.21438ccc95d62p-8);
    R = fma(R, X2, 0x1.b8a33b8e380efp-7);
    R = fma(R, X2, 0x1.c37061f4e5f55p-7);
    R = fma(R, X2, 0x1.1c875d6c5323dp-6);
    R = fma(R, X2, 0x1.6e88ce94d1149p-6);
    R = fma(R, X2, 0x1.f1c73443a02f5p-6);
    R = fma(R, X2, 0x1.6db6db3184756p-5);
    R = fma(R, X2, 0x1.3333333380df2p-4);
    R = fma(R, X2, 0x1.555555555531ep-3);
    R = fma(R, X2, 0x1p0);
    R *= X;

    f64 Result = needsTransform ? (PI_2 - R) : R;
    return Result;
    // return asin(x);
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

    // f64 rootTerm = (pow(sin(dY / 2.0), 2.0)) + cos(y0) * cos(y1) * (pow(sin(dX / 2.0), 2.0));
    f64 rootTerm = Square(sin_w(dY * 0.5)) + cos_w(y0) * cos_w(y1) * (Square(sin_w(dX * 0.5)));
    f64 result = 2.0 * R * asin_w(sqrt_w(rootTerm));

    RETURN_VAL(_s, result);
}
