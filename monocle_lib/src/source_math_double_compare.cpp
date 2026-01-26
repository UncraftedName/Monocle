#include "source_math.hpp"
#include "source_math_double.hpp"
#include "vag_logic.hpp"
#include "ulp_diff.hpp"

#include <format>

namespace mon {

std::ostream& TeleportChainResult::CompareWithHighPrecisionChainToCsv(std::ostream& os,
                                                                      const TeleportChainParams& params) const
{
    auto required_flags = TCRF_RECORD_TP_DIRS | TCRF_RECORD_ENTITY;
    if ((params.record_flags & required_flags) != required_flags)
        return os << __FUNCTION__ << ": TCRF_RECORD_TP_DIRS | TCRF_RECORD_ENTITY is required";

    MON_ASSERT(!!params.pp);
    MON_ASSERT(ents.size() == tp_dirs.size() + 1);

    const PortalPair& pp = *params.pp;
    PortalPairD ppd{pp};

    os << "name,float_val,double_val,ulp_diff,abs_diff,rel_diff,is_blue\n";

    auto write_row = [&](const std::string& entry_name, float float_val, double double_val, bool blue) -> void {
        os << std::format("{}," MON_F_FMT "," MON_D_FMT "," MON_D_FMT "," MON_D_FMT "," MON_D_FMT
                          ",{}"
                          "\n",
                          entry_name,
                          float_val,
                          double_val,
                          ulp::UlpDiffD(double_val, float_val),
                          double_val - float_val,
                          (double_val == 0. && float_val == 0.f) ? 0. : (float_val - double_val) / double_val,
                          blue ? "true" : "false");
    };

    auto write_vec = [&](const std::string& prefix, const Vector& v, const VectorD& vd, bool blue) {
        const char* elem_chars = "xyz";
        for (size_t i = 0; i < 3; i++)
            write_row(prefix + "_" + elem_chars[i], v[i], vd[i], blue);
    };

    auto write_mat =
        [&](const std::string& prefix, const float* m, const double* md, size_t rows, size_t cols, bool blue) {
            for (size_t r = 0; r < rows; r++)
                for (size_t c = 0; c < cols; c++)
                    write_row(prefix + "_" + std::to_string(r * cols + c), m[r * cols + c], md[r * cols + c], blue);
        };

    // write portal vals

    for (int is_blue = 0; is_blue < 2; is_blue++) {
        std::string prefix = is_blue ? "blue_" : "orange_";
        const Portal& p = is_blue ? pp.blue : pp.orange;
        const PortalD& pd = is_blue ? ppd.blue : ppd.orange;

        write_vec(prefix + "pos", p.pos, pd.pos, is_blue);
        write_vec(prefix + "ang", *(Vector*)&p.ang, *(VectorD*)&pd.ang, is_blue);
        write_vec(prefix + "f", p.f, pd.f, is_blue);
        write_vec(prefix + "r", p.r, pd.r, is_blue);
        write_vec(prefix + "u", p.u, pd.u, is_blue);
        write_vec(prefix + "plane_n", p.plane.n, pd.plane.n, is_blue);
        write_row(prefix + "plane_d", p.plane.d, pd.plane.d, is_blue);
        write_mat(prefix + "mat", &p.mat[0][0], &pd.mat[0][0], 3, 4, is_blue);

        const VMatrix& vm = is_blue ? pp.b_to_o : pp.o_to_b;
        const VMatrixD& vmd = is_blue ? ppd.b_to_o : ppd.o_to_b;
        write_mat(prefix + "tp_mat", &vm[0][0], &vmd[0][0], 4, 4, is_blue);
    }

    // write chain vals

    for (size_t i = 0; i < tp_dirs.size(); i++) {
        bool is_blue = params.first_tp_from_blue == tp_dirs[i];
        EntityD ent_d_pre{ents[i]};
        write_vec(std::string("ent_pre_teleport_") + std::to_string(i),
                  ents[i].GetCenter(),
                  ent_d_pre.GetCenter(),
                  is_blue);

        EntityD ent_d_post = ppd.Teleport(ent_d_pre, is_blue);
        write_vec(std::string("ent_post_teleport_") + std::to_string(i),
                  ents[i + 1].GetCenter(),
                  ent_d_post.GetCenter(),
                  is_blue);
    }

    return os;
}

} // namespace mon
