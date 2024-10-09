#include <cmath>
#include <algorithm>

#include "vag_logic.hpp"

void NudgePointTowardsPortalPlane(const Vector& pt,
                                  const Portal& portal,
                                  bool nudge_behind,
                                  Vector* nudged_to,
                                  VecUlpDiff* ulp_diff)
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
        ulp_diff->Reset();

    Vector nudged = pt;
    for (int i = 0; i < 2; i++) {
        bool inv_check = !nudge_behind ^ i;
        float nudge_towards = pt[nudge_axis] + portal.plane.n[nudge_axis] * (inv_check ? -1.f : 1.f);
        int ulp_diff_incr = inv_check ? -1 : 1;

        while (portal.ShouldTeleport(nudged, false) ^ inv_check) {
            nudged[nudge_axis] = std::nextafterf(nudged[nudge_axis], nudge_towards);
            if (ulp_diff)
                ulp_diff->Update(nudge_axis, ulp_diff_incr);
        }
    }
    if (nudged_to)
        *nudged_to = nudged;
}

void TryVag(const PortalPair& pair, const Vector& pt, bool tp_from_blue, TpInfo& info_out)
{
    auto& p1 = tp_from_blue ? pair.blue : pair.orange;
    auto& p2 = tp_from_blue ? pair.orange : pair.blue;

    NudgePointTowardsPortalPlane(pt, p1, true, &info_out.tp_from, &info_out.ulps_for_init_pt);
    info_out.tp_to = pair.TeleportNonPlayerEntity(info_out.tp_from, tp_from_blue);
    NudgePointTowardsPortalPlane(info_out.tp_to, p2, false, nullptr, &info_out.ulps_from_exit);

    if (p2.ShouldTeleport(info_out.tp_to, false)) {
        Vector pt_back_to_first = pair.TeleportNonPlayerEntity(info_out.tp_to, !tp_from_blue);
        if (p1.ShouldTeleport(pt_back_to_first, false)) {
            info_out.result = TpResult::RecursiveTp;
        } else {
            info_out.result = TpResult::VAG;
            info_out.tp_to_final = pair.TeleportNonPlayerEntity(pt_back_to_first, !tp_from_blue);
        }
    } else {
        info_out.result = TpResult::Nothing;
    }
}
