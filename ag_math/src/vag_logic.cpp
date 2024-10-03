#include <cmath>
#include <algorithm>

#include "vag_logic.hpp"

void NudgePointTowardsPortalPlane(const Vector& pt,
                                  const VPlane& portal_plane,
                                  Vector* nudged_to,
                                  IntVector* ulp_diff,
                                  bool nudge_behind)
{
    int nudge_axis;
    float biggest = -INFINITY;
    for (int i = 0; i < 3; i++) {
        if (fabsf(portal_plane.n[i]) > biggest) {
            biggest = fabsf(portal_plane.n[i]);
            nudge_axis = i;
        }
    }

    float towards_front = pt[nudge_axis] + portal_plane.n[nudge_axis];
    float towards_behind = pt[nudge_axis] - portal_plane.n[nudge_axis];

    Vector nudged = pt;
    for (int i = 0; i < 2; i++) {
        bool inv_check = !nudge_behind ^ i;
        float nudge_towards = inv_check ? towards_behind : towards_front;
        int ulp_diff_incr = inv_check ? -1 : 1;

        while (Portal::ShouldTeleport(portal_plane, nudged, false) ^ inv_check) {
            nudged[nudge_axis] = std::nextafterf(nudged[nudge_axis], nudge_towards);
            if (ulp_diff)
                (*ulp_diff)[nudge_axis] += ulp_diff_incr;
        }
    }
    if (nudged_to)
        *nudged_to = nudged;
}

void TryVag(PortalPairCache& cache, const Vector& pt, bool tp_from_p1, TpInfo& info_out)
{
    info_out.pp = {cache.p1.p, cache.p2.p};
    info_out.tp_from_p1 = tp_from_p1;
    std::fill(info_out.ulps_from_entry.begin(), info_out.ulps_from_entry.end(), 0);

    auto& cache_tp_1 = tp_from_p1 ? cache.p1 : cache.p2;
    auto& cache_tp_2 = tp_from_p1 ? cache.p2 : cache.p1;

    NudgePointTowardsPortalPlane(pt, cache_tp_1.GetPlane(), &info_out.tp_from, nullptr, true);
    info_out.tp_to = Portal::TeleportNonPlayerEntity(cache.GetTpMatrix(tp_from_p1), info_out.tp_from);
    NudgePointTowardsPortalPlane(info_out.tp_to, cache_tp_2.GetPlane(), nullptr, &info_out.ulps_from_exit, false);

    if (info_out.ulps_from_exit[0] < 0 || info_out.ulps_from_exit[1] < 0 || info_out.ulps_from_exit[2] < 0) {
        Vector pt_back_to_first = Portal::TeleportNonPlayerEntity(cache.GetTpMatrix(!tp_from_p1), info_out.tp_to);
        info_out.result = Portal::ShouldTeleport(cache_tp_1.GetPlane(), pt_back_to_first, false) ? TpResult::RecursiveTp
                                                                                                 : TpResult::VAG;
    } else {
        info_out.result = TpResult::Nothing;
    }
}
