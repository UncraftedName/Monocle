#pragma once

#include <cmath>
#include <stdint.h>

// pulled from https://www.emmtrix.com/wiki/ULP_Difference_of_Float_Numbers

namespace mon::ulp {

inline float UlpSizeF(float f)
{
    f = std::fabsf(f);
    return std::nextafterf(f, INFINITY) - f;
}

// difference between two floats in ulps
inline uint32_t UlpDiffF(float f1, float f2)
{
    if (!std::isfinite(f1) || !std::isfinite(f2))
        return UINT32_MAX;
    if (std::signbit(f1) != std::signbit(f2))
        return UlpDiffF(0.f, std::fabsf(f1)) + UlpDiffF(0.f, std::fabsf(f2)) + 1;
    uint32_t i1 = *(uint32_t*)&f1;
    uint32_t i2 = *(uint32_t*)&f2;
    return i1 > i2 ? i1 - i2 : i2 - i1;
}

inline float DoubleToFloatRoundDown(double value)
{
    float f = (float)value;
    return f > value ? std::nextafterf(f, -INFINITY) : f;
}

// fractional difference between double & float
inline double UlpDiffD(double ref, float f)
{
    if (!std::isfinite(ref) || !std::isfinite(f))
        return INFINITY;

    float fref = (float)ref;
    double int_part = (double)UlpDiffF(fref, f);

    // make everything positive
    ref = std::abs(ref);
    fref = std::fabsf(fref);
    f = std::fabsf(f);

    float ulp_ref = UlpSizeF(DoubleToFloatRoundDown(ref));

    // calculate fractional part
    double diff = ref - (double)fref;
    double frac_part = diff / ulp_ref;
    if (f > ref)
        frac_part = -frac_part;

    return int_part + frac_part;
}

} // namespace mon::ulp
