#pragma once

#include "source_math.hpp"
#include "source_math_double.hpp"
#include "vag_logic.hpp"
#include "ulp_diff.hpp"

#include <format>

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

/*
* For each teleport in the given chain, creates a double precision entity and applies the same
* teleport. The float entity, double entity, and difference between them is written to a string
* which can be exported as a CSV file.
*/
inline std::string TeleportChainCompareToCsv(const TeleportChainParams& params, const TeleportChainResult& result)
{
    MON_ASSERT(!!params.pp);
    MON_ASSERT(params.record_flags & (TCRF_RECORD_ENTITY | TCRF_RECORD_TP_DIRS));
    MON_ASSERT(result.ents.size() == result.tp_dirs.size() + 1);

    const PortalPair& pp = *params.pp;
    PortalPairD ppd{pp};

    std::string res = "name,float,double,ulp_diff,abs_diff,rel_diff\n";
    auto it = std::back_inserter(res);

    auto write_row = [&it](const std::string& entry_name, float float_val, double double_val) -> void {
        it = std::format_to(it,
                            "{}," MON_F_FMT "," MON_D_FMT ",{:.2f}," MON_D_FMT "," MON_D_FMT "\n",
                            entry_name,
                            float_val,
                            double_val,
                            UlpDiffD(double_val, float_val),
                            double_val - float_val,
                            (double_val == 0. && float_val == 0.f) ? 0. : (float_val - double_val) / double_val);
    };

    auto write_vec = [&](const std::string& prefix, const Vector& v, const VectorD& vd) {
        const char* elem_chars = "xyz";
        for (size_t i = 0; i < 3; i++)
            write_row(prefix + "_" + elem_chars[i], v[i], vd[i]);
    };

    auto write_mat = [&](const std::string& prefix, const float* m, const double* md, size_t rows, size_t cols) {
        for (size_t r = 0; r < rows; r++)
            for (size_t c = 0; c < cols; c++)
                write_row(prefix + "_" + std::to_string(r * cols + c), m[r * cols + c], md[r * cols + c]);
    };

    // write portal vals

    for (int is_blue = 0; is_blue < 2; is_blue++) {
        std::string prefix = is_blue ? "blue_" : "orange_";
        const Portal& p = is_blue ? pp.blue : pp.orange;
        const PortalD& pd = is_blue ? ppd.blue : ppd.orange;

        write_vec(prefix + "pos", p.pos, pd.pos);
        write_vec(prefix + "ang", *(Vector*)&p.ang, *(VectorD*)&pd.ang);
        write_vec(prefix + "f", p.f, pd.f);
        write_vec(prefix + "r", p.r, pd.r);
        write_vec(prefix + "u", p.u, pd.u);
        write_vec(prefix + "plane_n", p.plane.n, pd.plane.n);
        write_row(prefix + "plane_d", p.plane.d, pd.plane.d);
        write_mat(prefix + "mat", &p.mat[0][0], &pd.mat[0][0], 3, 4);

        const VMatrix& vm = is_blue ? pp.o_to_b : pp.b_to_o;
        const VMatrixD& vmd = is_blue ? ppd.o_to_b : ppd.b_to_o;
        write_mat(prefix + "tp_mat", &vm[0][0], &vmd[0][0], 4, 4);
    }

    // write chain vals

    for (size_t i = 0; i < result.tp_dirs.size(); i++) {
        EntityD ent_d_pre{result.ents[i]};
        write_vec(std::string("ent_pre_teleport_") + std::to_string(i),
                  result.ents[i].GetCenter(),
                  ent_d_pre.GetCenter());

        EntityD ent_d_post = ppd.Teleport(ent_d_pre, params.first_tp_from_blue == result.tp_dirs[i]);
        write_vec(std::string("ent_post_teleport_") + std::to_string(i),
                  result.ents[i + 1].GetCenter(),
                  ent_d_post.GetCenter());
    }

    return res;
}

} // namespace mon::ulp
