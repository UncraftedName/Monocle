#include <cmath>
#include <algorithm>

#include "vag_logic.hpp"

void NudgeEntityBehindPortalPlane(Entity& ent, const Portal& portal, bool change_ent_pos, VecUlpDiff* ulp_diff)
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
        float nudge_towards = ent.origin[nudge_axis] + portal.plane.n[nudge_axis] * (i ? -1.f : 1.f);
        int ulp_diff_incr = i ? -1 : 1;

        while (portal.ShouldTeleport(ent, false) != (bool)i) {
            ent.origin[nudge_axis] = std::nextafterf(ent.origin[nudge_axis], nudge_towards);
            if (ulp_diff)
                ulp_diff->Update(nudge_axis, ulp_diff_incr);
        }
    }
    if (!change_ent_pos)
        ent.origin = orig;
}

struct ChainGenerator {

    using portal_type = decltype(TpChain::_tp_queue)::value_type;

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

    TpChain& chain;
    Entity& ent;
    EntityInfo ent_info;
    const PortalPair& pp;
    size_t max_tps;
    int n_queued_nulls;
    int touch_scope_depth; // CPortalTouchScope::m_nDepth, not fully implemented
    portal_type owning_portal;
    bool blue_primary;

    template <portal_type PORTAL>
    inline bool PortalIsPrimary()
    {
        return blue_primary == (PORTAL == FUNC_TP_BLUE);
    }

    template <portal_type PORTAL>
    inline const Portal& GetPortal()
    {
        return PORTAL == FUNC_TP_BLUE ? pp.blue : pp.orange;
    }

    void CallQueued()
    {
        chain._tp_queue.push_back(-++n_queued_nulls);
        for (bool dequeued_null = false; !dequeued_null && !chain.max_tps_exceeded;) {
            int val = chain._tp_queue.front();
            chain._tp_queue.pop_front();
            switch (val) {
                case FUNC_RECHECK_COLLISION:
                    break;
                case FUNC_TP_BLUE:
                    TeleportEntity<FUNC_TP_BLUE>();
                    break;
                case FUNC_TP_ORANGE:
                    TeleportEntity<FUNC_TP_ORANGE>();
                    break;
                default:
                    assert(val < 0);
                    dequeued_null = true;
                    break;
            }
        }
    }

    template <portal_type PORTAL>
    void ReleaseOwnershipOfEntity(bool moving_to_linked)
    {
        if (PORTAL != owning_portal)
            return;
        owning_portal = PORTAL_NONE;
        if (touch_scope_depth > 0)
            for (size_t i = 0; i < ent_info.n_ent_children + !moving_to_linked; i++)
                chain._tp_queue.push_back(FUNC_RECHECK_COLLISION);
    }

    template <portal_type PORTAL>
    bool SharedEnvironmentCheck()
    {
        if (owning_portal == PORTAL_NONE || owning_portal == PORTAL)
            return true;
        Vector ent_center = ent.GetCenter();
        return ent_center.DistToSqr(GetPortal<PORTAL>().pos) <
               ent_center.DistToSqr(GetPortal<OppositePortalType<PORTAL>()>().pos);
    }

    template <portal_type PORTAL>
    void PortalTouchEntity()
    {
        if (chain.max_tps_exceeded)
            return;
        ++touch_scope_depth;
        if (owning_portal != PORTAL) {
            if (SharedEnvironmentCheck<PORTAL>()) {
                bool ent_in_front = GetPortal<PORTAL>().plane.n.Dot(ent.GetCenter()) > GetPortal<PORTAL>().plane.d;
                bool player_stuck = ent.player ? !ent_info.origin_inbounds : false;
                if (ent_in_front || player_stuck) {
                    if (owning_portal != PORTAL && owning_portal != PORTAL_NONE)
                        ReleaseOwnershipOfEntity<OppositePortalType<PORTAL>()>(false);
                    owning_portal = PORTAL;
                }
            }
        }
        if (GetPortal<PORTAL>().ShouldTeleport(ent, true)) {
            chain.tps_queued.back() += PortalIsPrimary<PORTAL>() ? 1 : -1;
            TeleportEntity<PORTAL>();
        }
        if (--touch_scope_depth == 0)
            CallQueued();
    }

    void EntityTouchPortal()
    {
        if (touch_scope_depth == 0)
            CallQueued();
    }

    template <portal_type PORTAL>
    void TeleportEntity()
    {
        if (touch_scope_depth > 0) {
            chain._tp_queue.push_back(PORTAL);
            return;
        }

        if (chain.tp_dirs.size() >= max_tps) {
            chain.max_tps_exceeded = true;
            return;
        }

        chain.tp_dirs.push_back(PortalIsPrimary<PORTAL>());
        chain.cum_primary_tps += PortalIsPrimary<PORTAL>() ? 1 : -1;
        pp.Teleport(ent, PORTAL == FUNC_TP_BLUE);
        chain.pts.push_back(ent.GetCenter());
        chain.tps_queued.push_back(0);

        // calc ulp diff to nearby portal
        chain.ulp_diffs.emplace_back();
        if (chain.cum_primary_tps == 0 || chain.cum_primary_tps == 1) {
            auto& p_ulp_diff_from = (chain.cum_primary_tps == 0) == blue_primary ? pp.blue : pp.orange;
            NudgeEntityBehindPortalPlane(ent, p_ulp_diff_from, false, &chain.ulp_diffs.back());
        } else {
            chain.ulp_diffs.back().SetInvalid();
        }

        ReleaseOwnershipOfEntity<PORTAL>(true);
        owning_portal = OppositePortalType<PORTAL>();

        EntityTouchPortal();
        PortalTouchEntity<OppositePortalType<PORTAL>()>();
        PortalTouchEntity<OppositePortalType<PORTAL>()>();
        EntityTouchPortal();
    }
};

void GenerateTeleportChain(TpChain& chain,
                           const PortalPair& pair,
                           bool tp_from_blue,
                           Entity& ent,
                           EntityInfo ent_info,
                           size_t n_max_teleports)
{
    chain.Clear();

    chain.ulp_diffs.emplace_back();
    NudgeEntityBehindPortalPlane(ent, tp_from_blue ? pair.blue : pair.orange, true, &chain.ulp_diffs.back());
    chain.pts.push_back(ent.GetCenter());
    chain.tps_queued.push_back(0);

    Entity ent_copy = ent;

    ChainGenerator generator{
        .chain = chain,
        .ent = ent_info.set_ent_pos_through_chain ? ent : ent_copy,
        .ent_info = ent_info,
        .pp = pair,
        .max_tps = n_max_teleports,
        .n_queued_nulls = 0,
        .touch_scope_depth = 0,
        .owning_portal = tp_from_blue ? ChainGenerator::FUNC_TP_BLUE : ChainGenerator::FUNC_TP_ORANGE,
        .blue_primary = tp_from_blue,
    };
    if (tp_from_blue)
        generator.PortalTouchEntity<ChainGenerator::FUNC_TP_BLUE>();
    else
        generator.PortalTouchEntity<ChainGenerator::FUNC_TP_ORANGE>();
    assert(chain.max_tps_exceeded || chain._tp_queue.empty());
    assert(generator.touch_scope_depth == 0);
}
