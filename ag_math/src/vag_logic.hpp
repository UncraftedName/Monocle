#pragma once

#include <stdint.h>
#include <array>
#include "source_math.hpp"

struct PortalCache {
    const Portal p;

    PortalCache(const Portal& p) : p{p}, calcedVectors{0}, calcedPlane{0}, calcedMatrix{0} {}

    const Vector& GetF()
    {
        if (!calcedVectors) {
            p.CalcVectors(&f, &r, &u);
            calcedVectors = 1;
        }
        return f;
    }

    const Vector& GetR()
    {
        if (!calcedVectors) {
            p.CalcVectors(&f, &r, &u);
            calcedVectors = 1;
        }
        return r;
    }

    const Vector& GetU()
    {
        if (!calcedVectors) {
            p.CalcVectors(&f, &r, &u);
            calcedVectors = 1;
        }
        return u;
    }

    const VPlane& GetPlane()
    {
        if (!calcedPlane) {
            p.CalcPlane(GetF(), plane);
            calcedPlane = 1;
        }
        return plane;
    }

    const matrix3x4_t& GetMat()
    {
        if (!calcedMatrix) {
            p.CalcMatrix(m);
            calcedMatrix = 1;
        }
        return m;
    }

private:
    Vector f, r, u;
    VPlane plane;
    matrix3x4_t m;
    uint32_t calcedVectors : 1;
    uint32_t calcedPlane : 1;
    uint32_t calcedMatrix : 1;
};

struct PortalPairCache {
    PortalCache p1, p2;

    PortalPairCache(const Vector& p1, const QAngle& q1, const Vector& p2, const QAngle& q2)
        : p1{{p1, q1}}, p2{{p2, q2}}, calced_p1_to_p2{0}, calced_p2_to_p1{0}
    {}

    const VMatrix& GetTpMatrix(bool p1_teleporting)
    {
        if (p1_teleporting) {
            if (!calced_p1_to_p2) {
                PortalPair::CalcTeleportMatrix(p1.GetMat(), p2.GetMat(), p1_to_p2, true);
                calced_p1_to_p2 = 1;
            }
            return p1_to_p2;
        } else {
            if (!calced_p2_to_p1) {
                PortalPair::CalcTeleportMatrix(p1.GetMat(), p2.GetMat(), p2_to_p1, false);
                calced_p2_to_p1 = 1;
            }
            return p2_to_p1;
        }
    }

private:
    VMatrix p1_to_p2, p2_to_p1;
    uint32_t calced_p1_to_p2 : 1;
    uint32_t calced_p2_to_p1 : 1;
};

enum class TpResult {
    Nothing,     // normal teleport
    VAG,         // would trigger VAG
    RecursiveTp, // recursive teleport (usually results in no free edicts crash)
};

using IntVector = std::array<int, 3>;

struct TpInfo {
    PortalPair pp;
    Vector tp_from, tp_to;
    IntVector ulps_from_entry, ulps_from_exit;
    TpResult result;
    bool tp_from_p1;
};

/*
* Calculates how many ulp nudges were needed to move the given point until it's past the portal
* plane (either behind or in front). ulp_diff is measured relative to the direction of the portal
* normal (towards portal normal is positive). If nudge_behind, will move the point until it's
* behind the portal plane as determined by Portal::ShouldTeleport(), otherwise will move in front.
*/
void NudgePointTowardsPortalPlane(const Vector& pt,
                                  const VPlane& portal_plane,
                                  Vector* nudged_to,
                                  IntVector* ulp_diff,
                                  bool nudge_behind);

void TryVag(PortalPairCache& cache, const Vector& pt, bool tp_from_p1, TpInfo& info_out);
