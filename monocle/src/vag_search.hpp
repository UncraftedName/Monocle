#pragma once

#include <vector>
#include <assert.h>
#include <optional>

#include "source_math.hpp"
#include "prng.hpp"

struct AABB {
    Vector mins, maxs;

    AABB() {}
    AABB(const Vector& p1, const Vector& p2)
        : mins{fminf(p1[0], p2[0]), fminf(p1[1], p2[1]), fminf(p1[2], p2[2])},
          maxs{fmaxf(p1[0], p2[0]), fmaxf(p1[1], p2[1]), fmaxf(p1[2], p2[2])}
    {}

    bool VectorInBox(const Vector& pt) const
    {
        return pt.x > mins.x && pt.y > mins.y && pt.z > mins.z && pt.x < maxs.x && pt.y < maxs.y && pt.z < maxs.z;
    }

    Vector RandomPtInBox(small_prng& rng) const
    {
        return {rng.next_float(mins[0], maxs[0]), rng.next_float(mins[1], maxs[1]), rng.next_float(mins[2], maxs[2])};
    }
};

enum SearchPortalType {
    SPT_WALL_XP,
    SPT_WALL_XN,
    SPT_WALL_YP,
    SPT_WALL_YN,
    SPT_WALL_ZP,
    SPT_WALL_ZN,
};

struct SearchPortal {
    std::vector<float> lock_opts;
    SearchPortalType type;
    std::vector<AABB> pos_spaces;

    // if the portal is fixed in place
    bool locked;
    Vector locked_pos;
    QAngle locked_ang;

    Portal Generate(small_prng& rng) const
    {
        if (locked)
            return Portal{locked_pos, locked_ang};
        QAngle ang;
        int lock_axis;
        switch (type) {
            case SPT_WALL_XP:
                lock_axis = 0;
                ang = {-0.f, 0.f, 0.f};
                break;
            case SPT_WALL_XN:
                lock_axis = 0;
                ang = {-0.f, 180.f, 0.f};
                break;
            case SPT_WALL_YP:
                lock_axis = 1;
                ang = {-0.f, 90.f, 0.f};
                break;
            case SPT_WALL_YN:
                lock_axis = 1;
                ang = {-0.f, -90.f, 0.f};
                break;
            case SPT_WALL_ZP:
                lock_axis = 2;
                ang = {-90.f, rng.next_float(-180.f, 180.f), 0.f};
                break;
            case SPT_WALL_ZN:
                lock_axis = 2;
                ang = {90.f, rng.next_float(-180.f, 180.f), 0.f};
                break;
            default:
                assert(0);
        }
        float lock_ax_val = rng.next_elem(lock_opts);
        const AABB& pos_space = rng.next_elem(pos_spaces);
        Vector pos{
            lock_axis == 0 ? lock_ax_val : rng.next_float(pos_space.mins[0], pos_space.maxs[0]),
            lock_axis == 1 ? lock_ax_val : rng.next_float(pos_space.mins[1], pos_space.maxs[1]),
            lock_axis == 2 ? lock_ax_val : rng.next_float(pos_space.mins[2], pos_space.maxs[2]),
        };
        return Portal{pos, ang};
    }
};

enum SearchEntryPosFlags {
    SEPF_RP = 1,
    SEPF_RN = 2,
    SEPF_UP = 4,
    SEPF_UN = 8,

    SEPF_LOWER = SEPF_RP | SEPF_RN | SEPF_UN,
    SEPF_UPPER = SEPF_RP | SEPF_RN | SEPF_UP,
    SEPF_ANY = SEPF_LOWER | SEPF_UPPER,
};

struct SearchResult {
    int n_iterations;
    PlacementOrder po;
    Entity ent;
    TeleportChain chain;
    PortalPair pp;

    void print() const
    {
        printf("Search found VAG at iteration %u\nPortal placement order: %s\n",
               n_iterations,
               PlacementOrderStrs[(int)po]);
        pp.PrintNewlocationCmd();
        ent.PrintSetposCmd();
    }
};

struct SearchSpace {
    SearchPortal blue_search;
    SearchPortal orange_search;
    AABB target_space;
    SearchEntryPosFlags entry_pos_search;
    std::vector<PlacementOrder> valid_placement_orders;
    EntityInfo ent_info;
    bool tp_from_blue;
    bool tp_player;

    std::optional<SearchResult> FindVag(small_prng& rng, int n_iterations) const
    {
        SearchResult st{.pp{{}, {}, {}, {}}};
        for (int i = 0; i < n_iterations; i++) {
            st.n_iterations = i;
            st.pp = PortalPair{blue_search.Generate(rng), orange_search.Generate(rng)};
            st.po = rng.next_elem(valid_placement_orders);
            st.pp.CalcTpMatrices(st.po);

            const Portal& p = tp_from_blue ? st.pp.blue : st.pp.orange;
            assert((entry_pos_search & SEPF_ANY) != 0);
            float rm = rng.next_float((entry_pos_search & SEPF_RN) ? -PORTAL_HALF_WIDTH : 0,
                                      (entry_pos_search & SEPF_RP) ? PORTAL_HALF_WIDTH : 0);
            float um = rng.next_float((entry_pos_search & SEPF_UN) ? -PORTAL_HALF_HEIGHT : 0,
                                      (entry_pos_search & SEPF_UP) ? PORTAL_HALF_HEIGHT : 0);
            Vector ent_pos = p.pos + (p.r * rm + p.u * um) * .5f;
            st.ent = tp_player ? Entity{ent_pos} : Entity{ent_pos, 1.f};
            st.chain.Generate(st.pp, tp_from_blue, st.ent, ent_info, 3);

            if (i == 0) {
                printf("sample portals:\n");
                st.pp.PrintNewlocationCmd();
                printf("sample player location:\n");
                st.ent.PrintSetposCmd();
                printf("expected result for %s: ", PlacementOrderStrs[(int)st.po]);
                if (st.chain.max_tps_exceeded)
                    printf("exceeded chain limit\n\n");
                else
                    printf("%d cum teleports\n\n", st.chain.cum_primary_tps);
            }

            if (st.chain.max_tps_exceeded)
                continue;
            if (st.chain.cum_primary_tps != CUM_TP_VAG)
                continue;
            if (!target_space.VectorInBox(st.chain.pts.back()))
                continue;
            return st;
        }
        return {};
    }
};
