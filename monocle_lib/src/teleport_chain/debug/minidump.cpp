#include "teleport_chain/generate.hpp"

namespace mon {

std::ostream& TeleportChainResult::MiniDump(std::ostream& os, const TeleportChainParams& params) const
{
    auto required_flags = TCRF_RECORD_TP_DIRS | TCRF_RECORD_ENTITY;
    if ((params.record_flags & required_flags) != required_flags)
        return os << __FUNCTION__ << ": TCRF_RECORD_TP_DIRS | TCRF_RECORD_ENTITY is required";

    int min_cum = 0, max_cum = 0, cum = 0;
    for (bool dir : tp_dirs) {
        cum += dir ? 1 : -1;
        min_cum = cum < min_cum ? cum : min_cum;
        max_cum = cum > max_cum ? cum : max_cum;
    }
    int left_pad = ents.size() <= 1 ? 1 : (int)std::floor(std::log10(ents.size() - 1)) + 1;
    cum = 0;
    for (size_t i = 0; i <= tp_dirs.size(); i++) {
        os << std::format("{:{}d}) ", i, left_pad);
        for (int c = min_cum; c <= max_cum; c++) {

            bool should_teleport = false;
            if (c == 0 || c == 1) {
                const Portal& p = params.first_tp_from_blue == !!c ? params.pp->orange : params.pp->blue;
                should_teleport = p.ShouldTeleport(ents[i], false);
            }

            if (c == cum) {
                if (c == 0)
                    os << (should_teleport ? "0|>" : ".|0");
                else if (c == 1)
                    os << (should_teleport ? "<|1" : "1|.");
                else if (c < 0)
                    os << c << '.';
                else
                    os << '.' << c << '.';
            } else {
                if (c == 0)
                    os << ".|>";
                else if (c == 1)
                    os << "<|.";
                else
                    os << "...";
            }
        }
        os << '\n';
        if (i < tp_dirs.size())
            cum += tp_dirs[i] ? 1 : -1;
    }

    return os;
}

} // namespace mon
