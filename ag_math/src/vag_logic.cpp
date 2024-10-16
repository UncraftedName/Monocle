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

struct ChainGenerator {

    using portal_type = decltype(TpChain::_tp_queue)::value_type;
    using func_type = portal_type;

    static constexpr func_type FUNC_RECHECK_COLLISION = 0;
    static constexpr func_type FUNC_TP_BLUE = 1;
    static constexpr func_type FUNC_TP_ORANGE = 2;

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
    const PortalPair& pp;
    size_t max_tps;
    size_t n_ent_children;
    int n_queued_nulls;
    int touch_scope_depth; // CPortalTouchScope::m_nDepth, not fully implemented
    bool blue_primary;

    template <portal_type PORTAL>
    inline bool PortalIsPrimary()
    {
        return blue_primary == (PORTAL == FUNC_TP_BLUE);
    }

    // clang-format off
    inline void PortalTouchScopeCtor() { ++touch_scope_depth; }
    inline void PortalTouchScopeDtor() { --touch_scope_depth; CallQueued(); }
    // clang-format on

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

    void ReleaseOwnershipOfEntity(bool moving_to_linked)
    {
        if (touch_scope_depth > 0)
            for (size_t i = 0; i < n_ent_children + !moving_to_linked; i++)
                chain._tp_queue.push_back(FUNC_RECHECK_COLLISION);
    }

    template <portal_type PORTAL>
    void PortalTouchEntity(size_t tps_queued_idx)
    {
        if (chain.max_tps_exceeded)
            return;
        PortalTouchScopeCtor();
        if ((PORTAL == FUNC_TP_BLUE ? pp.blue : pp.orange).ShouldTeleport(ent, true)) {
            chain.tps_queued[tps_queued_idx] += PortalIsPrimary<PORTAL>() ? 1 : -1;
            TeleportEntity<PORTAL>();
        }
        PortalTouchScopeDtor();
    }

    void EntityTouchPortal()
    {
        PortalTouchScopeCtor();
        PortalTouchScopeDtor();
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
        size_t tps_queued_idx = chain.tps_queued.size();
        chain.tps_queued.push_back(0);

        // calc ulp diff to nearby portal
        chain.ulp_diffs.emplace_back();
        if (chain.cum_primary_tps == 0 || chain.cum_primary_tps == 1) {
            auto& p_ulp_diff_from = (chain.cum_primary_tps == 0) == blue_primary ? pp.blue : pp.orange;
            NudgeEntityTowardsPortalPlane(ent, p_ulp_diff_from, true, false, &chain.ulp_diffs.back());
        } else {
            chain.ulp_diffs.back().ax = ULP_DIFF_TOO_LARGE_AX;
        }

        ReleaseOwnershipOfEntity(true);

        EntityTouchPortal();
        PortalTouchEntity<OppositePortalType<PORTAL>()>(tps_queued_idx);
        PortalTouchEntity<OppositePortalType<PORTAL>()>(tps_queued_idx);
        EntityTouchPortal();
    }
};

// TODO this really needs to be broken up and does not function correct for more than 3 max teleports
void GenerateTeleportChain(const PortalPair& pair,
                           Entity& ent,
                           size_t n_ent_children,
                           bool set_ent_pos_through_chain,
                           bool tp_from_blue,
                           TpChain& chain,
                           size_t n_max_teleports)
{
    chain.Clear();

    chain.ulp_diffs.emplace_back();
    NudgeEntityTowardsPortalPlane(ent, tp_from_blue ? pair.blue : pair.orange, true, true, &chain.ulp_diffs.back());
    chain.pts.push_back(ent.GetCenter());
    chain.tps_queued.push_back(0);

    Entity ent_copy = ent;
    Entity& ref_ent = set_ent_pos_through_chain ? ent : ent_copy;

    ChainGenerator generator{
        .chain = chain,
        .ent = ref_ent,
        .pp = pair,
        .max_tps = n_max_teleports,
        .n_ent_children = n_ent_children,
        .n_queued_nulls = 0,
        .touch_scope_depth = 0,
        .blue_primary = tp_from_blue,
    };
    if (tp_from_blue)
        generator.PortalTouchEntity<ChainGenerator::FUNC_TP_BLUE>(0);
    else
        generator.PortalTouchEntity<ChainGenerator::FUNC_TP_ORANGE>(0);
    assert(chain.max_tps_exceeded || chain._tp_queue.empty());
    assert(generator.touch_scope_depth == 0);
}
