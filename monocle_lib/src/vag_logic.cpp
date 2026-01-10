#include <cmath>
#include <algorithm>
#include <stdarg.h>
#include <vector>
#include <fstream>

#include "vag_logic.hpp"

std::pair<Entity, VecUlpDiff> NudgeEntityBehindPortalPlane(const Entity& ent, const Portal& portal, size_t n_max_nudges)
{
    int nudge_axis;
    float biggest = -INFINITY;
    for (int i = 0; i < 3; i++) {
        if (fabsf(portal.plane.n[i]) > biggest) {
            biggest = fabsf(portal.plane.n[i]);
            nudge_axis = i;
        }
    }
    VecUlpDiff ulp_diff{};

    Entity new_ent = ent;

    for (int i = 0; i < 2; i++) {
        Vector& ent_pos = new_ent.GetPosRef();
        float nudge_towards = ent_pos[nudge_axis] + portal.plane.n[nudge_axis] * (i ? -10000.f : 10000.f);
        int ulp_diff_incr = i ? -1 : 1;
        int n_nudges = 0;

        while (portal.ShouldTeleport(new_ent, false) != (bool)i && n_nudges++ < n_max_nudges) {
            ent_pos[nudge_axis] = std::nextafterf(ent_pos[nudge_axis], nudge_towards);
            ulp_diff.Update(nudge_axis, ulp_diff_incr);
        }

        // prevent infinite loop
        if (n_nudges >= n_max_nudges) {
            ulp_diff.diff_overflow = 1;
            return {new_ent, ulp_diff};
        }
    }
    return {new_ent, ulp_diff};
}

void TeleportChain::Clear()
{
    pts.clear();
    ulp_diffs.clear();
    tp_dirs.clear();
    tps_queued.clear();
    cum_primary_tps = 0;
    max_tps_exceeded = false;
    tp_queue.clear();
    cum_primary_tps = 0;
    max_tps_exceeded = 0;
    n_queued_nulls = 0;
    touch_scope_depth = 0;
}

void TeleportChain::DebugPrintTeleports() const
{
    int min_cum = 0, max_cum = 0, cum = 0;
    for (bool dir : tp_dirs) {
        cum += dir ? 1 : -1;
        min_cum = cum < min_cum ? cum : min_cum;
        max_cum = cum > max_cum ? cum : max_cum;
    }
    int left_pad = pts.size() <= 1 ? 1 : (int)floor(log10(pts.size() - 1)) + 1;
    cum = 0;
    for (size_t i = 0; i <= tp_dirs.size(); i++) {
        printf("%.*u) ", left_pad, i);
        for (int c = min_cum; c <= max_cum; c++) {
            VecUlpDiff diff = ulp_diffs[i];
            if (c == cum) {
                if (c == 0)
                    printf((i == 0 || diff.PtWasBehindPlane()) ? "0|>" : ".|0");
                else if (c == 1)
                    printf(diff.PtWasBehindPlane() ? "<|1" : "1|.");
                else if (c < 0)
                    printf("%d.", c);
                else
                    printf(".%d.", c);
            } else {
                if (c == 0)
                    printf(".|>");
                else if (c == 1)
                    printf("<|.");
                else
                    printf("...");
            }
        }
        putc('\n', stdout);
        if (i < tp_dirs.size())
            cum += tp_dirs[i] ? 1 : -1;
    }
}

void TeleportChain::Generate(const PortalPair& pair,
                             bool tp_from_blue,
                             const Entity& ent,
                             const EntityInfo& ent_info,
                             size_t n_max_teleports,
                             bool output_graphviz)
{
    Clear();

    pp = &pair;
    this->ent_info = ent_info;
    max_tps = n_max_teleports;
    blue_primary = tp_from_blue;
    owning_portal = tp_from_blue ? FUNC_TP_BLUE : FUNC_TP_ORANGE;

    // TODO change to param
    DotGen dotGen;
    if (output_graphviz)
        dg = &dotGen;

    if (dg)
        dg->ResetAndPushRootNode(tp_from_blue);

    // assume we're teleporting from the portal boundary
    auto [nudged_ent, ulp_diff] = NudgeEntityBehindPortalPlane(ent, tp_from_blue ? pair.blue : pair.orange);
    ulp_diffs.push_back(ulp_diff);
    pre_teleported_ent = nudged_ent;
    pts.push_back(ent.GetCenter());
    tps_queued.push_back(0);

    if (tp_from_blue)
        PortalTouchEntity<FUNC_TP_BLUE>();
    else
        PortalTouchEntity<FUNC_TP_ORANGE>();
    assert(max_tps_exceeded || tp_queue.empty());
    assert(touch_scope_depth == 0);

    if (dg) {
        dg->Finish();
        std::ofstream("chain.dot") << dg->buf;
        dg = nullptr;
    }
}

void TeleportChain::CallQueued()
{
    if (max_tps_exceeded)
        return;

    tp_queue.push_back(-++n_queued_nulls);

    if (dg)
        dg->PushCallQueuedNode(last_should_tp, tp_queue, cur_touch_call_idx);
    last_should_tp = false;

    for (bool dequeued_null = false; !dequeued_null && !max_tps_exceeded;) {
        int val = tp_queue.front();
        tp_queue.pop_front();
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
    if (dg)
        dg->PopNode();
}

template <TeleportChain::portal_type PORTAL>
void TeleportChain::ReleaseOwnershipOfEntity(bool moving_to_linked)
{
    if (PORTAL != owning_portal)
        return;
    owning_portal = PORTAL_NONE;
    if (touch_scope_depth > 0)
        for (int i = 0; i < ent_info.n_ent_children + !moving_to_linked; i++)
            tp_queue.push_back(FUNC_RECHECK_COLLISION);
}

template <TeleportChain::portal_type PORTAL>
bool TeleportChain::SharedEnvironmentCheck()
{
    if (owning_portal == PORTAL_NONE || owning_portal == PORTAL)
        return true;
    Vector ent_center = transformed_ent.GetCenter();
    return ent_center.DistToSqr(GetPortal<PORTAL>().pos) <
           ent_center.DistToSqr(GetPortal<OppositePortalType<PORTAL>()>().pos);
}

template <TeleportChain::portal_type PORTAL>
void TeleportChain::PortalTouchEntity()
{
    if (max_tps_exceeded)
        return;
    ++touch_scope_depth;
    if (owning_portal != PORTAL) {
        if (SharedEnvironmentCheck<PORTAL>()) {
            bool ent_in_front =
                GetPortal<PORTAL>().plane.n.Dot(transformed_ent.GetCenter()) > GetPortal<PORTAL>().plane.d;
            bool player_stuck = transformed_ent.isPlayer ? !ent_info.origin_inbounds : false;
            if (ent_in_front || player_stuck) {
                if (owning_portal != PORTAL && owning_portal != PORTAL_NONE)
                    ReleaseOwnershipOfEntity<OppositePortalType<PORTAL>()>(false);
                owning_portal = PORTAL;
            }
        }
    }
    if (GetPortal<PORTAL>().ShouldTeleport(transformed_ent, true))
        TeleportEntity<PORTAL>();
    if (--touch_scope_depth == 0)
        CallQueued();
}

void TeleportChain::EntityTouchPortal()
{
    if (touch_scope_depth == 0)
        CallQueued();
}

template <TeleportChain::portal_type PORTAL>
void TeleportChain::TeleportEntity()
{
    if (touch_scope_depth > 0) {
        tps_queued.back() += PortalIsPrimary<PORTAL>() ? 1 : -1;
        tp_queue.push_back(PORTAL);
        last_should_tp = true;
        return;
    }

    if (tp_dirs.size() >= max_tps) {
        max_tps_exceeded = true;
        if (dg)
            dg->PushExceededTpNode(PORTAL == FUNC_TP_BLUE);
        return;
    }

    tp_dirs.push_back(PortalIsPrimary<PORTAL>());
    cum_primary_tps += PortalIsPrimary<PORTAL>() ? 1 : -1;
    transformed_ent = pp->Teleport(transformed_ent, PORTAL == FUNC_TP_BLUE);
    pts.push_back(transformed_ent.GetCenter());
    tps_queued.push_back(0);

    // calc ulp diff to nearby portal
    VecUlpDiff ulp_diff{};
    if (cum_primary_tps == 0 || cum_primary_tps == 1) {
        auto& p_ulp_diff_from = (cum_primary_tps == 0) == blue_primary ? pp->blue : pp->orange;
        auto [_, ud] = NudgeEntityBehindPortalPlane(transformed_ent, p_ulp_diff_from);
        ulp_diff = ud;
    } else {
        ulp_diff.SetInvalid();
    }
    ulp_diffs.push_back(ulp_diff);

    assert(!last_should_tp);
    if (dg)
        dg->PushTeleportNode(PORTAL == FUNC_TP_BLUE, cum_primary_tps, 0);

    ReleaseOwnershipOfEntity<PORTAL>(true);
    owning_portal = OppositePortalType<PORTAL>();

    cur_touch_call_idx = 0;
    EntityTouchPortal();
    cur_touch_call_idx = 1;
    PortalTouchEntity<OppositePortalType<PORTAL>()>();
    cur_touch_call_idx = 2;
    PortalTouchEntity<OppositePortalType<PORTAL>()>();
    cur_touch_call_idx = 3;
    EntityTouchPortal();

    if (dg)
        dg->PopNode();
}

struct GenerateTeleportChainParams::InternalState {

    using defs = GenerateTeleportChainParams::InternalStateDefs;

    defs::queue_type tp_queue;
    defs::queue_entry n_queued_nulls;
    int touch_scope_depth; // CPortalTouchScope::m_nDepth, not fully implemented

    // state for graphviz dot generation
    bool last_should_tp;
    int cur_touch_call_idx;

    defs::portal_type owning_portal;
};

GenerateTeleportChainParams::GenerateTeleportChainParams(const PortalPair* pp, Entity ent)
    : pp(pp),
      ent(ent),
      first_tp_from_blue(pp->blue.plane.DistTo(ent.GetCenter()) < pp->orange.plane.DistTo(ent.GetCenter())),
      n_ent_children(ent.isPlayer ? N_CHILDREN_PLAYER_WITH_PORTAL_GUN : 0)
{}

GenerateTeleportChainParams::~GenerateTeleportChainParams() {}

struct GenerateTeleportChainImpl {

    const GenerateTeleportChainParams& usrParams;
    TeleportChainResult& result;

    using defs = GenerateTeleportChainParams::InternalStateDefs;
    GenerateTeleportChainParams::InternalState& st;

    GenerateTeleportChainImpl(const GenerateTeleportChainParams& params, TeleportChainResult& result)
        : usrParams(params), result(result), st(*params._st)
    {}

    void ResetState()
    {
        // clear internal state

        st.tp_queue.clear();
        st.n_queued_nulls = 0;
        st.touch_scope_depth = 0;
        st.last_should_tp = false;
        st.cur_touch_call_idx = -1;
        st.owning_portal = usrParams.first_tp_from_blue ? defs::FUNC_TP_BLUE : defs::FUNC_TP_ORANGE;

        // clear result

        result.max_tps_exceeded = false;
        result.first_ulp_nudge_exceed = false;
        result.total_n_teleports = 0;
        result.cum_teleports = 0;
        result.ent = usrParams.ent;
        result.ents.clear();
        result.ulp_diffs_from_portal_plane.clear();
    }

    template <defs::portal_type PORTAL>
    static constexpr defs::portal_type OppositePortalType()
    {
        if constexpr (PORTAL == defs::FUNC_TP_BLUE)
            return defs::FUNC_TP_ORANGE;
        else
            return defs::FUNC_TP_BLUE;
    }

    template <defs::portal_type PORTAL>
    inline bool PortalIsPrimary()
    {
        return usrParams.first_tp_from_blue == (PORTAL == defs::FUNC_TP_BLUE);
    }

    template <defs::portal_type PORTAL>
    inline const Portal& GetPortal()
    {
        return PORTAL == defs::FUNC_TP_BLUE ? usrParams.pp->blue : usrParams.pp->orange;
    }

    void CallQueued()
    {
        if (result.max_tps_exceeded)
            return;

        st.tp_queue.push_back(-++st.n_queued_nulls);

        if (result.dg)
            result.dg->PushCallQueuedNode(st.last_should_tp, st.tp_queue, st.cur_touch_call_idx);
        st.last_should_tp = false;

        for (bool dequeued_null = false; !dequeued_null && !result.max_tps_exceeded;) {
            int val = st.tp_queue.front();
            st.tp_queue.pop_front();
            switch (val) {
                case defs::FUNC_RECHECK_COLLISION:
                    break;
                case defs::FUNC_TP_BLUE:
                    TeleportEntity<defs::FUNC_TP_BLUE>();
                    break;
                case defs::FUNC_TP_ORANGE:
                    TeleportEntity<defs::FUNC_TP_ORANGE>();
                    break;
                default:
                    assert(val < 0);
                    dequeued_null = true;
                    break;
            }
        }
        if (result.dg)
            result.dg->PopNode();
    }

    template <defs::portal_type PORTAL>
    void ReleaseOwnershipOfEntity(bool moving_to_linked)
    {
        if (PORTAL != st.owning_portal)
            return;
        st.owning_portal = defs::PORTAL_NONE;
        if (st.touch_scope_depth > 0)
            for (int i = 0; i < usrParams.n_ent_children + !moving_to_linked; i++)
                st.tp_queue.push_back(defs::FUNC_RECHECK_COLLISION);
    }

    template <defs::portal_type PORTAL>
    bool SharedEnvironmentCheck()
    {
        if (st.owning_portal == defs::PORTAL_NONE || st.owning_portal == PORTAL)
            return true;
        Vector ent_center = result.ent.GetCenter();
        return ent_center.DistToSqr(GetPortal<PORTAL>().pos) <
               ent_center.DistToSqr(GetPortal<OppositePortalType<PORTAL>()>().pos);
    }

    template <defs::portal_type PORTAL>
    void PortalTouchEntity()
    {
        if (result.max_tps_exceeded)
            return;
        ++st.touch_scope_depth;
        if (st.owning_portal != PORTAL) {
            if (SharedEnvironmentCheck<PORTAL>()) {
                bool ent_in_front =
                    GetPortal<PORTAL>().plane.n.Dot(result.ent.GetCenter()) > GetPortal<PORTAL>().plane.d;
                bool player_stuck = result.ent.isPlayer ? !usrParams.map_origin_inbounds : false;
                if (ent_in_front || player_stuck) {
                    if (st.owning_portal != PORTAL && st.owning_portal != defs::PORTAL_NONE)
                        ReleaseOwnershipOfEntity<OppositePortalType<PORTAL>()>(false);
                    st.owning_portal = PORTAL;
                }
            }
        }
        if (GetPortal<PORTAL>().ShouldTeleport(result.ent, true))
            TeleportEntity<PORTAL>();
        if (--st.touch_scope_depth == 0)
            CallQueued();
    }

    void EntityTouchPortal()
    {
        if (st.touch_scope_depth == 0)
            CallQueued();
    }

    template <defs::portal_type PORTAL>
    void TeleportEntity()
    {
        if (st.touch_scope_depth > 0) {
            st.tp_queue.push_back(PORTAL);
            st.last_should_tp = true;
            return;
        }

        if (result.total_n_teleports >= usrParams.n_max_teleports) {
            result.max_tps_exceeded = true;
            if (result.dg)
                result.dg->PushExceededTpNode(PORTAL == defs::FUNC_TP_BLUE);
            return;
        }

        ++result.total_n_teleports;

        result.cum_teleports += PortalIsPrimary<PORTAL>() ? 1 : -1;
        result.ent = usrParams.pp->Teleport(result.ent, PORTAL == defs::FUNC_TP_BLUE);
        if (usrParams.recordFlags & TCRF_RECORD_ENTITY)
            result.ents.push_back(result.ent);

        int dgPlaneSide = 0;

        if (usrParams.recordFlags & TCRF_RECORD_ULP_DIFFS) {
            VecUlpDiff ulp_diff{};
            if (result.cum_teleports == 0 || result.cum_teleports == 1) {
                auto& p_ulp_diff_from = (result.cum_teleports == 0) == usrParams.first_tp_from_blue
                                          ? usrParams.pp->blue
                                          : usrParams.pp->orange;
                auto [_, ud] = NudgeEntityBehindPortalPlane(result.ent, p_ulp_diff_from);
                ulp_diff = ud;
                if (ulp_diff.Valid())
                    dgPlaneSide = ulp_diff.PtWasBehindPlane() ? -1 : 1;
            } else {
                ulp_diff.SetInvalid();
            }
            result.ulp_diffs_from_portal_plane.push_back(ulp_diff);
        }

        assert(!st.last_should_tp);
        if (result.dg)
            result.dg->PushTeleportNode(PORTAL == defs::FUNC_TP_BLUE, result.cum_teleports, dgPlaneSide);

        ReleaseOwnershipOfEntity<PORTAL>(true);
        st.owning_portal = OppositePortalType<PORTAL>();

        st.cur_touch_call_idx = 0;
        EntityTouchPortal();
        st.cur_touch_call_idx = 1;
        PortalTouchEntity<OppositePortalType<PORTAL>()>();
        st.cur_touch_call_idx = 2;
        PortalTouchEntity<OppositePortalType<PORTAL>()>();
        st.cur_touch_call_idx = 3;
        EntityTouchPortal();

        if (result.dg)
            result.dg->PopNode();
    }
};

void GenerateTeleportChain(const GenerateTeleportChainParams& params, TeleportChainResult& result)
{
    // init internal state
    if (!params._st)
        params._st = std::make_unique<GenerateTeleportChainParams::InternalState>();

    GenerateTeleportChainImpl impl{params, result};
    impl.ResetState();

    if (result.dg)
        result.dg->ResetAndPushRootNode(params.first_tp_from_blue);

    if ((params.recordFlags & TCRF_RECORD_ULP_DIFFS) || params.nudge_to_first_portal_plane) {
        auto [nudged_ent, ulp_diff] =
            NudgeEntityBehindPortalPlane(result.ent,
                                         params.first_tp_from_blue ? params.pp->blue : params.pp->orange,
                                         params.n_max_ulp_nudges);
        if (params.nudge_to_first_portal_plane)
            result.ent = nudged_ent;
        if (params.recordFlags & TCRF_RECORD_ULP_DIFFS)
            result.ulp_diffs_from_portal_plane.push_back(ulp_diff);
        if (ulp_diff.diff_overflow == 1) {
            result.first_ulp_nudge_exceed = true;
            if (result.dg)
                result.dg->Finish();
            return;
        }
    }

    if (params.recordFlags & TCRF_RECORD_ENTITY)
        result.ents.push_back(result.ent);

    if (params.first_tp_from_blue)
        impl.PortalTouchEntity<GenerateTeleportChainParams::InternalStateDefs::FUNC_TP_BLUE>();
    else
        impl.PortalTouchEntity<GenerateTeleportChainParams::InternalStateDefs::FUNC_TP_ORANGE>();

    if (result.dg)
        result.dg->Finish();
}
