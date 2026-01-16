#pragma once

#include <stdint.h>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <utility>

#include "source_math.hpp"

class GvGen;

/*
* Represents a "distance in ulps" from a point to a portal plane. Specifically, this the number of
* ulps a point would need to be nudged along the largest portal normal component until the point is
* exactly behind the portal.
* 
* v behind v         v in front v
* 1        0 |->     1         2
*            ^ portal plane
*/
struct PointToPortalPlaneUlpDist {
    uint32_t n_ulps;
    uint32_t ax : 2;
    uint32_t pt_was_behind_portal : 1;
    uint32_t is_valid : 1;
};

std::pair<Entity, PointToPortalPlaneUlpDist> ProjectEntityToPortalPlane(const Entity& ent, const Portal& portal);

// opt-in flags for stuff that's recorded for every teleport
enum TeleportChainRecordFlags : uint32_t {
    TCRF_NONE = 0,
    TCRF_RECORD_ENTITY = 1 << 0,
    TCRF_RECORD_PLANE_DIFFS = 1 << 1,
    TCRF_RECORD_TP_DIRS = 1 << 2,

    TCRF_RECORD_ALL = ~0u,
};

// this struct can (and should) be reused when generating multiple chains
struct TeleportChainParams {
    // the portals used for the teleport(s)
    const PortalPair* pp;
    // the entity being teleported
    Entity ent;
    /*
    * If true, the entity will be nudged until it is "1 ulp" behind the portal plane (along the
    * largest axis of the portal normal). Otherwise, an entity in front of the portal will not
    * be teleported at all.
    */
    bool project_to_first_portal_plane = true;
    /*
    * In the map where these portals exist, is the map origin inbounds?
    * 
    * This *is* used in the chain generation code, and it's possible that it may be relevant in
    * some niche cases, although I haven't found any (yet).
    */
    bool map_origin_inbounds = false;
    // which portal is the first teleport from?
    bool first_tp_from_blue;
    // TeleportChainRecordFlags
    uint32_t record_flags = TCRF_RECORD_ENTITY | TCRF_RECORD_TP_DIRS;
    // limit the maximum number of teleports that the chain can do
    size_t n_max_teleports = 10;

    // PImpl for internal state
    struct InternalState;
    mutable std::unique_ptr<InternalState> _st;

    // for internal use - have to keep these in the header for gv_gen.cpp
    struct InternalStateDefs {
        using queue_entry = int;
        using queue_type = std::deque<queue_entry>;

        using portal_type = queue_entry;
        static constexpr queue_entry FUNC_RECHECK_COLLISION = 0;
        static constexpr portal_type FUNC_TP_BLUE = 1;
        static constexpr portal_type FUNC_TP_ORANGE = 2;
        static constexpr portal_type PORTAL_NONE = 0;
    };

    /*
    * Annoyingly, to use PImpl this class must define a constructor. This inits the params with
    * sensible defaults, but every field above is public and can be changed.
    */
    TeleportChainParams(const PortalPair* pp, Entity ent);
    TeleportChainParams() : TeleportChainParams(nullptr, Entity{}) {}

    ~TeleportChainParams();
};

// this struct can (and should) be reused when generating multiple chains
struct TeleportChainResult {
    /*
    * If set, this chain has at least params.n_max_teleports and the fields in the this struct will
    * only be accurate up to params.n_max_teleports number of teleports.
    */
    bool max_tps_exceeded;
    // the total number of teleports done by both portals
    size_t total_n_teleports;
    /*
    * A cumulative teleport count. The primary (entry) portal is counted as +1, and the opposite
    * portal is -1. A normal teleport (no angle glitch) would be +1, and a normal angle glitch
    * would be -1.
    */
    int cum_teleports;
    // the final entity position
    Entity ent;
    /*
    * If params.flags has TCRF_RECORD_ENTITY, this has the entity position at each step (including
    * the position before any teleports and the final position), has total_n_teleports+1 elements.
    * If params.project_to_first_portal_plane is true, the first element will be the entity *after*
    * the nudge towards the portal and the other elements will not be nudged.
    */
    std::vector<Entity> ents;
    /*
    * If params.flags has TCRF_RECORD_PLANE_DIFFS, this has the "distance in ulps" to the nearby
    * portal plane. The diffs are only recorded when the entity is on a portal plane (cum teleports
    * is 0 or 1), otherwise the diff is set to invalid. Has total_n_teleports + 1 elements.
    */
    std::vector<PointToPortalPlaneUlpDist> portal_plane_diffs;
    /*
    * If params.flags has TCRF_RECORD_TP_DIRS, this records which portals did each teleport. True
    * for primary (first) portal, false otherwise. Has total_n_teleports elements.
    */
    std::vector<bool> tp_dirs;
    // an optional pointer (user-supplied), which will record the chain as a graphviz graph
    GvGen* dg = nullptr;

    /*
    * Create a string that can be printed to stdout for debugging what the chain looks like. This
    * chain result should have already be populated by GenerateTeleportChain(), and params should
    * be the same as those passed to that function. This requires params.recordFlags to have both
    * flags (TCRF_RECORD_TP_DIRS | TCRF_RECORD_ENTITY).
    */
    std::string CreateDebugString(const TeleportChainParams& params) const;
};

/*
* The real meat, juice and bones. This replicates (most of) the game's teleportation code to
* determine if an angle glitch occurs. It can also be used to detect VAG crashes or more exotic
* angle glitches.
*/
void GenerateTeleportChain(const TeleportChainParams& params, TeleportChainResult& result);

/*
* Generates a GraphViz graph representing a teleport chain. Originally this was done via the
* Graphviz C API, but that led to DLL linking hell. So instead this just formats the file manually.
*/
class GvGen {
    std::vector<int> node_stack;
    int node_counter;
    int touch_call_index; // which of the 4 touch calls are we in right now?
    bool last_teleport_call_queued_a_teleport;

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

private:
    // clears the state and starts a new graph with the given color for the initial teleport portal
    void ResetAndPushRootNode(bool blue);
    void PushCallQueuedNode(const TeleportChainParams::InternalStateDefs::queue_type& queue);
    // planeSide=-1 -> ShouldTeleport()=true, planeSide=1 -> ShouldTeleport()=false, otherwise ignored
    void PushTeleportNode(bool blue, int cum_teleports, int planeSide);
    void PushExceededTpNode(bool blue);

    void SetTouchCallIndex(int i)
    {
        touch_call_index = i;
    }

    void SetLastTeleportCallQueuedTeleport(bool x)
    {
        last_teleport_call_queued_a_teleport = x;
    }

    // for every push, there is a pop
    void PopNode()
    {
        node_stack.pop_back();
    }

    void Finish()
    {
        buf += "}\n";
    };

    friend void GenerateTeleportChain(const TeleportChainParams& params, TeleportChainResult& result);
    friend struct GenerateTeleportChainImpl;
};
