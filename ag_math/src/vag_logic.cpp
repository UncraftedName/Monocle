#include <cmath>
#include <algorithm>

#include "vag_logic.hpp"

void NudgeEntityTowardsPortalPlane(Entity& ent,
                                   const Portal& portal,
                                   bool nudge_behind,
                                   bool change_ent_pos,
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

    Vector orig = ent.origin;

    for (int i = 0; i < 2; i++) {
        bool inv_check = nudge_behind == (bool)i;
        float nudge_towards = ent.origin[nudge_axis] + portal.plane.n[nudge_axis] * (inv_check ? -1.f : 1.f);
        int ulp_diff_incr = inv_check ? -1 : 1;

        while (portal.ShouldTeleport(ent, false) != inv_check) {
            ent.origin[nudge_axis] = std::nextafterf(ent.origin[nudge_axis], nudge_towards);
            if (ulp_diff)
                ulp_diff->Update(nudge_axis, ulp_diff_incr);
        }
    }
    if (!change_ent_pos)
        ent.origin = orig;
}

struct TpChainData {
    TpChain& chain;
    Entity& ent;
    const PortalPair& pp;
    const Portal& p1;
    const Portal& p2;
    int max_tps;
    bool p1_blue;
};

/*
* This is the real guts of the teleport chain generation. It is basically a replication of
* CProp_Portal::TeleportTouchingEntity but without all the unnecessary stuff :). To understand
* this, you must understand what the game does.
* 
* CProp_Portal::TeleportTouchingEntity
* 1) applies the teleport transform and teleports the entity
* 2) calls teleport_entity->PhysicsMarkEntitiesAsTouching(linked_portal)
* 3) calls linked_portal->PhysicsMarkEntitiesAsTouching(teleport_entity)
* 
* Each a->PhysicsMarkEntitiesAsTouching(b) expands to:
* 1) a->PhysicsMarkEntityAsTouched(b)
* 2) b->PhysicsMarkEntityAsTouched(a)
* 
* This means that TeleportTouchingEntity triggers 4 touch calls.
* - each touch call from the linked_portal checks to see if it should teleport the entity
* - each touch call calls queued functions
* 
* Each TeleportTouchingEntity function is not called immediately, but is instead added to a queue.
* It is only called at the end of touch interactions.
* 
* When queued functions are called, a null queued and functions are dequeued and executed until a
* null is reached.
* 
* The template is here just so that it's easier to see what's going on in the callstack.
*/
template <bool DIR>
static bool ChainTeleportRecursive(TpChainData& data)
{
    auto& chain = data.chain;
    if ((int)chain.tp_dirs.size() >= data.max_tps)
        return false;
    chain.tp_dirs.push_back(DIR);
    chain.cum_primary_tps += DIR ? 1 : -1;
    data.pp.Teleport(data.ent, data.p1_blue == DIR);
    chain.pts.push_back(data.ent.GetCenter());
    chain.tps_queued.push_back(0);
    chain.ulp_diffs.emplace_back();
    auto& tp_towards_p = DIR ? data.p2 : data.p1;
    if ((chain.cum_primary_tps == 0 && !DIR) || (chain.cum_primary_tps == 1 && DIR))
        NudgeEntityTowardsPortalPlane(data.ent, tp_towards_p, true, false, &chain.ulp_diffs.back());
    else
        chain.ulp_diffs.back().ax = ULP_DIFF_TOO_LARGE_AX;

    auto& queue = chain._tp_queue;
    for (int touch = 0; touch < 4; touch++) {

        if ((touch == 1 || touch == 2) && tp_towards_p.ShouldTeleport(data.ent, true)) {
            queue.push_back(DIR ? -1 : 1);
            chain.tps_queued.back() += DIR ? -1 : 1;
        }

        queue.push_back(0);
        for (;;) {
            int i = queue.front();
            queue.pop_front();
            if (i == 0)
                break;
            bool ret = i == 1 ? ChainTeleportRecursive<1>(data) : ChainTeleportRecursive<0>(data);
            if (!ret)
                return false;
        }
    }
    return true;
}

void GenerateTeleportChain(const PortalPair& pair, Entity& ent, bool tp_from_blue, TpChain& chain, int n_max_teleports)
{
    assert(n_max_teleports > 0);
    chain.Clear();
    TpChainData data{
        .chain = chain,
        .ent = ent,
        .pp = pair,
        .p1 = tp_from_blue ? pair.blue : pair.orange,
        .p2 = tp_from_blue ? pair.orange : pair.blue,
        .max_tps = n_max_teleports,
        .p1_blue = tp_from_blue,
    };
    // add data for the first point
    chain.ulp_diffs.emplace_back();
    NudgeEntityTowardsPortalPlane(ent, data.p1, true, true, &chain.ulp_diffs.back());
    chain.pts.push_back(ent.GetCenter());
    chain.tps_queued.push_back(1);

    /*
    * Pretend we're at the end of the first PortalTouchScope and we're about to trigger a teleport.
    * We add a "null" and call the "teleport" function, and when it returns (if we don't exceed the
    * teleport limit) we expect a single null to still be there.
    */
    chain._tp_queue.push_back(0);
    chain.max_tps_exceeded = !ChainTeleportRecursive<1>(data);
    assert(chain.max_tps_exceeded || (chain._tp_queue.size() == 1 && chain._tp_queue.front() == 0));
}
