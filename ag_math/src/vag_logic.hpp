#pragma once

#include <stdint.h>
#include <vector>
#include <deque>

#include "source_math.hpp"

#ifdef CHAIN_WITH_GRAPHVIZ
#include "chain_graphviz.h"
#endif

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

/*
* Calculates how many ulp nudges were needed to move the given entity until it's behind the portal
* plane as determined by Portal::ShouldTeleport(). If the point is already behind, nudges are done
* in the direction of the portal normal so long as ShouldTeleport returns true.
*/
Entity NudgeEntityBehindPortalPlane(const Entity& ent, const Portal& portal, VecUlpDiff* ulp_diff);

#define CUM_TP_NORMAL_TELEPORT 1
#define CUM_TP_VAG (-1)

#define N_CHILDREN_PLAYER_WITHOUT_PORTAL_GUN 1
#define N_CHILDREN_PLAYER_WITH_PORTAL_GUN 2

struct EntityInfo {
    // how many children does this entity have? N_CHILDREN_PLAYER_WITH_PORTAL_GUN is usually the case for the player, otherwise 0
    short n_ent_children;
    // is the origin of the map inbounds?
    bool origin_inbounds;
};

class TeleportChain {

public:
    // the center of the entity to/from which the teleports happen, has n_teleports + 1 elems
    std::vector<Vector> pts;
    // ulps from each pt to behind the portal; only applicable if the point is near a portal,
    // otherwise will have ax=ULP_DIFF_TOO_LARGE_AX; has n_teleports + 1 elems
    std::vector<VecUlpDiff> ulp_diffs;
    // direction of each teleport (true for the primary portal), has n_teleports elems
    std::vector<bool> tp_dirs;
    // at each point, the number of teleports queued by a portal; 1 or 2 for the primary portal and
    // -1 or -2 for the secondary; has n_teleports + 1 elems
    std::vector<char> tps_queued;
    // (number of teleports done by primary portal) - (number done by secondary portal), 1 for normal tp, -1 for VAG
    int cum_primary_tps;
    // if true, the chain has more teleports (but all the fields are accurate in the chain so far)
    bool max_tps_exceeded;
    // the entity just before the first teleport (possibly adjusted to be on the portal boundary)
    Entity pre_teleported_ent;
    // the entity as it was transformed through the whole chain
    Entity transformed_ent;

    TeleportChain()
    {
        Clear();
    };

    void Clear();
    void DebugPrintTeleports() const;

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
    void Generate(const PortalPair& pair,
                  bool tp_from_blue,
                  const Entity& ent,
                  const EntityInfo& ent_info,
                  size_t n_max_teleports,
                  bool output_graphviz = false);

private:
    // state for chain generation

    using queue_type = short;

    std::deque<queue_type> tp_queue;
    queue_type n_queued_nulls;
    const PortalPair* pp;
    EntityInfo ent_info;
    size_t max_tps;
    int touch_scope_depth; // CPortalTouchScope::m_nDepth, not fully implemented
    bool blue_primary;

#ifdef CHAIN_WITH_GRAPHVIZ
    bool do_graphviz;
    bool last_should_tp = false;
    Agraph_t* gv_graph;
    std::vector<Agnode_t*> gv_node_stack;
    int cur_touch_call_idx = -1;
#endif

    using portal_type = queue_type;

    portal_type owning_portal;

    static constexpr portal_type FUNC_RECHECK_COLLISION = 0;
    static constexpr portal_type FUNC_TP_BLUE = 1;
    static constexpr portal_type FUNC_TP_ORANGE = 2;
    static constexpr portal_type PORTAL_NONE = 0;

    template <portal_type PORTAL>
    static constexpr portal_type OppositePortalType()
    {
        if constexpr (PORTAL == FUNC_TP_BLUE)
            return FUNC_TP_ORANGE;
        else
            return FUNC_TP_BLUE;
    }

    template <portal_type PORTAL>
    inline bool PortalIsPrimary()
    {
        return blue_primary == (PORTAL == FUNC_TP_BLUE);
    }

    template <portal_type PORTAL>
    inline const Portal& GetPortal()
    {
        return PORTAL == FUNC_TP_BLUE ? pp->blue : pp->orange;
    }

    void CallQueued();

    template <portal_type>
    void ReleaseOwnershipOfEntity(bool moving_to_linked);

    template <portal_type>
    bool SharedEnvironmentCheck();

    template <portal_type>
    void PortalTouchEntity();

    void EntityTouchPortal();

    template <portal_type>
    void TeleportEntity();
};
