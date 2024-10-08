#pragma once

#include <stdint.h>
#include <array>
#include "source_math.hpp"

enum class TpResult {
    Nothing,     // normal teleport
    VAG,         // would trigger VAG
    RecursiveTp, // recursive teleport (usually results in no free edicts crash)

    COUNT,
};

/*
* Represents a difference between two Vectors in ULPs. All VAG related code only nudges points
* along a single axis, hence the abstraction. For VAG code positive ulps means in the same
* direction as the portal normal.
*/
struct VecUlpDiff {
    int ax;   // [0, 2] - the axis along which the difference is measured, -1 if diff == 0
    int diff; // the difference in ulps

    void Reset()
    {
        ax = -1;
        diff = 0;
    }

    void Update(int along, int by)
    {
        assert(ax == -1 || ax == along);
        ax = along;
        diff += by;
        if (diff == 0)
            ax = -1;
    }
};

struct TpInfo {
    Vector tp_from, tp_to;
    VecUlpDiff ulps_for_init_pt, ulps_from_exit;
    TpResult result;
};

/*
* Calculates how many ulp nudges were needed to move the given point until it's past the portal
* plane (either behind or in front). If nudge_behind, will move the point until it's behind the
* portal plane as determined by Portal::ShouldTeleport(), otherwise will move in front. Finds the
* point which is closest to the portal plane.
*/
void NudgePointTowardsPortalPlane(const Vector& pt,
                                  const Portal& portal,
                                  bool nudge_behind,
                                  Vector* nudged_to,
                                  VecUlpDiff* ulp_diff);

void TryVag(const PortalPair& pair, const Vector& pt, bool tp_from_blue, TpInfo& info_out);
