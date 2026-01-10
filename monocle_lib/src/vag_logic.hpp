#pragma once

#include <stdint.h>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <utility>

#include "source_math.hpp"

class DotGen;

/*
* Represents a difference between two Vectors in ULPs. All VAG related code only nudges points
* along a single axis, hence the abstraction. For VAG code a positive diff is in the same
* direction as the portal normal, and diff=0 means exactly 
*/
struct VecUlpDiff {
    uint32_t ax : 2;            // [0, 2] - the axis along which the difference is measured
    int diff : 29;              // the difference in ulps
    uint32_t diff_overflow : 1; // if set, the difference is bigger than can be expressed

    VecUlpDiff()
    {
        Reset();
    }

    void Reset()
    {
        ax = 3;
        diff = 0;
        diff_overflow = 0;
    }

    void Update(int along, int by)
    {
        assert(ax == 3 || along == ax || by == 0);
        ax = along;
        diff += by;
    }

    void SetInvalid()
    {
        ax = 3;
    }

    bool Valid() const
    {
        return ax != 3;
    }

    bool PtWasBehindPlane() const
    {
        assert(Valid());
        return diff <= 0; // TODO isn't this the opposite of what I want?
    }
};

/*
* Calculates how many ulp nudges were needed to move the given entity until it's behind the portal
* plane as determined by Portal::ShouldTeleport(). If the point is already behind, nudges are done
* in the direction of the portal normal so long as ShouldTeleport returns true.
* 
* TODO check if this has the write comment and result
*/
std::pair<Entity, VecUlpDiff> NudgeEntityBehindPortalPlane(const Entity& ent,
                                                           const Portal& portal,
                                                           size_t n_max_nudges = 1000);

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

// opt-in flags for stuff that's recorded for every teleport
enum TeleportChainRecordFlags : uint32_t {
    TCRF_NONE = 0,
    TCRF_RECORD_ENTITY = 1 << 0,
    // do not use for LAG/AAG - those teleport the player center far away from the portal plane
    TCRF_RECORD_ULP_DIFFS = 1 << 1,

    TCRF_RECORD_ALL = ~0u,
};

struct GenerateTeleportChainParams {
    // the portals used for the teleport(s)
    const PortalPair* pp;
    // the entity being teleported
    Entity ent;
    /*
    * The number of children the entity has. Usually this is N_CHILDREN_PLAYER_WITH_PORTAL_GUN for
    * the player and 0 otherwise.
    * 
    * This *is* used in the chain generation code, and it's possible that it may be relevant in
    * that it may be relevant in some niche cases, although I haven't found any (yet).
    */
    unsigned char n_ent_children;
    /*
    * If true, the entity will be nudged until it is "1 ulp" behind the portal plane (along the
    * largest axis of the portal normal). Otherwise, an entity in front of the portal will not
    * be teleported at all. If you set this to true, check that the entity is very close to
    */
    bool nudge_to_first_portal_plane = true;
    /*
    * In the map where these portals exist, is the map origin inbounds?
    * 
    * This *is* used in the chain generation code, and it's possible that it may be relevant in
    * that it may be relevant in some niche cases, although I haven't found any (yet).
    */
    bool map_origin_inbounds = false;
    // which portal is the first teleport from?
    bool first_tp_from_blue;
    TeleportChainRecordFlags recordFlags = TCRF_RECORD_ENTITY;
    // limit the maximum number of teleports that the chain can do
    size_t n_max_teleports = 10;
    // if using TCRF_RECORD_ULP_DIFFS, this is the max number of nudges to attempt for each portal,
    // if this number is exceeded, the ulp diff is set to invalid
    /*
    * If nudge_to_first_portal_plane=true, this is the maximum number of nudges done only for the
    * first portal. If the max number of nudge is exceeded, first_ulp_nudge_exceed will be true in
    * the result struct and no teleports will be done.
    * 
    * If TCRF_RECORD_ULP_DIFFS is set, this is the maximum number of nudges done at each teleport
    * where the entity is at a portal boundary (cum_tp == 0 || cum_tp == 1).
    */
    size_t n_max_ulp_nudges = 100;

    struct InternalState;
    mutable std::unique_ptr<InternalState> _st;

    // for internal use - have to keep them in the header for dot_gen.cpp
    struct InternalStateDefs {
        using queue_entry = short;
        using queue_type = std::deque<queue_entry>;

        using portal_type = queue_entry;
        static constexpr queue_entry FUNC_RECHECK_COLLISION = 0;
        static constexpr portal_type FUNC_TP_BLUE = 1;
        static constexpr portal_type FUNC_TP_ORANGE = 2;
        static constexpr portal_type PORTAL_NONE = 0;
    };

    GenerateTeleportChainParams(const PortalPair* pp, Entity ent);

    ~GenerateTeleportChainParams();
};

// TODO doc
struct TeleportChainResult {
    bool max_tps_exceeded;
    bool first_ulp_nudge_exceed;
    int total_n_teleports;
    int cum_teleports;
    Entity ent;
    std::vector<Entity> ents;
    std::vector<VecUlpDiff> ulp_diffs_from_portal_plane;
    DotGen* dg = nullptr;
};

void GenerateTeleportChain(const GenerateTeleportChainParams& params, TeleportChainResult& result);

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

    using queue_entry = short;
    using queue_type = std::deque<queue_entry>;

    using portal_type = queue_entry;

    static constexpr portal_type FUNC_RECHECK_COLLISION = 0;
    static constexpr portal_type FUNC_TP_BLUE = 1;
    static constexpr portal_type FUNC_TP_ORANGE = 2;
    static constexpr portal_type PORTAL_NONE = 0;

    // state for chain generation

    queue_type tp_queue;
    queue_entry n_queued_nulls;
    const PortalPair* pp;
    EntityInfo ent_info;
    size_t max_tps;
    int touch_scope_depth; // CPortalTouchScope::m_nDepth, not fully implemented
    bool blue_primary;

    // state for graphviz dot generation
    DotGen* dg = nullptr;
    bool last_should_tp = false;
    int cur_touch_call_idx = -1;

    portal_type owning_portal;

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

/*
* Generates a DOT format graph representing a teleport chain. Originally this was done via the
* Graphviz C API, but that led to DLL linking hell. So instead we just format the dot format
* manually.
*/
class DotGen {
    std::vector<int> nodeStack;
    int nodeCounter = 0;

public:
    // after Finish(), contains the DOT graph text
    std::string buf;

    struct {
        const char* indent = "    ";
        const char* blueCol = "cornflowerblue";
        const char* orangeCol = "orange";
        const char* defaultEdgeAttributes = "style = dotted";
        const char* teleportEdgeAttributes = "color = red";
    } style;

    // clears the state and starts a new graph, with the given color for the initial teleport portal
    void ResetAndPushRootNode(bool blue);
    void PushCallQueuedNode(bool queued_teleport, const TeleportChain::queue_type& queue, int from_touch_call_idx);
    // planeSide=-1 -> ShouldTeleport()=true, planeSide=1 -> ShouldTeleport()=false, otherwise ignored
    void PushTeleportNode(bool blue, int cum_teleports, int planeSide);
    void PushExceededTpNode(bool blue);

    // for every push, there is a pop
    void PopNode()
    {
        nodeStack.pop_back();
    }

    void Finish()
    {
        buf += "}\n";
    };
};
