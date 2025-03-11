#include <cmath>
#include <algorithm>
#include <stdarg.h>
#include <vector>

#include "chain_graphviz.h"

#include "vag_logic.hpp"

#ifdef CHAIN_WITH_GRAPHVIZ
// clang-format off
struct GvcInit {
    GvcInit() { GvInitGlobalContext(); }
    ~GvcInit() { GvDestroyGlobalContext(); }
} static _gvc_init{};
// clang-format on
#endif

Entity NudgeEntityBehindPortalPlane(const Entity& ent, const Portal& portal, VecUlpDiff* ulp_diff)
{
    int nudge_axis;
    float biggest = -INFINITY;
    for (int i = 0; i < 3; i++) {
        if (fabsf(portal.plane.n[i]) > biggest) {
            biggest = fabsf(portal.plane.n[i]);
            nudge_axis = i;
        }
    }
    VecUlpDiff tmp_diff{};
    VecUlpDiff* diff = ulp_diff ? ulp_diff : &tmp_diff;
    diff->Reset();

    Entity new_ent = ent;

    for (int i = 0; i < 2; i++) {
        float nudge_towards = new_ent.origin[nudge_axis] + portal.plane.n[nudge_axis] * (i ? -10000.f : 10000.f);
        int ulp_diff_incr = i ? -1 : 1;
        constexpr int nudge_limit = 1000000;
        int n_nudges = 0;

        while (portal.ShouldTeleport(new_ent, false) != (bool)i && n_nudges++ < nudge_limit) {
            new_ent.origin[nudge_axis] = std::nextafterf(new_ent.origin[nudge_axis], nudge_towards);
            diff->Update(nudge_axis, ulp_diff_incr);
        }

        // prevent infinite loop
        if (n_nudges >= nudge_limit) {
            assert(0);
            diff->SetInvalid();
            return ent;
        }
    }
    return new_ent;
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
#ifdef CHAIN_WITH_GRAPHVIZ
    do_graphviz = output_graphviz;
    gv_graph = output_graphviz ? GvCreateGraph() : nullptr;
    gv_node_stack = {GvCreateRootNode(gv_graph, tp_from_blue)};
#endif

    // assume we're teleporting from the portal boundary
    ulp_diffs.emplace_back();
    transformed_ent = NudgeEntityBehindPortalPlane(ent, tp_from_blue ? pair.blue : pair.orange, &ulp_diffs.back());
    pre_teleported_ent = transformed_ent;
    pts.push_back(ent.GetCenter());
    tps_queued.push_back(0);

    if (tp_from_blue)
        PortalTouchEntity<FUNC_TP_BLUE>();
    else
        PortalTouchEntity<FUNC_TP_ORANGE>();
    assert(max_tps_exceeded || tp_queue.empty());
    assert(touch_scope_depth == 0);

#ifdef CHAIN_WITH_GRAPHVIZ
    GvWriteGraph(gv_graph, "dot", "chain.dot");
    GvDestroyGraph(gv_graph);
#endif
}

void TeleportChain::CallQueued()
{
    if (max_tps_exceeded)
        return;

    tp_queue.push_back(-++n_queued_nulls);

#ifdef CHAIN_WITH_GRAPHVIZ
    if (do_graphviz) {
        char* cq_label = (char*)_malloca(tp_queue.size() + 1);
        int i = 0;
        for (int q : tp_queue) {
            char symbol;
            if (q == FUNC_TP_BLUE)
                symbol = 'B';
            else if (q == FUNC_TP_ORANGE)
                symbol = 'O';
            else if (q == FUNC_RECHECK_COLLISION)
                symbol = 'R';
            else
                symbol = 'N';
            cq_label[i++] = symbol;
        }
        cq_label[i] = '\0';
        gv_node_stack.push_back(
            GvCreateCallQueuedNode(gv_graph, gv_node_stack.back(), last_should_tp, cq_label, cur_touch_call_idx));
        _freea(cq_label);
        last_should_tp = false;
    }
#endif

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
#ifdef CHAIN_WITH_GRAPHVIZ
    if (do_graphviz)
        gv_node_stack.pop_back();
#endif
}

template <TeleportChain::portal_type PORTAL>
void TeleportChain::ReleaseOwnershipOfEntity(bool moving_to_linked)
{
    if (PORTAL != owning_portal)
        return;
    owning_portal = PORTAL_NONE;
    if (touch_scope_depth > 0)
        for (size_t i = 0; i < ent_info.n_ent_children + !moving_to_linked; i++)
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
            bool player_stuck = transformed_ent.player ? !ent_info.origin_inbounds : false;
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
#ifdef CHAIN_WITH_GRAPHVIZ
        last_should_tp = true;
#endif
        return;
    }

    if (tp_dirs.size() >= max_tps) {
        max_tps_exceeded = true;
#ifdef CHAIN_WITH_GRAPHVIZ
        GvCreateExceededTpNode(gv_graph, gv_node_stack.back(), PORTAL == FUNC_TP_BLUE);
#endif
        return;
    }

    tp_dirs.push_back(PortalIsPrimary<PORTAL>());
    cum_primary_tps += PortalIsPrimary<PORTAL>() ? 1 : -1;
    pp->Teleport(transformed_ent, PORTAL == FUNC_TP_BLUE);
    pts.push_back(transformed_ent.GetCenter());
    tps_queued.push_back(0);

    // calc ulp diff to nearby portal
    ulp_diffs.emplace_back();
    if (cum_primary_tps == 0 || cum_primary_tps == 1) {
        auto& p_ulp_diff_from = (cum_primary_tps == 0) == blue_primary ? pp->blue : pp->orange;
        NudgeEntityBehindPortalPlane(transformed_ent, p_ulp_diff_from, &ulp_diffs.back());
    } else {
        ulp_diffs.back().SetInvalid();
    }

#ifdef CHAIN_WITH_GRAPHVIZ
    if (do_graphviz) {
        assert(!last_should_tp);
        gv_node_stack.push_back(
            GvCreateTeleportNode(gv_graph,
                                 gv_node_stack.back(),
                                 PORTAL == FUNC_TP_BLUE,
                                 cum_primary_tps,
                                 ulp_diffs.back().Valid() ? ulp_diffs.back().diff : INVALID_ULP_DIFF));
    }
#endif

    ReleaseOwnershipOfEntity<PORTAL>(true);
    owning_portal = OppositePortalType<PORTAL>();

#ifdef CHAIN_WITH_GRAPHVIZ
    cur_touch_call_idx = 0;
    EntityTouchPortal();
    cur_touch_call_idx = 1;
    PortalTouchEntity<OppositePortalType<PORTAL>()>();
    cur_touch_call_idx = 2;
    PortalTouchEntity<OppositePortalType<PORTAL>()>();
    cur_touch_call_idx = 3;
    EntityTouchPortal();

    if (do_graphviz)
        gv_node_stack.pop_back();
#else
    EntityTouchPortal();
    PortalTouchEntity<OppositePortalType<PORTAL>()>();
    PortalTouchEntity<OppositePortalType<PORTAL>()>();
    EntityTouchPortal();
#endif
}
