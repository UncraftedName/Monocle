#include <cmath>
#include <algorithm>
#include <stdarg.h>
#include <vector>
#include <fstream>
#include <format>

#include "vag_logic.hpp"
#include "ulp_diff.hpp"

std::pair<Entity, PointToPortalPlaneUlpDist> ProjectEntityToPortalPlane(const Entity& ent, const Portal& portal)
{
    const VPlane& plane = portal.plane;

    uint32_t ax = 0;
    for (int i = 1; i < 3; i++)
        if (std::fabsf(plane.n[i]) > std::fabsf(plane.n[ax]))
            ax = i;

    Vector old_center = ent.GetCenter();
    Entity proj_ent = ent;
    float& new_ax_val = proj_ent.GetPosRef()[ax]; // the component we're nudging
    float old_ax_val = new_ax_val;
    /*
    * plane: ax+by+cz=d, center: <i,j,k>
    * If e.g. we're projecting along the x-axis, then: x = (d-by-cz)/a = old_x + (d - plane*center) / a.
    * We're trying to project the ent center onto the plane, but for e.g. the player we must adjust
    * the origin, so use the difference of centers to adjust PosRef.
    */
    float new_center_ax_val = (float)(old_center[ax] + (plane.d - plane.n.Dot(old_center)) / plane.n[ax]);
    new_ax_val += new_center_ax_val - old_center[ax];
    /*
    * Mathematically, the entity is on the plane now. But we want to make sure it's as close as
    * possible. Move along the plane normal until we're in front, then move back.
    */
    for (int same_as_plane_norm = 1; same_as_plane_norm >= 0; same_as_plane_norm--) {
        float nudge_towards = !!same_as_plane_norm == std::signbit(plane.n[ax]) ? -INFINITY : INFINITY;
        while (portal.ShouldTeleport(proj_ent, false) == !!same_as_plane_norm)
            new_ax_val = std::nextafterf(new_ax_val, nudge_towards);
    }

    uint32_t ulp_diff = mon::ulp::UlpDiffF(new_ax_val, old_ax_val);
    PointToPortalPlaneUlpDist dist_info{
        .n_ulps = ulp_diff,
        .ax = ax,
        .pt_was_behind_portal = ulp_diff == 0 || std::signbit(new_ax_val - old_ax_val) == std::signbit(plane.n[ax]),
        .is_valid = true,
    };
    return {proj_ent, dist_info};
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
        result.total_n_teleports = 0;
        result.cum_teleports = 0;
        result.ent = usrParams.ent;
        result.ents.clear();
        result.portal_plane_diffs.clear();
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

        if (usrParams.record_flags & TCRF_RECORD_PLANE_DIFFS) {
            PointToPortalPlaneUlpDist plane_dist{.is_valid = false};
            if (result.cum_teleports == 0 || result.cum_teleports == 1) {
                auto& p_plane_diff_from = (result.cum_teleports == 0) == usrParams.first_tp_from_blue
                                          ? usrParams.pp->blue
                                          : usrParams.pp->orange;
                plane_dist = ProjectEntityToPortalPlane(result.ent, p_plane_diff_from).second;
                if (plane_dist.is_valid)
                    dgPlaneSide = plane_dist.pt_was_behind_portal ? -1 : 1;
            }
            result.portal_plane_diffs.push_back(plane_dist);
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

    if ((params.record_flags & TCRF_RECORD_PLANE_DIFFS) || params.project_to_first_portal_plane) {
        auto [nudged_ent, plane_diff] =
            ProjectEntityToPortalPlane(result.ent, params.first_tp_from_blue ? params.pp->blue : params.pp->orange);
        if (params.project_to_first_portal_plane)
            result.ent = nudged_ent;
        if (params.record_flags & TCRF_RECORD_PLANE_DIFFS)
            result.portal_plane_diffs.push_back(plane_diff);
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
