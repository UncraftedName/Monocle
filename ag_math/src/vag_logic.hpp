#pragma once

#include <stdint.h>
#include <vector>
#include <deque>

#include "source_math.hpp"

/*
* Represents a difference between two Vectors in ULPs. All VAG related code only nudges points
* along a single axis, hence the abstraction. For VAG code positive ulps means in the same
* direction as the portal normal.
*/
struct VecUlpDiff {
    int ax;   // [0, 2] - the axis along which the difference is measured
    int diff; // the difference in ulps

    void Reset()
    {
        ax = -1;
        diff = 0;
    }

    void Update(int along, int by)
    {
        assert(ax == -1 || along == ax || by == 0);
        ax = along;
        diff += by;
    }

    void SetInvalid()
    {
        ax = -666;
    }

    bool Valid() const
    {
        return ax != -666;
    }

    bool PtWasBehindPlane() const
    {
        assert(Valid());
        return diff >= 0;
    }
};

#define CUM_TP_NORMAL_TELEPORT 1
#define CUM_TP_VAG (-1)

struct TpChain {

    // the points to/from which the teleports happen, has n_teleports + 1 elems
    std::vector<Vector> pts;
    // ulps from each pt to behind the portal; only applicable if the point is near a portal,
    // otherwise will have ax=ULP_DIFF_TOO_LARGE_AX; has n_teleports + 1 elems
    std::vector<VecUlpDiff> ulp_diffs;
    // direction of each teleport (true for the primary portal), has n_teleports elems
    std::vector<bool> tp_dirs;
    // at each point, the number of teleports queued by a portal; 1 or 2 for the primary portal and
    // -1 or -2 for the secondary; has n_teleports + 1 elems
    std::vector<int> tps_queued;
    // (number of teleports done by primary portal) - (number done by secondary portal), 1 for normal tp, -1 for VAG
    int cum_primary_tps;
    // if true, the chain has more teleports (but all the fields are accurate in the chain so far)
    bool max_tps_exceeded;

    // internal teleport queue
    std::deque<int> _tp_queue;

    void Clear()
    {
        pts.clear();
        ulp_diffs.clear();
        tp_dirs.clear();
        tps_queued.clear();
        cum_primary_tps = 0;
        max_tps_exceeded = false;
        _tp_queue.clear();
    }
};

/*
* Calculates how many ulp nudges were needed to move the given entity until it's past the portal
* plane (either behind or in front). If nudge_behind, will move the center until it's behind the
* portal plane as determined by Portal::ShouldTeleport(), otherwise will move in front. Finds the
* point which is closest to the portal plane.
*/
void NudgeEntityBehindPortalPlane(Entity& ent, const Portal& portal, bool change_ent_pos, VecUlpDiff* ulp_diff);

#define N_CHILDREN_PLAYER_WITHOUT_PORTAL_GUN 1
#define N_CHILDREN_PLAYER_WITH_PORTAL_GUN 2

struct EntityInfo {
    size_t n_ent_children;
    bool set_ent_pos_through_chain;
    bool origin_inbounds;
};

/*
* Generates the chain of teleports that is produced within a single tick when an entity teleports.
* This is the function you use to determine if a teleport will turn into a VAG or something more
* complicated.
* 
* The entity is expected to be very close to the portal it is being teleported from, and it will
* automatically be nudged until it is just barely behind the portal. Putting the entity too far
* away will cause the nudge to take an extremely long time.
* 
* The number of children the entities has is only relevant for chains more complicated than a VAG.
* The player normally has 2 (portal gun & view model).
*/
void GenerateTeleportChain(TpChain& chain,
                           const PortalPair& pair,
                           bool tp_from_blue,
                           Entity& ent,
                           EntityInfo ent_info,
                           size_t n_max_teleports);
