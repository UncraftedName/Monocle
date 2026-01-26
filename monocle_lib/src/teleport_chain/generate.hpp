#pragma once

#include "monocle_config.hpp"
#include "game/source_math.hpp"
#include "generate_state.hpp"
#include "debug/gv_flow_control.hpp"

#include <stdint.h>
#include <vector>
#include <string>
#include <utility>
#include <ostream>

namespace mon {

class GraphvizGen;

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

// nudges an entity as close to behind-the-portal-plane as possible
std::pair<Entity, PointToPortalPlaneUlpDist> ProjectEntityToPortalPlane(const Entity& ent, const Portal& portal);

// opt-in flags for stuff that's recorded for every teleport
enum TeleportChainRecordFlags : uint32_t {
    TCRF_NONE = 0,
    TCRF_RECORD_ENTITY = 1 << 0,
    TCRF_RECORD_PLANE_DIFFS = 1 << 1,
    TCRF_RECORD_TP_DIRS = 1 << 2,
    TCRF_RECORD_GRAPHVIZ_FLOW_CONTROL = 1 << 3,

    TCRF_RECORD_ALL = ~0u,
};

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

    // this inits everything with sensible defaults, but every field above is public and can be changed
    TeleportChainParams(const PortalPair* pp, Entity ent);
    TeleportChainParams() : TeleportChainParams(nullptr, Entity{}) {}
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
    /*
    * If params.flags has TCRF_RECORD_GRAPHVIZ_FLOW_CONTROL, contains detailed information about
    * the flow control of the teleport & touch functions. This can be dumped to a graphviz (.gv)
    * file. If TCRF_RECORD_PLANE_DIFFS is also set, this will have slightly more infomration.
    */
    GraphvizFlowControlResult graphviz_flow_control;

    // internal
    TeleportChainInternalState _st;

    /*
    * Write a debug string to e.g. stdout to see at a glance what the chain looks like.
    * 
    * This chain result should have already be populated by GenerateTeleportChain(), and params
    * should be the same as those passed to that function. This requires params.recordFlags to
    * have (TCRF_RECORD_ENTITY | TCRF_RECORD_TP_DIRS).
    */
    std::ostream& MiniDump(std::ostream& os, const TeleportChainParams& params) const;

    /*
    * For each teleport in this chain, creates a double precision entity and applies the same
    * teleport. The float entity, double entity, and difference between them is written to a stream
    * which can be exported as a CSV file.
    * 
    * This chain result should have already be populated by GenerateTeleportChain(), and params
    * should be the same as those passed to that function. This requires params.recordFlags to
    * have (TCRF_RECORD_ENTITY | TCRF_RECORD_TP_DIRS).
    */
    std::ostream& CompareWithHighPrecisionChainToCsv(std::ostream&, const TeleportChainParams& params) const;
};

/*
* The real meat, juice and bones. This replicates (most of) the game's teleportation code to
* determine if an angle glitch occurs. It can also be used to detect VAG crashes or more exotic
* angle glitches.
*/
void GenerateTeleportChain(const TeleportChainParams& params, TeleportChainResult& result);

} // namespace mon
