#include <cmath>
#include <algorithm>

#include "vag_logic.hpp"

void NudgePointTowardsPortalPlane(const Vector& pt,
                                  const Portal& portal,
                                  Vector* nudged_to,
                                  IntVector* ulp_diff,
                                  bool nudge_behind)
{
    int nudge_axis;
    float biggest = -INFINITY;
    for (int i = 0; i < 3; i++) {
        if (fabsf(portal.plane.n[i]) > biggest) {
            biggest = fabsf(portal.plane.n[i]);
            nudge_axis = i;
        }
    }
    if (ulp_diff)
        (*ulp_diff)[0] = (*ulp_diff)[1] = (*ulp_diff)[2] = 0;

    Vector nudged = pt;
    for (int i = 0; i < 2; i++) {
        bool inv_check = !nudge_behind ^ i;
        float nudge_towards = pt[nudge_axis] + portal.plane.n[nudge_axis] * (inv_check ? -1.f : 1.f);
        int ulp_diff_incr = inv_check ? -1 : 1;

        while (portal.ShouldTeleport(nudged, false) ^ inv_check) {
            nudged[nudge_axis] = std::nextafterf(nudged[nudge_axis], nudge_towards);
            if (ulp_diff)
                (*ulp_diff)[nudge_axis] += ulp_diff_incr;
        }
    }
    if (nudged_to)
        *nudged_to = nudged;
}

void TryVag(const PortalPair& pair, const Vector& pt, bool tp_from_p1, TpInfo& info_out)
{
    auto& first_tp_p = tp_from_p1 ? pair.p1 : pair.p2;
    auto& second_tp_p = tp_from_p1 ? pair.p2 : pair.p1;

    NudgePointTowardsPortalPlane(pt, first_tp_p, &info_out.tp_from, nullptr, true);
    info_out.tp_to = pair.TeleportNonPlayerEntity(info_out.tp_from, tp_from_p1);
    NudgePointTowardsPortalPlane(info_out.tp_to, second_tp_p, nullptr, &info_out.ulps_from_exit, false);

    if (info_out.ulps_from_exit[0] < 0 || info_out.ulps_from_exit[1] < 0 || info_out.ulps_from_exit[2] < 0) {
        Vector pt_back_to_first = pair.TeleportNonPlayerEntity(info_out.tp_to, !tp_from_p1);
        info_out.result = first_tp_p.ShouldTeleport(pt_back_to_first, false) ? TpResult::RecursiveTp : TpResult::VAG;
    } else {
        info_out.result = TpResult::Nothing;
    }
}
