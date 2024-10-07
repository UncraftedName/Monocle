#pragma once

#include <stdint.h>
#include <array>
#include "source_math.hpp"

enum class TpResult {
    Nothing,     // normal teleport
    VAG,         // would trigger VAG
    RecursiveTp, // recursive teleport (usually results in no free edicts crash)
};

using IntVector = std::array<int, 3>;

struct TpInfo {
    Vector tp_from, tp_to;
    IntVector ulps_from_exit;
    TpResult result;
};

/*
* Calculates how many ulp nudges were needed to move the given point until it's past the portal
* plane (either behind or in front). ulp_diff is measured relative to the direction of the portal
* normal (towards portal normal is positive). If nudge_behind, will move the point until it's
* behind the portal plane as determined by Portal::ShouldTeleport(), otherwise will move in front.
*/
void NudgePointTowardsPortalPlane(const Vector& pt,
                                  const Portal& portal,
                                  Vector* nudged_to,
                                  IntVector* ulp_diff,
                                  bool nudge_behind);

void TryVag(const PortalPair& pair, const Vector& pt, bool tp_from_blue, TpInfo& info_out);
