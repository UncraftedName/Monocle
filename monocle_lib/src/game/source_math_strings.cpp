#include "source_math.hpp"

#include <format>
#include <vector>

namespace mon {

std::string Portal::NewLocationCmd(std::string_view portal_name, bool escape_quotes) const
{
    std::string_view quote_escape = escape_quotes ? "\\" : "";
    return std::format("ent_fire {}\"{}{}\" newlocation {}\"{} {}{}\"",
                       quote_escape,
                       portal_name,
                       quote_escape,
                       quote_escape,
                       pos.ToString(" "),
                       ang.ToString(" "),
                       quote_escape);
}

std::string Portal::DebugToString(std::string_view portal_name) const
{
    return std::format(
        "{}\n"
        "f: {}\n"
        "r: {}\n"
        "u: {}\n"
        "plane: {}\n"
        "mat:\n"
        "{}",
        NewLocationCmd(portal_name),
        f.ToString(),
        r.ToString(),
        u.ToString(),
        plane.ToString(),
        mat.DebugToString());
}

std::string PortalPair::NewLocationCmd(std::string_view delim, bool escape_quotes) const
{
    std::string blue_str = blue.NewLocationCmd("blue", escape_quotes);
    std::string orange_str = orange.NewLocationCmd("orange", escape_quotes);
    /*
    * orange UpdatePortalTeleportMatrix -> orange must be last
    * blue UpdatePortalTeleportMatrix -> blue must be last
    * UpdateLinkMatrix -> order agnostic
    */
    bool blue_first = order == PlacementOrder::_ORANGE_UPTM;
    return std::format("{}{}{}", blue_first ? blue_str : orange_str, delim, blue_first ? orange_str : blue_str);
}

std::string PortalPair::DebugToString() const
{
    bool blue_first = order == PlacementOrder::_ORANGE_UPTM;
    return std::format(
        "Placement order: {}\n"
        "{:->80}\n"
        "{}\n"
        "mat_to_linked:\n"
        "{}\n"
        "{:->80}\n"
        "{}\n"
        "mat_to_linked:\n"
        "{}",
        PlacementOrderStrs[(int)order],
        '-',
        blue_first ? blue.DebugToString("blue") : orange.DebugToString("orange"),
        blue_first ? b_to_o.DebugToString() : o_to_b.DebugToString(),
        '-',
        blue_first ? orange.DebugToString("orange") : blue.DebugToString("blue"),
        blue_first ? o_to_b.DebugToString() : b_to_o.DebugToString());
}

std::string Entity::SetPosCmd() const
{
    MON_ASSERT(is_player);
    return std::format("setpos {:.9g} {:.9g} {:.9g}", player.origin.x, player.origin.y, player.origin.z);
}

std::string Vector::ToString(std::string_view delim) const
{
    return std::format(MON_F_FMT "{}" MON_F_FMT "{}" MON_F_FMT, x, delim, y, delim, z, delim);
}

std::string VPlane::ToString() const
{
    return std::format("n: <{}>, d: " MON_F_FMT, n.ToString(), d);
}

static std::string DebugFmtMatrix(const float* mat, size_t rows, size_t cols)
{
    std::string buf;
    buf.reserve(15 * rows * cols);
    std::vector<size_t> sizes(rows * cols); // size of each formatted string
    size_t total_size = 0;

    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            auto res = std::format_to_n(std::back_inserter(buf), 100, MON_F_FMT, mat[r * cols + c]);
            sizes[r * cols + c] = res.size;
            total_size += res.size;
        }
    }

    std::vector<std::string_view> views(sizes.size());
    for (size_t i = 0, acc = 0; i < sizes.size(); acc += sizes[i++])
        views[i] = {buf.begin() + acc, i == sizes.size() - 1 ? buf.end() : buf.begin() + acc + sizes[i]};

    // re-use the sizes vec, now have it store the largest size in each column
    sizes.assign(cols, 0);
    for (size_t r = 0; r < rows; r++)
        for (size_t c = 0; c < cols; c++)
            sizes[c] = std::max(sizes[c], views[r * cols + c].size());

    std::string out_buf;
    out_buf.reserve(total_size + (cols - 1) * rows + rows - 1);
    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            std::string_view sv = views[r * cols + c];
            out_buf.append(sizes[c] - sv.size() + (c != 0), ' ');
            out_buf += sv;
        }
        out_buf += '\n';
    }
    return out_buf;
}

std::string matrix3x4_t::DebugToString() const
{
    return DebugFmtMatrix((float*)m_flMatVal, 3, 4);
}

std::string VMatrix::DebugToString() const
{
    return DebugFmtMatrix((float*)m, 4, 4);
}

std::optional<std::pair<Portal, std::from_chars_result>> Portal::FromString(std::string_view sv, GameVersion gv)
{
    std::array<float, 6> flts;

    const char* first = sv.data();
    const char* last = first + sv.size();
    std::from_chars_result res{};

    for (auto& flt : flts) {
        for (;;) {
            res = std::from_chars(first, last, flt);
            if (res.ec == std::errc{}) {
                first = res.ptr;
                break;
            } else if (first == last) {
                return {};
            } else {
                ++first;
            }
        }
    }

    return std::pair(Portal{{flts[0], flts[1], flts[2]}, {flts[3], flts[4], flts[5]}, gv}, res);
}

} // namespace mon
