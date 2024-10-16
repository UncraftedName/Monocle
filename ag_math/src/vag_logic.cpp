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
    assert(fabsf(portal.pos[nudge_axis]) > 0.03f); // this'll take too long to converge close to 0
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
    size_t max_tps;
    size_t n_ent_children;
    int n_queued_nulls;
    int touch_scope_depth; // CPortalTouchScope::m_nDepth, not fully implemented
    bool blue_primary;
};

#define FUNC_RECHECK_COLLISION 0
#define FUNC_TP_BLUE 1
#define FUNC_TP_ORANGE 2
#define QUEUE_NEXT_NULL(chain_data) (--(chain_data).n_queued_nulls)
#define FUNC_IS_NULL(val) ((val) < 0)

/*
* This is the real guts of the teleport chain generation. It is basically a replication of
* CProp_Portal::TeleportTouchingEntity but without all the unnecessary stuff :). This is tricky to
* follow even with source code!
* 
* The key idea is that there are two functions (TeleportTouchEntity & RecheckEntityCollision) which
* are deferred by the use of a queue until the end of the current touch interaction. Despite the
* queue having a touch depth, it seems to not be correctly designed for nested touch interactions.
* 
* The template is here just so that it's easier to see what's going on in the callstack.
*/
template <int PORTAL>
static bool ChainTeleportRecursive(TpChainData& data)
{
    static_assert(PORTAL == FUNC_TP_BLUE || PORTAL == FUNC_TP_ORANGE);
    auto& chain = data.chain;

    if (chain.tp_dirs.size() >= data.max_tps)
        return false;

    // do teleport (start of TeleportTouchingEntity)
    bool p_is_primary = data.blue_primary == (PORTAL == FUNC_TP_BLUE);
    chain.tp_dirs.push_back(p_is_primary);
    chain.cum_primary_tps += p_is_primary ? 1 : -1;
    data.pp.Teleport(data.ent, PORTAL == FUNC_TP_BLUE);
    chain.pts.push_back(data.ent.GetCenter());
    size_t tps_queued_idx = chain.tps_queued.size();
    chain.tps_queued.push_back(0);

    // calc ulp diff to nearby portal
    chain.ulp_diffs.emplace_back();
    if (chain.cum_primary_tps == 0 || chain.cum_primary_tps == 1) {
        auto& p_ulp_diff_from = (chain.cum_primary_tps == 0) == data.blue_primary ? data.pp.blue : data.pp.orange;
        NudgeEntityTowardsPortalPlane(data.ent, p_ulp_diff_from, true, false, &chain.ulp_diffs.back());
    } else {
        chain.ulp_diffs.back().ax = ULP_DIFF_TOO_LARGE_AX;
    }

    auto& queue = chain._tp_queue;

    /*
    * Queue RecheckEntityCollision as many times as the game does. During a normal teleport this
    * function is not deferred through the queue (it is executed immediately) because the first
    * TeleportTouchingEntity is not in a touch scope. The function itself doesn't do anything
    * relevant - we only care how it interacts with the queue.
    * 
    * Each RecheckEntityCollision function is called from ReleaseOwnershipOfEntity. The first one
    * is ignored because bMovingToLinkedSimulator=true, but that parameter is not forwarded to the
    * ReleaseOwnershipOfEntity calls for the entity children.
    * 
    * TODO why do VAGs not queue this??
    */
    if (data.touch_scope_depth > 0)
        for (size_t i = 1; i < data.n_ent_children; i++)
            queue.push_back(FUNC_RECHECK_COLLISION);

    /*
    * Replicate the 4 touch calls that the game does to the linked portal. The following:
    * 
    * pOther->PhysicsMarkEntitiesAsTouching( m_hLinkedPortal.Get(), ... );
    * m_hLinkedPortal.Get()->PhysicsMarkEntitiesAsTouching( pOther, ... );
    * 
    * expands to:
    * 
    * pOther->PhysicsMarkEntityAsTouched(m_hLinkedPortal.Get())
    * m_hLinkedPortal.Get()->PhysicsMarkEntityAsTouched(pOther)
    * m_hLinkedPortal.Get()->PhysicsMarkEntityAsTouched(pOther)
    * pOther->PhysicsMarkEntityAsTouched(m_hLinkedPortal.Get())
    * 
    * Each touch call has some body an and associated CPortalTouchScope. The only relevant body is
    * CProp_Portal::Touch since it calls ShouldTeleportTouchEntity which defers TeleportTouchEntity
    * through the queue.
    */
    for (int touch = 0; touch < 4; touch++) {

        data.touch_scope_depth++;

        // body of CProp_Portal::Touch
        auto& linked = PORTAL == FUNC_TP_BLUE ? data.pp.orange : data.pp.blue;
        if ((touch == 1 || touch == 2) && linked.ShouldTeleport(data.ent, true)) {
            queue.push_back(PORTAL == FUNC_TP_BLUE ? FUNC_TP_ORANGE : FUNC_TP_BLUE);
            chain.tps_queued[tps_queued_idx] += p_is_primary ? 1 : -1; // TODO I might need to count this per portal
        }

        /*
        * This is the logic from ~CPortalTouchScope. Queue a NULL, then dequeue and execute
        * functions until a NULL is reached.
        */
        assert(data.touch_scope_depth >= 1);
        if (--data.touch_scope_depth > 0)
            continue;
        queue.push_back(QUEUE_NEXT_NULL(data));
        for (bool dequeued_null = false; !dequeued_null;) {
            int val = queue.front();
            queue.pop_front();
            switch (val) {
                case FUNC_RECHECK_COLLISION:
                    break;
                case FUNC_TP_BLUE:
                case FUNC_TP_ORANGE:
                    if (val == FUNC_TP_BLUE ? !ChainTeleportRecursive<FUNC_TP_BLUE>(data)
                                            : !ChainTeleportRecursive<FUNC_TP_ORANGE>(data))
                        return false;
                    break;
                default:
                    assert(FUNC_IS_NULL(val));
                    dequeued_null = true;
                    break;
            }
        }
    }
    return true;
}

// TODO this really needs to be broken up and does not function correct for more than 3 max teleports
void GenerateTeleportChain(const PortalPair& pair,
                           Entity& ent,
                           size_t n_ent_children,
                           bool tp_from_blue,
                           TpChain& chain,
                           size_t n_max_teleports)
{
    chain.Clear();
    TpChainData data{
        .chain = chain,
        .ent = ent,
        .pp = pair,
        .max_tps = n_max_teleports,
        .n_ent_children = n_ent_children,
        .n_queued_nulls = 0,
        .touch_scope_depth = 0,
        .blue_primary = tp_from_blue,
    };
    // add data for the first point
    chain.ulp_diffs.emplace_back();
    NudgeEntityTowardsPortalPlane(ent, tp_from_blue ? pair.blue : pair.orange, true, true, &chain.ulp_diffs.back());
    chain.pts.push_back(ent.GetCenter());
    chain.tps_queued.push_back(1);

    /*
    * Pretend we've just finished a CProp_Portal::Touch call (and we've queued a teleport) during 
    * Pretend we're at the end of the first PortalTouchScope and we're about to trigger a teleport.
    * We add a "null" and call the "teleport" function, and when it returns (if we don't exceed the
    * teleport limit) we expect a single null to still be there. Since this is after the touch,
    * CPortalTouchScope::m_nDepth is 0 right now.
    */
    chain._tp_queue.push_back(QUEUE_NEXT_NULL(data));
    chain.max_tps_exceeded =
        tp_from_blue ? !ChainTeleportRecursive<FUNC_TP_BLUE>(data) : !ChainTeleportRecursive<FUNC_TP_ORANGE>(data);
    assert(chain.max_tps_exceeded || (chain._tp_queue.size() == 1 && FUNC_IS_NULL(chain._tp_queue.front())));
    assert(data.touch_scope_depth == 0);
}
