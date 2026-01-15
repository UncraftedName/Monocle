#include <cmath>
#include <algorithm>
#include <stdarg.h>
#include <vector>
#include <fstream>
#include <format>

#include "vag_logic.hpp"

std::pair<Entity, VecUlpDiff> NudgeEntityBehindPortalPlane(const Entity& ent, const Portal& portal, size_t n_max_nudges)
{
    int nudge_axis = 0;
    float biggest = -INFINITY;
    for (int i = 0; i < 3; i++) {
        if (fabsf(portal.plane.n[i]) > biggest) {
            biggest = fabsf(portal.plane.n[i]);
            nudge_axis = i;
        }
    }
    VecUlpDiff ulp_diff{};

    Entity nudged_ent = ent;

    for (int i = 0; i < 2; i++) {
        Vector& ent_pos = nudged_ent.GetPosRef();
        float nudge_towards = ent_pos[nudge_axis] + portal.plane.n[nudge_axis] * (i ? -10000.f : 10000.f);
        int ulp_diff_incr = i ? -1 : 1;
        size_t n_nudges = 0;

        while (portal.ShouldTeleport(nudged_ent, false) != (bool)i && n_nudges++ < n_max_nudges) {
            ent_pos[nudge_axis] = std::nextafterf(ent_pos[nudge_axis], nudge_towards);
            ulp_diff.Update(nudge_axis, ulp_diff_incr);
        }

        // prevent infinite loop
        if (n_nudges >= n_max_nudges) {
            ulp_diff.diff_overflow = 1;
            return {nudged_ent, ulp_diff};
        }
    }
    return {nudged_ent, ulp_diff};
}

using CHAIN_DEFS = TeleportChainParams::InternalStateDefs;

struct TeleportChainParams::InternalState {
    /*
    * CPortalTouchScope::m_CallQueue. Holds values representing:
    * - FUNC_TP_BLUE: a blue teleport
    * - FUNC_TP_ORANGE: an orange teleport
    * - FUNC_RECHECK_COLLISION: RecheckEntityCollision
    * - negative values: a null
    */
    CHAIN_DEFS::queue_type tp_queue;
    // total number of nulls queued so far, only for debugging
    CHAIN_DEFS::queue_entry n_queued_nulls;
    int touch_scope_depth; // CPortalTouchScope::m_nDepth, not fully implemented

    CHAIN_DEFS::portal_type owning_portal;
};

TeleportChainParams::TeleportChainParams(const PortalPair* pp, Entity ent)
    : pp(pp),
      ent(ent),
      first_tp_from_blue(pp ? (pp->blue.plane.DistTo(ent.GetCenter()) < pp->orange.plane.DistTo(ent.GetCenter()))
                            : true)
{}

TeleportChainParams::~TeleportChainParams() {}

/*
* This is a simplified replicate of the game's logic for teleporting stuff. The source is in e.g.
* - CProp_Portal::ShouldTeleportTouchingEntity
* - CProp_Portal::TeleportTouchingEntity
* - CProp_Portal::Touch
* etc.
* 
* Most of the functions are templated here for debugging - the call stack will have e.g.
* GenerateTeleportChainImpl::TeleportEntity<1>           // blue portal
* GenerateTeleportChainImpl::ReleaseOwnershipOfEntity<2> // orange portal
*/
struct GenerateTeleportChainImpl {

    const TeleportChainParams& usrParams;
    TeleportChainParams::InternalState& st;
    TeleportChainResult& result;

    GenerateTeleportChainImpl(const TeleportChainParams& params, TeleportChainResult& result)
        : usrParams(params),
          st(params._st ? *params._st : *(params._st = std::make_unique<TeleportChainParams::InternalState>())),
          result(result)
    {}

    void ResetState()
    {
        // clear internal state

        st.tp_queue.clear();
        st.n_queued_nulls = 0;
        st.touch_scope_depth = 0;
        st.owning_portal = usrParams.first_tp_from_blue ? CHAIN_DEFS::FUNC_TP_BLUE : CHAIN_DEFS::FUNC_TP_ORANGE;

        // clear result

        result.max_tps_exceeded = false;
        result.first_ulp_nudge_exceed = false;
        result.total_n_teleports = 0;
        result.cum_teleports = 0;
        result.ent = usrParams.ent;
        result.ents.clear();
        result.ulp_diffs_from_portal_plane.clear();
        result.tp_dirs.clear();
    }

    template <CHAIN_DEFS::portal_type PORTAL>
    static constexpr CHAIN_DEFS::portal_type OppositePortalType()
    {
        if constexpr (PORTAL == CHAIN_DEFS::FUNC_TP_BLUE)
            return CHAIN_DEFS::FUNC_TP_ORANGE;
        else
            return CHAIN_DEFS::FUNC_TP_BLUE;
    }

    template <CHAIN_DEFS::portal_type PORTAL>
    inline bool PortalIsPrimary()
    {
        return usrParams.first_tp_from_blue == (PORTAL == CHAIN_DEFS::FUNC_TP_BLUE);
    }

    template <CHAIN_DEFS::portal_type PORTAL>
    inline const Portal& GetPortal()
    {
        return PORTAL == CHAIN_DEFS::FUNC_TP_BLUE ? usrParams.pp->blue : usrParams.pp->orange;
    }

    void CallQueued()
    {
        if (result.max_tps_exceeded)
            return;

        st.tp_queue.push_back(-++st.n_queued_nulls);

        if (result.dg) {
            result.dg->PushCallQueuedNode(st.tp_queue);
            result.dg->SetLastTeleportCallQueuedTeleport(false);
        }

        for (bool dequeued_null = false; !dequeued_null && !result.max_tps_exceeded;) {
            int val = st.tp_queue.front();
            st.tp_queue.pop_front();
            switch (val) {
                case CHAIN_DEFS::FUNC_RECHECK_COLLISION:
                    break;
                case CHAIN_DEFS::FUNC_TP_BLUE:
                    TeleportEntity<CHAIN_DEFS::FUNC_TP_BLUE>();
                    break;
                case CHAIN_DEFS::FUNC_TP_ORANGE:
                    TeleportEntity<CHAIN_DEFS::FUNC_TP_ORANGE>();
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

    template <CHAIN_DEFS::portal_type PORTAL>
    void ReleaseOwnershipOfEntity(bool moving_to_linked)
    {
        if (PORTAL != st.owning_portal)
            return;
        st.owning_portal = CHAIN_DEFS::PORTAL_NONE;
        if (st.touch_scope_depth > 0)
            for (int i = 0; i < result.ent.n_children + !moving_to_linked; i++)
                st.tp_queue.push_back(CHAIN_DEFS::FUNC_RECHECK_COLLISION);
    }

    template <CHAIN_DEFS::portal_type PORTAL>
    bool SharedEnvironmentCheck()
    {
        if (st.owning_portal == CHAIN_DEFS::PORTAL_NONE || st.owning_portal == PORTAL)
            return true;
        Vector ent_center = result.ent.GetCenter();
        return ent_center.DistToSqr(GetPortal<PORTAL>().pos) <
               ent_center.DistToSqr(GetPortal<OppositePortalType<PORTAL>()>().pos);
    }

    template <CHAIN_DEFS::portal_type PORTAL>
    void PortalTouchEntity()
    {
        if (result.max_tps_exceeded)
            return;
        ++st.touch_scope_depth;
        if (st.owning_portal != PORTAL) {
            if (SharedEnvironmentCheck<PORTAL>()) {
                bool ent_in_front =
                    GetPortal<PORTAL>().plane.n.Dot(result.ent.GetCenter()) > GetPortal<PORTAL>().plane.d;
                bool player_stuck = result.ent.is_player ? !usrParams.map_origin_inbounds : false;
                if (ent_in_front || player_stuck) {
                    if (st.owning_portal != PORTAL && st.owning_portal != CHAIN_DEFS::PORTAL_NONE)
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

    template <CHAIN_DEFS::portal_type PORTAL>
    void TeleportEntity()
    {
        if (st.touch_scope_depth > 0) {
            st.tp_queue.push_back(PORTAL);
            if (result.dg)
                result.dg->SetLastTeleportCallQueuedTeleport(true);
            return;
        }

        if (result.total_n_teleports >= usrParams.n_max_teleports) {
            result.max_tps_exceeded = true;
            if (result.dg)
                result.dg->PushExceededTpNode(PORTAL == CHAIN_DEFS::FUNC_TP_BLUE);
            return;
        }

        ++result.total_n_teleports;

        result.cum_teleports += PortalIsPrimary<PORTAL>() ? 1 : -1;
        result.ent = usrParams.pp->Teleport(result.ent, PORTAL == CHAIN_DEFS::FUNC_TP_BLUE);
        if (usrParams.record_flags & TCRF_RECORD_ENTITY)
            result.ents.push_back(result.ent);
        if (usrParams.record_flags & TCRF_RECORD_TP_DIRS)
            result.tp_dirs.push_back(PortalIsPrimary<PORTAL>());

        int dgPlaneSide = 0;

        if (usrParams.record_flags & TCRF_RECORD_ULP_DIFFS) {
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

        if (result.dg)
            result.dg->PushTeleportNode(PORTAL == CHAIN_DEFS::FUNC_TP_BLUE, result.cum_teleports, dgPlaneSide);

        ReleaseOwnershipOfEntity<PORTAL>(true);
        st.owning_portal = OppositePortalType<PORTAL>();

        // do the 4 touch calls
        for (int i = 0; i < 4; i++) {
            if (result.dg)
                result.dg->SetTouchCallIndex(i);
            if (i == 0 || i == 3)
                EntityTouchPortal();
            else
                PortalTouchEntity<OppositePortalType<PORTAL>()>();
        }

        if (result.dg)
            result.dg->PopNode();
    }
};

void GenerateTeleportChain(const TeleportChainParams& params, TeleportChainResult& result)
{
    assert(!!params.pp);

    GenerateTeleportChainImpl impl{params, result};
    impl.ResetState();

    if (result.dg)
        result.dg->ResetAndPushRootNode(params.first_tp_from_blue);

    if ((params.record_flags & TCRF_RECORD_ULP_DIFFS) || params.nudge_to_first_portal_plane) {
        auto [nudged_ent, ulp_diff] =
            NudgeEntityBehindPortalPlane(result.ent,
                                         params.first_tp_from_blue ? params.pp->blue : params.pp->orange,
                                         params.n_max_ulp_nudges);
        if (params.nudge_to_first_portal_plane) {
            if (ulp_diff.diff_overflow) {
                result.first_ulp_nudge_exceed = true;
                if (result.dg)
                    result.dg->Finish();
                return;
            }
            result.ent = nudged_ent;
        }
        if (params.record_flags & TCRF_RECORD_ULP_DIFFS)
            result.ulp_diffs_from_portal_plane.push_back(ulp_diff);
    }

    if (params.record_flags & TCRF_RECORD_ENTITY)
        result.ents.push_back(result.ent);

    if (params.first_tp_from_blue)
        impl.PortalTouchEntity<CHAIN_DEFS::FUNC_TP_BLUE>();
    else
        impl.PortalTouchEntity<CHAIN_DEFS::FUNC_TP_ORANGE>();

    if (result.dg)
        result.dg->Finish();
}

std::string TeleportChainResult::CreateDebugString(const TeleportChainParams& params) const
{
    if (!(params.record_flags & (TCRF_RECORD_TP_DIRS | TCRF_RECORD_ENTITY)))
        return std::string(__FUNCTION__) + ": TCRF_RECORD_TP_DIRS | TCRF_RECORD_ENTITY is required";

    std::string out;

    int min_cum = 0, max_cum = 0, cum = 0;
    for (bool dir : tp_dirs) {
        cum += dir ? 1 : -1;
        min_cum = cum < min_cum ? cum : min_cum;
        max_cum = cum > max_cum ? cum : max_cum;
    }
    int left_pad = ents.size() <= 1 ? 1 : (int)std::floor(std::log10(ents.size() - 1)) + 1;
    cum = 0;
    for (size_t i = 0; i <= tp_dirs.size(); i++) {
        std::format_to(std::back_inserter(out), "{:{}d}) ", i, left_pad);
        for (int c = min_cum; c <= max_cum; c++) {

            bool should_teleport = false;
            if (c == 0 || c == 1) {
                const Portal& p = params.first_tp_from_blue == !!c ? params.pp->orange : params.pp->blue;
                should_teleport = p.ShouldTeleport(ents[i], false);
            }

            if (c == cum) {
                if (c == 0)
                    out += should_teleport ? "0|>" : ".|0";
                else if (c == 1)
                    out += should_teleport ? "<|1" : "1|.";
                else if (c < 0)
                    std::format_to(std::back_inserter(out), "{}.", c);
                else
                    std::format_to(std::back_inserter(out), ".{}.", c);
            } else {
                if (c == 0)
                    out += ".|>";
                else if (c == 1)
                    out += "<|.";
                else
                    out += "...";
            }
        }
        out += '\n';
        if (i < tp_dirs.size())
            cum += tp_dirs[i] ? 1 : -1;
    }

    return out;
}
