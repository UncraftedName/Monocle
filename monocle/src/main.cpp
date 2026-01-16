#include "source_math.hpp"
#include "vag_logic.hpp"
#include "prng.hpp"
#include "tga.hpp"
#include "ctpl_stl.h"
#include "time_scope.hpp"
#include "vag_search.hpp"

#include <iostream>
#include <algorithm>
#include <ranges>
#include <numeric>
#include <fstream>
#include <vector>

enum PITCH_YAW_TYPE {
    PYT_WALL_ALIGNED,
    PYT_WALL_ANY,
    PYT_FLOOR_CEIL_ALIGNED,
    PYT_FLOOR_CEIL_ANY,
    PYT_POSSIBLE,
    PYT_ANY,
    PYT_COUNT,
};

static std::array<const char*, PYT_COUNT> PITCH_YAW_STRS{
    "wall_aligned",
    "wall_any",
    "floor_ceil_aligned",
    "floor_ceil_any",
    "any_possible",
    "any",
};

static mon::Vector RandomPos(small_prng& rng, size_t oct_bits, size_t dist_scale)
{
    float a = 1 << dist_scale;
    float b = 1 << (dist_scale + 1);
    return {
        rng.next_float(a, b) * (oct_bits & 1 ? 1.f : -1.f),
        rng.next_float(a, b) * (oct_bits & 2 ? 1.f : -1.f),
        rng.next_float(a, b) * (oct_bits & 4 ? 1.f : -1.f),
    };
}

static mon::QAngle RandomAng(small_prng& rng, PITCH_YAW_TYPE type, bool has_roll)
{
    float p, y;
    switch (type) {
        case PYT_WALL_ALIGNED:
            p = 0.f;
            y = rng.next_int(-2, 2) * 90.f;
            break;
        case PYT_WALL_ANY:
            p = 0.f;
            y = rng.next_float(-180.f, 180.f);
            break;
        case PYT_FLOOR_CEIL_ALIGNED:
            p = rng.next_bool() ? -90.f : 90.f;
            y = rng.next_int(-2, 2) * 90.f;
            break;
        case PYT_FLOOR_CEIL_ANY:
            p = rng.next_bool() ? -90.f : 90.f;
            y = rng.next_float(-180.f, 180.f);
            break;
        case PYT_POSSIBLE:
            p = rng.next_float(-90.f, 90.f);
            y = rng.next_float(-180.f, 180.f);
            break;
        case PYT_ANY:
            p = rng.next_float(-180.f, 180.f);
            y = rng.next_float(-180.f, 180.f);
            break;
        case PYT_COUNT:
        default:
            p = y = 0.f;
            MON_ASSERT(0);
    }
    return {p, y, has_roll ? rng.next_float(-180.f, 180.f) : 0.f};
}

static void GenerateResultsDistributionsToFile()
{
    enum RS_AX {
        RESULT_TP,
        RESULT_VAG,
        RESULT_UNKNOWN,
        RESULT_COUNT,
    };

    small_prng rng{};
    mon::TeleportChainParams params;
    mon::TeleportChainResult result;
    const int n_runs_per_combination = 10000;

    std::ofstream of{"results.txt"};
    of << "{\n";
    of << "\"n_runs_per_iteration\": " << n_runs_per_combination << ",\n";
    of << "\"result_keys\": [\"normal_teleport\", \"VAG\", \"unknown\"],\n";
    of << "\"runs\": [";

    std::array pos_scales{6, 7, 8, 9};
    std::array<int, PYT_COUNT> py_types;
    std::array roll_opts{false, true};
    // not per-portal
    std::array<int, (int)mon::PlacementOrder::COUNT> mat_opts;

    std::iota(py_types.begin(), py_types.end(), 0);
    std::iota(mat_opts.begin(), mat_opts.end(), 0);

    auto portal_cp = std::ranges::views::cartesian_product(pos_scales, py_types, roll_opts);
    auto misc_cp = std::ranges::views::cartesian_product(mat_opts);
    auto all_opts = std::ranges::views::cartesian_product(portal_cp, portal_cp, misc_cp);

    size_t n_complete = 0;
    size_t n_combinations = (size_t)all_opts.size();

    for (const auto& [blue_opts, orange_opts, misc_opts] : all_opts) {

        auto bp = std::get<0>(blue_opts);

        int results[RESULT_COUNT]{};

        for (int i = 0; i < n_runs_per_combination; i++) {
            mon::PortalPair pp{
                RandomPos(rng, rng.next_int(0, 8), std::get<0>(blue_opts)),
                RandomAng(rng, (PITCH_YAW_TYPE)std::get<1>(blue_opts), std::get<2>(blue_opts)),
                RandomPos(rng, rng.next_int(0, 8), std::get<0>(orange_opts)),
                RandomAng(rng, (PITCH_YAW_TYPE)std::get<1>(orange_opts), std::get<2>(orange_opts)),
                (mon::PlacementOrder)std::get<0>(misc_opts),
            };
            mon::Vector ent_pos =
                pp.blue.pos + pp.blue.r * rng.next_float(-mon::PORTAL_HALF_WIDTH * .5f, mon::PORTAL_HALF_WIDTH * .5f) +
                pp.blue.u * rng.next_float(-mon::PORTAL_HALF_HEIGHT * .5f, mon::PORTAL_HALF_HEIGHT * .5f);

            params.pp = &pp;
            params.ent = mon::Entity::CreatePlayerFromCenter(ent_pos, true);
            params.map_origin_inbounds = false;
            params.first_tp_from_blue = true;
            params.n_max_teleports = 3;
            params.record_flags = mon::TCRF_NONE;

            GenerateTeleportChain(params, result);

            if (result.max_tps_exceeded)
                results[RESULT_UNKNOWN]++;
            else if (result.cum_teleports == 1)
                results[RESULT_TP]++;
            else if (result.cum_teleports == -1)
                results[RESULT_VAG]++;
            else
                MON_ASSERT(0);
        }
        of << (n_complete ? ",\n" : "\n") << "{\n\t\"label\":\n\t{\n";
        for (int i = 0; i < 2; i++) {
            auto& opts = i ? blue_opts : orange_opts;
            const char* prefix = i ? "\t\t\"blue." : "\t\t\"orange.";
            of << prefix << "pos_scale\": " << std::get<0>(opts) << ",\n";
            of << prefix << "pitch_yaw_type\": \"" << PITCH_YAW_STRS[std::get<1>(opts)] << "\",\n";
            of << prefix << "has_roll\": " << (std::get<2>(opts) ? "true" : "false") << ",\n";
        }
        of << "\t\t\"matrix\": \"" << mon::PlacementOrderStrs[(int)std::get<0>(misc_opts)] << "\"";
        of << "\n\t},\n\t\"results\": [";
        for (int i = 0; i < RESULT_COUNT; i++)
            of << ((double)results[i] / n_runs_per_combination) << (i == RESULT_COUNT - 1 ? "]\n}" : ", ");

        printf("finished %u/%u runs\n", ++n_complete, n_combinations);
    }

    of << "\n]\n}\n";
}

static void CreateOverlayPortalImage(const mon::PortalPair& pair,
                                     const char* file_name,
                                     size_t y_res,
                                     bool from_blue,
                                     bool rand_nudge = false)
{
    TIME_FUNC();

    const mon::Portal& p = from_blue ? pair.blue : pair.orange;
    size_t x_res = (size_t)((double)y_res * mon::PORTAL_HALF_WIDTH / mon::PORTAL_HALF_HEIGHT);
    struct pixel {
        uint8_t b, g, r, a;
    };
    std::vector<pixel> pixels{y_res * x_res};
    int n_threads = std::thread::hardware_concurrency();
    ctpl::thread_pool pool{n_threads ? n_threads : 4};
    for (size_t y = 0; y < y_res; y++) {

        float oy = mon::PORTAL_HALF_HEIGHT * (-1 + 1.f / y_res);
        float ty = (float)y / (y_res - 1);
        float my = oy * (1 - 2 * ty);
        mon::Vector u_off = p.u * my;

        pool.push([x_res, u_off, y, from_blue, &p, &pair, &pixels, rand_nudge](int) -> void {
            small_prng rng{y};
            mon::TeleportChainParams params;
            mon::TeleportChainResult result;
            for (size_t x = 0; x < x_res; x++) {
                float rx = rand_nudge ? rng.next_float(-.1f, .1f) : 0.f;
                float ox = mon::PORTAL_HALF_WIDTH * (-1 + 1.f / x_res);
                float tx = ((float)x + rx) / (x_res - 1);
                float mx = ox * (1 - 2 * tx);

                mon::Vector r_off = p.r * mx;
                params.pp = &pair;
                params.ent = mon::Entity::CreatePlayerFromCenter(p.pos + r_off + u_off, true);
                params.map_origin_inbounds = false;
                params.n_max_teleports = 10;
                params.first_tp_from_blue = from_blue;
                params.record_flags = mon::TCRF_NONE;

                GenerateTeleportChain(params, result);

                pixel& pix = pixels[x_res * y + x];
                pix.a = 255;
                if (result.max_tps_exceeded)
                    pix.r = pix.g = pix.b = 0;
                else if (result.cum_teleports == 0)
                    pix.r = pix.g = pix.b = 125;
                else if (result.cum_teleports == 1)
                    pix.r = pix.g = pix.b = 255;
                else if (result.cum_teleports < 0 && result.cum_teleports >= -3)
                    pix.r = 85 * -result.cum_teleports;
                else if (result.cum_teleports > 1 && result.cum_teleports <= 4)
                    pix.g = 85 * (result.cum_teleports - 1);
                else
                    pix.b = 255;
            }
        });
    }
    pool.stop(true);
    TIME_SCOPE("TGA_WRITE");
    tga_write(file_name, x_res, y_res, (uint8_t*)pixels.data(), 4, 3);
}

static void FindVagIn04()
{
    TIME_FUNC();

    small_prng rng{0};
    mon::TeleportChainParams params;
    mon::TeleportChainResult result;

    float xmin = -75.70868f;
    float xmax = 145.85846f;
    float ymin = 311.f;
    float ymax = 350.70612f;
    float yaw_min = -6;
    float yaw_max = 28;

    mon::Vector target_mins{-30.f, 706.f, 566.5f};
    mon::Vector target_maxs{223.f, 958.f, 700.f};

    for (int i = 0; i < 100000; i++) {
        mon::PortalPair pp{
            {rng.next_float(xmin, xmax), rng.next_float(ymin, ymax), 255.96877},
            {90, rng.next_float(yaw_min, yaw_max), 0},
            {-448, 0.03125, 54.9502},
            {0, 90, 0},
            mon::PlacementOrder::BLUE_WAS_CLOSED_ORANGE_OPENED,
        };
        float r = rng.next_float(-mon::PORTAL_HALF_WIDTH * 0.5f, mon::PORTAL_HALF_WIDTH * 0.5f);
        float u = rng.next_float(-mon::PORTAL_HALF_HEIGHT * 0.5f, mon::PORTAL_HALF_HEIGHT * 0.5f);

        params.pp = &pp;
        params.ent = mon::Entity::CreatePlayerFromCenter(pp.blue.pos + pp.blue.r * r + pp.blue.u * u, true);
        params.map_origin_inbounds = true;
        params.first_tp_from_blue = true;
        params.n_max_teleports = 3;

        GenerateTeleportChain(params, result);

        if (result.max_tps_exceeded)
            continue;
        if (result.cum_teleports != -1)
            continue;
        mon::Vector t = result.ent.GetCenter();
        if (t.x < target_mins.x || t.y < target_mins.y || t.z < target_mins.z)
            continue;
        if (t.x > target_maxs.x || t.y > target_maxs.y || t.z > target_maxs.z)
            continue;
        printf("%s\n", pp.NewLocationCmd().c_str());
        printf("%s\n", result.ent.SetPosCmd().c_str());
        const char* file_name = "04_blue.tga";
        printf("Found portal, generating overlay image\n");
        CreateOverlayPortalImage(pp, file_name, 1000, true);
        break;
    }
}

static void FindVag18StartCeilCubeRoom()
{
    mon::SearchSpace ss{
        .blue_search{
            .lock_opts{1023.96875f, 1023.9687f, 1023.9686f},
            .type = mon::SPT_WALL_ZN,
            .pos_spaces{mon::AABB{{605, -230, 1075}, {-90, -20, 930}}},
        },
        .orange_search{
            .lock_opts{2559.9688f},
            .type = mon::SPT_WALL_XN,
            .pos_spaces{
                mon::AABB{{2556, 284, 1278}, {2532, 611, 586}},
                mon::AABB{{2532, 611, 586}, {2624, 1522, 761}},
                mon::AABB{{2624, 1522, 761}, {2568, 1182, 1268}},
            },
        },
        .target_space = mon::AABB{{-1022, 620, 3310}, {-1395, 877, 3540}},
        .entry_pos_search = mon::SEPF_ANY,
        .valid_placement_orders{
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
            mon::PlacementOrder::BLUE_OPEN_ORANGE_NEW_LOCATION,
        },
        .tp_from_blue = true,
        .tp_player = true,
    };
    small_prng rng{0};
    auto result = ss.FindVag(rng, 100000);
    if (!result) {
        printf("no VAG found\n");
        return;
    }
    (*result).print();
    printf("generating overlay image...\n");
    CreateOverlayPortalImage(result->pp, __FUNCTION__ ".tga", 1000, true);
}

static void FindComplexChain()
{
    small_prng rng{0};
    mon::AABB pos_space{{30, 30, 750}, {400, 400, 1000}};
    mon::TeleportChainParams params;
    mon::TeleportChainResult result;

    for (int i = 0; i < 1000000; i++) {
        mon::PortalPair pp{
            pos_space.RandomPtInBox(rng),
            {0, rng.next_int(-2, 2) * 90.f, 0},
            pos_space.RandomPtInBox(rng),
            {0, rng.next_int(-2, 2) * 90.f, 0},
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
        };
        if (pp.blue.pos.DistToSqr(pp.orange.pos) < 200 * 200)
            continue;

        params.pp = &pp;
        params.ent = mon::Entity::CreatePlayerFromCenter(pp.blue.pos, true);
        params.map_origin_inbounds = true;
        params.n_max_teleports = 10;
        params.first_tp_from_blue = true;

        GenerateTeleportChain(params, result);

        if (result.max_tps_exceeded)
            continue;
        if (result.cum_teleports >= -1 && result.cum_teleports <= 1)
            continue;
        printf("iteration %u, %u teleports, %d cum teleports\n", i, result.tp_dirs.size(), result.cum_teleports);
        printf("%s\n", pp.NewLocationCmd().c_str());
        printf("%s\n", result.ent.SetPosCmd().c_str());
        printf("generating overlay image...\n");
        CreateOverlayPortalImage(pp, "complex_chain.tga", 1000, true);
        break;
    }
}

static void DebugInfinite09Chain()
{
    (void)freopen("vs_2022_infinite.txt", "w", stdout);

    mon::PortalPair pp{
        {255.96875f, -161.01295f, 201.96875f},
        {-0.f, 180.f, 0.f},
        {-127.96875f, -191.24300f, 182.03125f},
        {0.f, 0.f, 0.f},
        mon::PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED,
    };

    mon::TeleportChainParams params{
        &pp,
        mon::Entity::CreatePlayerFromCenter({-127.96876f, -191.24300f, 182.03125f}, true),
    };
    params.n_max_teleports = 5000;
    mon::TeleportChainResult result;

    mon::GenerateTeleportChain(params, result);
}

static void CreateSpinAnimation()
{
    small_prng rng{20};
    mon::AABB pos_space{{30, 30, 750}, {400, 400, 1000}};
    mon::TeleportChainParams params;
    mon::TeleportChainResult result;

    for (int i = 0; i < 100000; i++) {
        mon::PortalPair pp{
            pos_space.RandomPtInBox(rng),
            {rng.next_float(-180.f, 180.f), -180.f, 0},
            pos_space.RandomPtInBox(rng),
            {rng.next_float(-180.f, 180.f), rng.next_float(-180.f, 180.f), 0},
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
        };
        if (pp.blue.pos.DistToSqr(pp.orange.pos) < 100 * 100)
            continue;

        params.pp = &pp;
        params.ent = mon::Entity::CreatePlayerFromCenter(pp.blue.pos + pp.blue.r, true);
        params.n_max_teleports = 10;
        params.first_tp_from_blue = true;

        GenerateTeleportChain(params, result);

        if (result.max_tps_exceeded || result.cum_teleports != 1)
            continue;
        params.ent = mon::Entity::CreatePlayerFromCenter(pp.blue.pos - pp.blue.r, true);
        GenerateTeleportChain(params, result);

        if (result.max_tps_exceeded || result.cum_teleports != -1)
            continue;
        for (int i = -180; i < 180; i++) {
            mon::PortalPair pp2{pp.blue.pos, {pp.blue.ang.x, (float)i, 0}, pp.orange.pos, pp.orange.ang, pp.order};
            char name[32];
            sprintf(name, "spin_anim/ang_%03d.tga", (360 + (i % 360)) % 360);
            CreateOverlayPortalImage(pp2, name, 350, true);
        }
        break;
    }
}

static void FindInfiniteChain()
{
    small_prng rng{0};
    mon::AABB pos_space{{30, 30, 750}, {400, 400, 1000}};
    mon::TeleportChainParams params;
    mon::TeleportChainResult result;

    for (int i = 0; i < 100000; i++) {
        mon::PortalPair pp{
            pos_space.RandomPtInBox(rng),
            {0, rng.next_int(-2, 2) * 90.f, 0},
            pos_space.RandomPtInBox(rng),
            {0, rng.next_int(-2, 2) * 90.f, 0},
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
        };
        if (pp.blue.pos.DistToSqr(pp.orange.pos) < 100 * 100)
            continue;

        params.pp = &pp;
        params.ent = mon::Entity::CreatePlayerFromCenter(pp.blue.pos, true);
        params.n_max_teleports = 400000;
        params.first_tp_from_blue = true;

        mon::GenerateTeleportChain(params, result);

        if (!result.max_tps_exceeded)
            continue;
        printf("%s\n", pp.NewLocationCmd().c_str());
        printf("%s\n", params.ent.SetPosCmd().c_str());
        break;
    }
}

static void FindFiniteChainThatGivesNFE()
{
    small_prng rng{0};
    mon::AABB pos_space{{30, 30, 750}, {400, 400, 1000}};
    mon::TeleportChainParams params;
    mon::TeleportChainResult result;

    for (int i = 0; i < 1000000; i++) {
        mon::PortalPair pp{
            pos_space.RandomPtInBox(rng),
            {0, rng.next_int(-2, 2) * 90.f, 0},
            pos_space.RandomPtInBox(rng),
            {0, rng.next_int(-2, 2) * 90.f, 0},
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
        };
        if (pp.blue.pos.DistToSqr(pp.orange.pos) < 100 * 100)
            continue;

        params.pp = &pp;
        params.ent = mon::Entity::CreatePlayerFromCenter(pp.blue.pos, true);
        params.n_max_teleports = 34;
        params.map_origin_inbounds = false;
        params.first_tp_from_blue = true;

        GenerateTeleportChain(params, result);

        if (result.max_tps_exceeded)
            continue;
        if (result.cum_teleports != 0)
            continue;
        auto [nudged_ent, plane_dist] = ProjectEntityToPortalPlane(result.ent, pp.blue);
        if (!plane_dist.pt_was_behind_portal)
            continue;
        printf("found chain of length %u on iteration %d\n", result.tp_dirs.size(), i);
        printf("%s\n", pp.NewLocationCmd().c_str());
        printf("%s\n", mon::Entity::CreatePlayerFromCenter(result.ents[0].GetCenter(), true).SetPosCmd().c_str());
        break;
    }
}

static void FindVagIn09EleTop()
{
    mon::SearchSpace ss{
        .blue_search{
            .lock_opts{
                0.0312423706f,
                0.0312497951f,
                0.0312499776f,
                0.03125f,
                0.0312501229f,
                0.0312502459f,
                0.0312504061f,
                0.0312505625f,
                0.0312515162f,
            },
            .type = mon::SPT_WALL_ZP,
            .pos_spaces = {mon::AABB{{101, 771, 0}, {-198, 880, 12}}},
        },
        .orange_search{
            .locked = true,
            .locked_pos{-127.96875f, -191.242996f, 182.03125f},
            .locked_ang{0, 0, 0},
        },
        .target_space = mon::AABB{{-259, 752, 568}, {391, 946, 740}},
        .entry_pos_search = mon::SEPF_LOWER,
        .valid_placement_orders{
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
            mon::PlacementOrder::AFTER_LOAD,
        },
        .tp_from_blue = false,
        .tp_player = true,
    };
    small_prng rng{0};
    auto sr = ss.FindVag(rng, 1000000);
    if (sr)
        sr->print();
}

static void FindVagIn09EleBottom()
{
    mon::SearchSpace ss{
        .blue_search{
            .lock_opts{
                0.0312423706f,
                0.0312497951f,
                0.0312499776f,
                0.03125f,
                0.0312501229f,
                0.0312502459f,
                0.0312504061f,
                0.0312505625f,
                0.0312515162f,
            },
            .type = mon::SPT_WALL_ZP,
            .pos_spaces = {mon::AABB{{843, -170, -5}, {743, -1, 25}}},
        },
        .orange_search{
            .locked = true,
            .locked_pos{-127.96875f, -191.242996f, 182.03125f},
            .locked_ang{0, 0, 0},
        },
        .target_space = mon::AABB{{-267, 962, -11}, {-360, 720, 190}},
        .entry_pos_search = mon::SEPF_LOWER,
        .valid_placement_orders{
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
            mon::PlacementOrder::AFTER_LOAD,
        },
        .tp_from_blue = false,
        .tp_player = true,
    };
    small_prng rng{0};
    auto sr = ss.FindVag(rng, 1000000);
    if (sr)
        sr->print();
}

static void FindVagIn11()
{
    mon::SearchSpace ss{
        .blue_search{
            .lock_opts{128.03125f},
            .type = mon::SPT_WALL_ZP,
            .pos_spaces = {mon::AABB{{-860, 280, 175}, {-551, -43, 127}}},
        },
        .orange_search{
            .lock_opts{-1183.96875f},
            .type = mon::SPT_WALL_YP,
            .pos_spaces = {mon::AABB{{-80, -1167, 285}, {-536, -1193, 373}}},
        },
        .target_space = mon::AABB{{-600, -1260, 1647}, {-369, -1413, 1762}},
        .entry_pos_search = mon::SEPF_LOWER,
        .valid_placement_orders{
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
            mon::PlacementOrder::BLUE_OPEN_ORANGE_NEW_LOCATION,
            mon::PlacementOrder::AFTER_LOAD,
        },
        .tp_from_blue = false,
        .tp_player = true,
    };
    small_prng rng{0};
    auto sr = ss.FindVag(rng, 1000000);
    if (sr)
        sr->print();
}

static void FindKnownVagIn11()
{
    mon::SearchSpace ss{
        .blue_search{
            .lock_opts{383.96875f},
            .type = mon::SPT_WALL_ZN,
            .pos_spaces = {mon::AABB{{-860, 280, 450}, {-551, -43, 380}}},
        },
        .orange_search{
            .lock_opts{
                -64.03125f,
                -64.0312653f,
            },
            .type = mon::SPT_WALL_XN,
            .pos_spaces = {mon::AABB{{-80, -816, 284}, {-40, -1154, 509}}},
        },
        .target_space = mon::AABB{{-106, -1427, 1597}, {-273, -1282, 1729}},
        .entry_pos_search = mon::SEPF_LOWER,
        .valid_placement_orders{
            mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
            mon::PlacementOrder::BLUE_OPEN_ORANGE_NEW_LOCATION,
            mon::PlacementOrder::AFTER_LOAD,
        },
        .tp_from_blue = false,
        .tp_player = true,
    };
    small_prng rng{0};
    auto sr = ss.FindVag(rng, 1000000);
    if (sr)
        sr->print();
}

int main()
{
    mon::MonocleFloatingPointScope scope{};

    FindKnownVagIn11();
}
