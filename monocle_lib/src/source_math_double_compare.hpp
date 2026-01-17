#pragma once

#include "source_math.hpp"
#include "source_math_double.hpp"
#include "ulp_diff.hpp"

namespace mon::ulp {

// compares a struct of floats and a struct of doubles by extracting the fractional ulp diff element-wise
template <typename FT, typename DT>
inline DT UlpStructDiffD(const FT& f, const DT& d)
{
    static_assert(sizeof(FT) / sizeof(float) == sizeof(DT) / sizeof(double));
    DT d_out;
    for (size_t i = 0; i < sizeof(FT) / sizeof(float); i++)
        ((double*)&d_out)[i] = UlpDiffD(((float*)&f)[i], ((double*)&d)[i]);
    return d_out;
}

} // namespace mon::ulp
