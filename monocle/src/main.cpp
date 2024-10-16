#include <iostream>
#include <algorithm>
#include <ranges>
#include <numeric>

#include <fstream>

#include "source_math.hpp"
#include "vag_logic.hpp"
#include "prng.hpp"
#include "tga.hpp"

#include <vector>
// #include <matplot/matplot.h>

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

static Vector RandomPos(small_prng& rng, size_t oct_bits, size_t dist_scale)
{
    float a = 1 << dist_scale;
    float b = 1 << (dist_scale + 1);
    return Vector{
        rng.next_float(a, b) * (oct_bits & 1 ? 1.f : -1.f),
        rng.next_float(a, b) * (oct_bits & 2 ? 1.f : -1.f),
        rng.next_float(a, b) * (oct_bits & 4 ? 1.f : -1.f),
    };
}

static QAngle RandomAng(small_prng& rng, PITCH_YAW_TYPE type, bool has_roll)
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
            assert(0);
    }
    return QAngle(p, y, has_roll ? rng.next_float(-180.f, 180.f) : 0.f);
}

static void PlotUlpDistribution()
{
    small_prng rng{2};
    TpChain chain;
    std::vector<int> ulp_diffs;
    for (int i = 0; i < 100000; i++) {
        PortalPair pp{
            RandomPos(rng, rng.next_int(0, 8), rng.next_int(4, 10)),
            RandomAng(rng, PYT_ANY, true),
            RandomPos(rng, rng.next_int(0, 8), rng.next_int(4, 10)),
            RandomAng(rng, PYT_ANY, true),
        };
        pp.CalcTpMatrices(PlacementOrder::BLUE_OPEN_ORANGE_NEW_LOCATION);
        Entity ent{pp.blue.pos};
        GenerateTeleportChain(pp, ent, N_CHILDREN_PLAYER_WITH_PORTAL_GUN, true, chain, 3);
        if (chain.max_tps_exceeded)
            continue;
        assert(chain.ulp_diffs[1].ax != ULP_DIFF_TOO_LARGE_AX);
        ulp_diffs.push_back(chain.ulp_diffs[1].diff);
    }

    int smallest = *std::ranges::min_element(ulp_diffs);
    int biggest = *std::ranges::max_element(ulp_diffs);

    /*using namespace matplot;
    auto h = hist(ulp_diffs);
    show();*/
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
    const int n_runs_per_combination = 10000;
    TpChain chain;

    std::ofstream of{"results.txt"};
    of << "{\n";
    of << "\"n_runs_per_iteration\": " << n_runs_per_combination << ",\n";
    of << "\"result_keys\": [\"normal_teleport\", \"VAG\", \"unknown\"],\n";
    of << "\"runs\": [";

    std::array pos_scales{6, 7, 8, 9};
    std::array<int, 8> octs;
    std::array<int, PYT_COUNT> py_types;
    std::array roll_opts{false, true};
    // not per-portal
    std::array<int, (int)PlacementOrder::COUNT> mat_opts;
    std::array<int, 4> init_pos_bits;
    std::array is_player_opts{false, true};

    std::iota(octs.begin(), octs.end(), 0);
    std::iota(py_types.begin(), py_types.end(), 0);
    std::iota(mat_opts.begin(), mat_opts.end(), 0);
    std::iota(init_pos_bits.begin(), init_pos_bits.end(), 0);

    auto portal_cp = std::ranges::views::cartesian_product(pos_scales, octs, py_types, roll_opts);
    auto misc_cp = std::ranges::views::cartesian_product(mat_opts, init_pos_bits, is_player_opts);
    auto all_opts = std::ranges::views::cartesian_product(portal_cp, portal_cp, misc_cp);

    size_t n_complete = 0;
    size_t n_combinations = (size_t)all_opts.size();

    for (const auto& [blue_opts, orange_opts, misc_opts] : all_opts) {

        auto bp = std::get<0>(blue_opts);

        bool pqr = std::get<1>(misc_opts) & 1;
        bool pqu = std::get<1>(misc_opts) & 2;
        int results[RESULT_COUNT]{};

        for (int i = 0; i < n_runs_per_combination; i++) {
            PortalPair pp{
                RandomPos(rng, std::get<1>(blue_opts), std::get<0>(blue_opts)),
                RandomAng(rng, (PITCH_YAW_TYPE)std::get<2>(blue_opts), std::get<3>(blue_opts)),
                RandomPos(rng, std::get<1>(orange_opts), std::get<0>(orange_opts)),
                RandomAng(rng, (PITCH_YAW_TYPE)std::get<2>(orange_opts), std::get<3>(orange_opts)),
            };
            pp.CalcTpMatrices((PlacementOrder)std::get<0>(misc_opts));
            Vector ent_pos = pp.blue.pos + pp.blue.r * rng.next_float(0, PORTAL_HALF_WIDTH / 2.f) * (pqr ? 1.f : -1.f) +
                             pp.blue.u * rng.next_float(0, PORTAL_HALF_HEIGHT / 2.f) * (pqu ? 1.f : -1.f);
            Entity ent = std::get<2>(misc_opts) ? Entity{ent_pos} : Entity{ent_pos, 1.f};
            GenerateTeleportChain(pp, ent, N_CHILDREN_PLAYER_WITH_PORTAL_GUN, true, chain, 3);
            if (chain.max_tps_exceeded)
                results[RESULT_UNKNOWN]++;
            else if (chain.cum_primary_tps == 1)
                results[RESULT_TP]++;
            else if (chain.cum_primary_tps == -1)
                results[RESULT_VAG]++;
            else
                assert(0);
        }
        of << (n_complete ? ",\n" : "\n") << "{\n\t\"label\":\n\t{\n";
        for (int i = 0; i < 2; i++) {
            auto& opts = i ? blue_opts : orange_opts;
            const char* prefix = i ? "\t\t\"blue." : "\t\t\"orange.";
            of << prefix << "pos_scale\": " << std::get<0>(opts) << ",\n";
            of << prefix << "octant\": " << std::get<1>(opts) << ",\n";
            of << prefix << "pitch_yaw_type\": \"" << PITCH_YAW_STRS[std::get<2>(opts)] << "\",\n";
            of << prefix << "has_roll\": " << (std::get<3>(opts) ? "true" : "false") << ",\n";
        }
        of << "\t\t\"matrix\": \"" << PlacementOrderStrs[(int)std::get<0>(misc_opts)] << "\",\n";
        of << "\t\t\"is_player\": " << (std::get<2>(misc_opts) ? "true" : "false") << ",\n";
        of << "\t\t\"init_portal_quadrant\": " << std::get<1>(misc_opts);
        of << "\n\t},\n\t\"results\": [";
        for (int i = 0; i < RESULT_COUNT; i++)
            of << ((double)results[i] / n_runs_per_combination) << (i == RESULT_COUNT - 1 ? "]\n}" : ", ");

        printf("finished %u/%u runs\n", ++n_complete, n_combinations);
    }

    of << "\n]\n}\n";
}

static void CreateOverlayPortalImage(const PortalPair& pair, const char* file_name, size_t y_res, bool from_blue)
{
    size_t x_res = (size_t)((double)y_res * PORTAL_HALF_WIDTH / PORTAL_HALF_HEIGHT);
    struct pixel {
        uint8_t b, g, r, a;
    };
    std::vector<pixel> pixels{y_res * x_res};
    TpChain chain;
    Vector ul{};
    for (size_t y = 0; y < y_res; y++) {
        for (size_t x = 0; x < x_res; x++) {
            float ox = PORTAL_HALF_WIDTH * (-1 + 1.f / x_res);
            float tx = (float)x / (x_res - 1);
            float mx = ox * (1 - 2 * tx);

            float oy = PORTAL_HALF_HEIGHT * (-1 + 1.f / y_res);
            float ty = (float)y / (y_res - 1);
            float my = oy * (1 - 2 * ty);

            const Portal& p = from_blue ? pair.blue : pair.orange;
            Entity ent{p.pos + p.r * mx + p.u * my};

            GenerateTeleportChain(pair, ent, N_CHILDREN_PLAYER_WITHOUT_PORTAL_GUN, from_blue, chain, 3);
            pixel& pix = pixels[x_res * y + x];
            pix.a = 255;
            if (chain.max_tps_exceeded)
                pix.r = pix.g = pix.b = 0;
            else if (chain.cum_primary_tps == 1)
                pix.r = pix.g = pix.b = 125;
            else if (chain.cum_primary_tps == -1)
                pix.g = 255;
            else
                assert(0);
        }
    }
    tga_write(file_name, x_res, y_res, (uint8_t*)pixels.data(), 4, 3);
}

static void FindVagIn04()
{
    small_prng rng{0};
    TpChain chain;
    float xmin = -75.70868f;
    float xmax = 145.85846f;
    float ymin = 311.f;
    float ymax = 350.70612f;
    float yaw_min = -6;
    float yaw_max = 28;

    Vector target_mins{-30.f, 706.f, 566.5f};
    Vector target_maxs{223.f, 958.f, 700.f};

    for (int i = 0; i < 100000; i++) {
        PortalPair pp{
            {rng.next_float(xmin, xmax), rng.next_float(ymin, ymax), 255.96877},
            {90, rng.next_float(yaw_min, yaw_max), 0},
            {-448, 0.03125, 54.9502},
            {0, 90, 0},
        };
        pp.CalcTpMatrices(PlacementOrder::BLUE_WAS_CLOSED_ORANGE_OPENED);
        float r = rng.next_float(-PORTAL_HALF_WIDTH * 0.5f, PORTAL_HALF_WIDTH * 0.5f);
        float u = rng.next_float(-PORTAL_HALF_HEIGHT * 0.5f, PORTAL_HALF_HEIGHT * 0.5f);
        Entity ent{pp.blue.pos + pp.blue.r * r + pp.blue.u * u};
        Entity ent_copy = ent;
        GenerateTeleportChain(pp, ent, N_CHILDREN_PLAYER_WITH_PORTAL_GUN, true, chain, 3);
        if (chain.max_tps_exceeded)
            continue;
        if (chain.cum_primary_tps != -1)
            continue;
        Vector t = chain.pts[chain.pts.size() - 1];
        if (t.x < target_mins.x || t.y < target_mins.y || t.z < target_mins.z)
            continue;
        if (t.x > target_maxs.x || t.y > target_maxs.y || t.z > target_maxs.z)
            continue;
        pp.PrintNewlocationCmd();
        ent_copy.PrintSetposCmd();
        const char* file_name = "04_blue.tga";
        printf("writing image %s...\n", file_name);
        CreateOverlayPortalImage(pp, file_name, 101, true);
        printf("done.\n");
        break;
    }
}

int main()
{
    SyncFloatingPointControlWord();

    FindVagIn04();

    /*small_prng rng{2};
    TpChain chain;

    for (int i = 0; i < 100000; i++) {
        constexpr float p_min = 650.f, p_max = 1200.f, a_min = -180.f, a_max = 180.f;

        PortalPair pp{
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{0, rng.next_int(-2, 2) * 90.f, 0},
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{rng.next_float(-3, 3), rng.next_int(-2, 2) * 90.f, 0},
        };

        if (pp.blue.pos.DistToSqr(pp.orange.pos) < 300 * 300)
            continue;

        pp.CalcTpMatrices(PlacementOrder::BLUE_OPEN_ORANGE_NEW_LOCATION);

        Entity e1{pp.blue.pos}, e2{pp.blue.pos + pp.blue.u};
        GenerateTeleportChain(pp, e1, N_CHILDREN_PLAYER_WITH_PORTAL_GUN, true, chain, 3);
        if (chain.max_tps_exceeded)
            continue;
        int n1 = chain.cum_primary_tps;
        GenerateTeleportChain(pp, e2, N_CHILDREN_PLAYER_WITH_PORTAL_GUN, true, chain, 3);
        if (chain.max_tps_exceeded)
            continue;
        if (n1 == chain.cum_primary_tps)
            continue;
        printf("found iteration %d\n", i);

        using namespace matplot;

        size_t y_res = 100;
        size_t x_res = (size_t)(y_res * PORTAL_HALF_WIDTH / PORTAL_HALF_HEIGHT);
        std::vector<std::vector<uint8_t>> im;

        for (size_t y = 0; y < y_res; y++) {
            im.emplace_back(x_res);
            for (size_t x = 0; x < x_res; x++) {
                float xp = ((float)x / (x_res - 1) - 0.5f) * 2 * PORTAL_HALF_WIDTH;
                float yp = ((float)y / (y_res - 1) - 0.5f) * 2 * PORTAL_HALF_HEIGHT;
                Entity ent{pp.blue.pos + pp.blue.r * xp + pp.blue.u * yp};
                GenerateTeleportChain(pp, ent, N_CHILDREN_PLAYER_WITH_PORTAL_GUN, true, chain, 3);
                if (chain.max_tps_exceeded)
                    im[y][x] = 0;
                else
                    im[y][x] = chain.tp_dirs.size();
            }
        }
        imagesc(im);
        colorbar();

        show();

        break;
    }*/

    /*small_prng rng{7};
    int n_chains = 100000;
    TpChain chain;
    for (int i = 0; i < n_chains; i++) {
        constexpr float p_min = 650.f, p_max = 1200.f, a_min = -180.f, a_max = 180.f;

        PortalPair pp{
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{0, rng.next_float(a_min, a_max), 0},
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{0, rng.next_int(-1, 2) * 90.f, 0},
        };
        if (pp.blue.pos.DistToSqr(pp.orange.pos) < 300 * 300)
            continue;

        pp.CalcTpMatrices(PlacementOrder::BLUE_OPEN_ORANGE_NEW_LOCATION);
        Entity ent{pp.blue.pos};
        GenerateTeleportChain(pp, ent, N_CHILDREN_PLAYER_WITH_PORTAL_GUN, true, chain, 20);

        if (chain.max_tps_exceeded)
            continue;
        if (chain.cum_primary_tps >= -1 && chain.cum_primary_tps <= 1)
            continue;
        if (pp.blue.pos.DistToSqr(chain.pts[chain.pts.size() - 1]) < 300 * 300)
            continue;
        if (pp.orange.pos.DistToSqr(chain.pts[chain.pts.size() - 1]) < 300 * 300)
            continue;

        // clang-format off
        Vector sp = Entity{chain.pts[0]}.origin;
        printf(
            "iteration %d\n"
            "%d primary teleports:\n"
            "ent_fire blue   newlocation \"%.9g %.9g %.9g %.9g %.9g %.9g\"\n"
            "ent_fire orange newlocation \"%.9g %.9g %.9g %.9g %.9g %.9g\"\n"
            "setpos %.9g %.9g %.9g\n",
            i,
            chain.cum_primary_tps,
            pp.orange.pos.x, pp.orange.pos.y, pp.orange.pos.z, pp.orange.ang.x, pp.orange.ang.y, pp.orange.ang.z,
            pp.blue.pos.x, pp.blue.pos.y, pp.blue.pos.z, pp.blue.ang.x, pp.blue.ang.y, pp.blue.ang.z,
            sp.x, sp.y, sp.z
        );
        break;
        // clang-format on
    };*/

    /*printf("\n\n");

    std::vector<int> cum;
    cum.resize(chains.size());

    std::transform(chains.cbegin(), chains.cend(), cum.begin(), [](const TpChain& chain) {
        return chain.cum_primary_tps;
    });

    using namespace matplot;

    auto h = hist(cum);
    show();*/
}