#include "catch_amalgamated.hpp"
#include "source_math.hpp"
#include "vag_logic.hpp"
#include "prng.hpp"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <chrono>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define CATCH_SEED ((uint32_t)42069)

int main(int argc, char* argv[])
{
    SyncFloatingPointControlWord();
    Catch::Session session;
    session.libIdentify();
    session.configData().rngSeed = CATCH_SEED;

    using namespace std::chrono;
    auto t = high_resolution_clock::now();
    int ret = session.run(argc, argv);
    auto td = duration_cast<duration<double>>(high_resolution_clock::now() - t);
    printf("Tests completed after %.3fs\n", td.count());
}

/*
* I'm not too familiar with catch, but I tried using the built-in generators for a bit and they
* seem too complicated & verbose for my needs, so I'm just using good ol' rng in each test.
*/
#define REPEAT_TEST(n)                    \
    int _test_it = GENERATE(range(0, n)); \
    INFO("test iteration " << _test_it)

static Portal RandomPortal(small_prng& rng)
{
    constexpr float p_min = -3000.f, p_max = 3000.f, a_min = -180.f, a_max = 180.f;
    return Portal{
        Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
        QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
    };
}

TEST_CASE("ShouldTeleport (no portal hole check)")
{
    REPEAT_TEST(1000);
    static small_prng rng;
    Portal p = RandomPortal(rng);
    bool is_player = rng.next_bool();
    INFO("entity is " << (is_player ? "player" : "non-player"));

    if (is_player) {
        Entity ent1 = Entity::CreatePlayerFromCenter(p.pos - p.f, true);
        REQUIRE(p.ShouldTeleport(ent1, false));
        Entity ent2 = Entity::CreatePlayerFromCenter(p.pos + p.f, true);
        REQUIRE_FALSE(p.ShouldTeleport(ent2, false));
    } else {
        Entity ent1 = Entity::CreateBall(p.pos - p.f, 0.f);
        REQUIRE(p.ShouldTeleport(ent1, false));
        Entity ent2 = Entity::CreateBall(p.pos + p.f, 0.f);
        REQUIRE_FALSE(p.ShouldTeleport(ent2, false));
    }
}

TEST_CASE("ShouldTeleport (with portal hole check")
{
    /*
    * Looking at a portal hole from the side, here's the points we're testing:
    * 
    *      .   .   .   .                                    .   .   .   .
    *
    *      .   @   @   @                                    @   @   .   .
    *            --------------------------------------------------
    *      .   @ | *   *                                    *   * | .   .
    *            |                                                |      
    *      .   @ | *   *                                    *   * | .   .
    *            |                                                |
    *            |                                                | --->  out of the portal
    *            |                                                |
    *      .   @ | *   *                                    *   * | .   .
    *            |                                                | .   .
    *      .   @ | *   *                                    *   * | .   .
    *            --------------------------------------------------
    *      .   @   @   @                                    @   @   .   .
    *
    *      .   .   .   .                                    .   .   .   .
    *
    * - the corner to test at is determined by face_bits
    * - the point within each grid is determined by grid_bits
    * - ent_type is 0: ball w/ radius 1; 1: ball w/ radius 20; 2: crouched player
    * 
    * The spacing of the grid is 20 units between each point. In the diagram, this means:
    * . = points that should not be teleported
    * * = points that should be teleported
    * @ = points that should be teleported if it's a big ball or player
    */

    REPEAT_TEST(10000);
    static small_prng rng;
    int ent_type = rng.next_int(0, 3);
    int corner_bits = rng.next_int(0, 8);
    int grid_bits = rng.next_int(0, 0b111111);
    bool axial = rng.next_bool();

    Vector pos{rng.next_float(-3000.f, 3000.f), rng.next_float(-3000.f, 3000.f), rng.next_float(-3000.f, 3000.f)};
    QAngle ang;
    if (axial) {
        // BoxOnPlaneSide has special axial cases, test those too
        float roll = rng.next_bool() ? rng.next_float(-180.f, 180.f) : 0.f;
        switch (rng.next_int(0, 6)) {
            case 0:
                ang = QAngle{0, 0, roll};
                break;
            case 1:
                ang = QAngle{0, 90, roll};
                break;
            case 2:
                ang = QAngle{0, 180, roll};
                break;
            case 3:
                ang = QAngle{0, -90, roll};
                break;
            case 4:
                ang = QAngle{90, 0, roll};
                break;
            case 5:
                ang = QAngle{-90, 0, roll};
                break;
            default:
                FAIL();
        }
    } else {
        ang = QAngle{rng.next_float(-180.f, 180.f), rng.next_float(-180.f, 180.f), rng.next_float(-180.f, 180.f)};
    }

    Portal p{pos, ang};

    INFO("corner: " << ((corner_bits & 1) ? "-f " : "+f ") << ((corner_bits & 2) ? "-r " : "+r ")
                    << ((corner_bits & 4) ? "-u" : "+u"));

    Vector corner_off = p.f * ((corner_bits & 1) ? -PORTAL_HOLE_DEPTH : 0.f) +
                        p.r * ((corner_bits & 2) ? -PORTAL_HALF_WIDTH : PORTAL_HALF_WIDTH) +
                        p.u * ((corner_bits & 4) ? -PORTAL_HALF_HEIGHT : PORTAL_HALF_HEIGHT);

    constexpr float SPACING = 20.f;

    INFO("grid offset: (f*" << (-1.5f + ((grid_bits >> 0) & 0b11)) * ((corner_bits & 1) ? -1.f : 1.f) << " + r*"
                            << (-1.5f + ((grid_bits >> 2) & 0b11)) * ((corner_bits & 2) ? -1.f : 1.f) << " + u*"
                            << (-1.5f + ((grid_bits >> 4) & 0b11)) * ((corner_bits & 4) ? -1.f : 1.f) << ") * "
                            << SPACING);

    // the grid offset is set so that bigger grid bits point are further away from the portal center, this makes the should_teleport condition simplier
    Vector grid_off = p.f * (-1.5f + ((grid_bits >> 0) & 0b11)) * ((corner_bits & 1) ? -1.f : 1.f) +
                      p.r * (-1.5f + ((grid_bits >> 2) & 0b11)) * ((corner_bits & 2) ? -1.f : 1.f) +
                      p.u * (-1.5f + ((grid_bits >> 4) & 0b11)) * ((corner_bits & 4) ? -1.f : 1.f);

    Vector ent_pos = p.pos + corner_off + grid_off * SPACING;
    Entity ent;
    switch (ent_type) {
        case 0: {
            UNSCOPED_INFO("entity is small ball");
            ent = Entity::CreateBall(ent_pos, 1.f);
            break;
        }
        case 1: {
            UNSCOPED_INFO("entity is ball w/ radius " << SPACING);
            ent = Entity::CreateBall(ent_pos, SPACING);
            break;
        }
        case 2: {
            UNSCOPED_INFO("entity is crouched player");
            ent = Entity::CreatePlayerFromCenter(ent_pos, true);
            break;
        }
        default:
            FAIL();
    }
    bool should_teleport;
    if (ent_type == 0) {
        // only inner-most octant in the grid
        should_teleport = !(grid_bits & 0b101010);
    } else {
        // carve out the set of non-allowed points
        should_teleport = true;
        should_teleport &= (grid_bits & 0b110000) != 0b110000;
        should_teleport &= (grid_bits & 0b001100) != 0b001100;
        if (corner_bits & 1)
            should_teleport &= (grid_bits & 0b000011) != 0b000011;
        else
            should_teleport &= (grid_bits & 0b000010) == 0;
    }

    REQUIRE(should_teleport == p.ShouldTeleport(ent, true));
}

TEST_CASE("Teleport")
{
    REPEAT_TEST(10000);
    static small_prng rng;
    Portal p1 = RandomPortal(rng);
    Portal p2 = RandomPortal(rng);
    int translate_mask = rng.next_int(0, 16);
    int order = rng.next_int(0, (int)PlacementOrder::COUNT);
    bool p1_teleporting = rng.next_bool();
    bool is_player = rng.next_bool();
    INFO("entity is " << (is_player ? "player" : "non-player"));

    PortalPair pp{p1, p2};
    pp.CalcTpMatrices((PlacementOrder)order);

    Vector off1{0.f, 0.f, 0.f}, off2{0.f, 0.f, 0.f};
    if (translate_mask & 1) {
        off1 += p1.f;
        off2 -= p2.f;
    }
    if (translate_mask & 2) {
        off1 += p1.r;
        off2 -= p2.r;
    }
    if (translate_mask & 4) {
        off1 += p1.u;
        off2 += p2.u;
    }
    bool sign = translate_mask & 8;
    Vector pt1 = p1.pos + (sign ? off1 : -off1);
    Vector pt2 = p2.pos + (sign ? off2 : -off2);

    Vector center = p1_teleporting ? pt1 : pt2;
    Entity ent = is_player ? Entity::CreatePlayerFromCenter(center, true) : Entity::CreateBall(center, 0.f);
    ent = pp.Teleport(ent, p1_teleporting);
    REQUIRE_THAT(ent.GetCenter().DistToSqr(p1_teleporting ? pt2 : pt1), Catch::Matchers::WithinAbs(0.0, 0.01));
}

TEST_CASE("Nudging point towards portal plane (close)")
{
    REPEAT_TEST(10000);
    static small_prng rng;
    Portal p = RandomPortal(rng);
    for (int i = 0; i < 3; i++)
        if (fabsf(p.pos[i]) < 0.5f)
            return;
    int translate_mask = rng.next_int(0, 8);
    bool is_player = rng.next_bool();
    INFO("entity is " << (is_player ? "player" : "non-player"));

    Vector off{0.f, 0.f, 0.f};
    if (translate_mask & 1)
        off += p.r;
    if (translate_mask & 2)
        off += p.u;
    Vector pt = p.pos + ((translate_mask & 4) ? off : -off);
    // add a little "random" nudge
    pt[0] += pt[0] / 100000.f;

    Entity ent = is_player ? Entity::CreatePlayerFromCenter(pt, true) : Entity::CreateBall(pt, 0.f);
    auto [nudged_ent, plane_dist] = ProjectEntityToPortalPlane(ent, p);
    REQUIRE(!!plane_dist.is_valid);
    REQUIRE(p.ShouldTeleport(ent, false) == !!plane_dist.pt_was_behind_portal);
    // nudged entity is guaranteed to be behind the portal
    REQUIRE(p.ShouldTeleport(nudged_ent, false));
    // nudge the entity across the portal boundary
    Vector& ent_pos = nudged_ent.GetPosRef();
    float target = ent_pos[plane_dist.ax] + p.plane.n[plane_dist.ax];
    ent_pos[plane_dist.ax] = std::nextafterf(ent_pos[plane_dist.ax], target);
    REQUIRE_FALSE(p.ShouldTeleport(nudged_ent, false));
}

// basically this test shouldn't take forever (the old version of this would take ages)
TEST_CASE("Nudging point towards portal plane (far)")
{
    static small_prng rng;
    Portal p = RandomPortal(rng);
    Entity ent = Entity::CreatePlayerFromCenter(p.pos + p.r * 2.5f - p.u * 1.5f + p.f * 12345.f, false);
    auto [nudged_ent, plane_dist] = ProjectEntityToPortalPlane(ent, p);
    REQUIRE(!!plane_dist.is_valid);
    REQUIRE_FALSE(p.ShouldTeleport(ent, false));
    // nudged entity is guaranteed to be behind the portal
    REQUIRE(p.ShouldTeleport(nudged_ent, false));
    // nudge the entity across the portal boundary
    Vector& ent_pos = nudged_ent.GetPosRef();
    float target = ent_pos[plane_dist.ax] + p.plane.n[plane_dist.ax];
    ent_pos[plane_dist.ax] = std::nextafterf(ent_pos[plane_dist.ax], target);
    REQUIRE_FALSE(p.ShouldTeleport(nudged_ent, false));
}

TEST_CASE("Teleport chain results in VAG")
{
    /*
    * chamber 09 - blue portal on opposite wall of orange, bottom left corner
    * setpos -127.96875385 -191.24299622 164.03125
    * teleports by: orange, blue, blue
    */
    PortalPair pp{
        Vector{255.96875f, -161.01294f, 54.031242f},
        QAngle{-0.f, 180.f, 0.f},
        Vector{-127.96875f, -191.24300f, 182.03125f},
        QAngle{0.f, 0.f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED);

    TeleportChainParams params{
        &pp,
        Entity::CreatePlayerFromCenter(Vector{-127.96876f, -191.24300f, 182.03125f}, true),
    };
    params.record_flags = TCRF_RECORD_ALL;
    TeleportChainResult result;

    Vector target_vag_pos = pp.Teleport(params.ent.GetCenter(), true);

    const int n_teleports_success = 3;

    for (int n_max_teleports = 0; n_max_teleports < n_teleports_success + 2; n_max_teleports++) {
        DYNAMIC_SECTION("teleport limit is " << n_max_teleports)
        {
            params.n_max_teleports = n_max_teleports;
            GenerateTeleportChain(params, result);

            int n_expected_teleports = n_max_teleports > n_teleports_success ? n_teleports_success : n_max_teleports;

            REQUIRE(result.total_n_teleports == n_expected_teleports);
            REQUIRE(result.max_tps_exceeded == (n_expected_teleports < n_teleports_success));
            REQUIRE(result.tp_dirs.size() == n_expected_teleports);
            REQUIRE(result.ents.size() == (n_expected_teleports + 1));
            REQUIRE(result.portal_plane_diffs.size() == (n_expected_teleports + 1));
            REQUIRE(result.cum_teleports == std::vector<int>{0, 1, 0, -1}[n_expected_teleports]);
            REQUIRE(result.ents[0] == params.ent);
            REQUIRE(result.ents.back() == result.ent);

            switch (n_expected_teleports) {
                case 3:
                    REQUIRE(result.tp_dirs[2] == false);
                    REQUIRE_THAT(result.ents[3].GetCenter().DistToSqr(target_vag_pos),
                                 Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE_FALSE(!!result.portal_plane_diffs[3].is_valid);
                    [[fallthrough]];
                case 2:
                    REQUIRE(result.tp_dirs[1] == false);
                    REQUIRE_THAT(result.ents[2].GetCenter().DistToSqr(pp.orange.pos),
                                 Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(!!result.portal_plane_diffs[2].is_valid);
                    REQUIRE(result.portal_plane_diffs[2].ax == 0);
                    REQUIRE_FALSE(!!result.portal_plane_diffs[2].pt_was_behind_portal);
                    [[fallthrough]];
                case 1:
                    REQUIRE(result.tp_dirs[0] == true);
                    REQUIRE_THAT(result.ents[1].GetCenter().DistToSqr(pp.blue.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(!!result.portal_plane_diffs[1].is_valid);
                    REQUIRE(result.portal_plane_diffs[1].ax == 0);
                    REQUIRE(!!result.portal_plane_diffs[1].pt_was_behind_portal);
                    [[fallthrough]];
                case 0:
                    REQUIRE_THAT(result.ents[0].GetCenter().DistToSqr(pp.orange.pos),
                                 Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(!!result.portal_plane_diffs[0].is_valid);
                    REQUIRE(result.portal_plane_diffs[0].ax == 0);
                    REQUIRE(!!result.portal_plane_diffs[0].pt_was_behind_portal);
                    break;
                default:
                    FAIL();
            }
        }
    }
}

TEST_CASE("Teleport chain results in 5 teleports (cum=1)")
{
    /*
    * chamber 09 - blue portal on opposite wall of orange, bottom right corner
    * setpos -127.96875385 -191.24299622 164.03125
    * teleports by: orange, blue, blue, orange, orange
    */
    PortalPair pp{
        Vector{255.96875f, -223.96875f, 54.031242f},
        QAngle{-0.f, 180.f, 0.f},
        Vector{-127.96875f, -191.24300f, 182.03125f},
        QAngle{0.f, 0.f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED);

    TeleportChainParams params{
        &pp,
        Entity::CreatePlayerFromCenter(Vector{-127.96876f, -191.24300f, 182.03125f}, true),
    };
    params.record_flags = TCRF_RECORD_ALL;
    TeleportChainResult result;

    Vector target_vag_pos = pp.Teleport(params.ent.GetCenter(), true);

    const int n_teleports_success = 5;

    for (int n_max_teleports = 0; n_max_teleports < n_teleports_success + 2; n_max_teleports++) {
        DYNAMIC_SECTION("teleport limit is " << n_max_teleports)
        {
            params.n_max_teleports = n_max_teleports;
            GenerateTeleportChain(params, result);
            int n_expected_teleports = n_max_teleports > n_teleports_success ? n_teleports_success : n_max_teleports;

            REQUIRE(result.total_n_teleports == n_expected_teleports);
            REQUIRE(result.max_tps_exceeded == (n_expected_teleports < n_teleports_success));
            REQUIRE(result.tp_dirs.size() == n_expected_teleports);
            REQUIRE(result.ents.size() == (n_expected_teleports + 1));
            REQUIRE(result.portal_plane_diffs.size() == (n_expected_teleports + 1));
            REQUIRE(result.cum_teleports == std::vector<int>{0, 1, 0, -1, 0, 1}[n_expected_teleports]);
            REQUIRE(result.ents[0] == params.ent);
            REQUIRE(result.ents.back() == result.ent);

            switch (n_expected_teleports) {
                case 5:
                    REQUIRE(result.tp_dirs[4] == true);
                    REQUIRE_THAT(result.ents[5].GetCenter().DistToSqr(pp.blue.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(!!result.portal_plane_diffs[5].is_valid);
                    REQUIRE(result.portal_plane_diffs[5].ax == 0);
                    REQUIRE_FALSE(result.portal_plane_diffs[5].pt_was_behind_portal);
                    [[fallthrough]];
                case 4:
                    REQUIRE(result.tp_dirs[3] == true);
                    REQUIRE_THAT(result.ents[4].GetCenter().DistToSqr(pp.orange.pos),
                                 Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(!!result.portal_plane_diffs[4].is_valid);
                    REQUIRE(result.portal_plane_diffs[4].ax == 0);
                    REQUIRE(!!result.portal_plane_diffs[4].pt_was_behind_portal);
                    [[fallthrough]];
                case 3:
                    REQUIRE(result.tp_dirs[2] == false);
                    REQUIRE_THAT(result.ents[3].GetCenter().DistToSqr(target_vag_pos),
                                 Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE_FALSE(!!result.portal_plane_diffs[3].is_valid);
                    [[fallthrough]];
                case 2:
                    REQUIRE(result.tp_dirs[1] == false);
                    REQUIRE_THAT(result.ents[2].GetCenter().DistToSqr(pp.orange.pos),
                                 Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(!!result.portal_plane_diffs[2].is_valid);
                    REQUIRE(result.portal_plane_diffs[2].ax == 0);
                    REQUIRE(!!result.portal_plane_diffs[2].pt_was_behind_portal);
                    [[fallthrough]];
                case 1:
                    REQUIRE(result.tp_dirs[0] == true);
                    REQUIRE_THAT(result.ents[1].GetCenter().DistToSqr(pp.blue.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(!!result.portal_plane_diffs[1].is_valid);
                    REQUIRE(result.portal_plane_diffs[1].ax == 0);
                    REQUIRE(!!result.portal_plane_diffs[1].pt_was_behind_portal);
                    [[fallthrough]];
                case 0:
                    REQUIRE_THAT(result.ents[0].GetCenter().DistToSqr(pp.orange.pos),
                                 Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(!!result.portal_plane_diffs[0].is_valid);
                    REQUIRE(result.portal_plane_diffs[0].ax == 0);
                    REQUIRE(!!result.portal_plane_diffs[0].pt_was_behind_portal);
                    break;
                default:
                    FAIL();
            }
        }
    }
}

// an actual "double VAG" found by xeonic
TEST_CASE("Teleport chain results in 6 teleports (cum=-2)")
{
    PortalPair pp{
        Vector{1001.2641f, 40.5883064f, 64.03125f},
        QAngle{-90.f, -92.752083f, 0.f},
        Vector{511.96875f, 25.968760f, 54.031242f},
        QAngle{-0.f, 180.f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION);

    TeleportChainParams params{
        &pp,
        Entity::CreatePlayerFromOrigin(Vector{1000.405640f, 51.970764f, 46.03125f}, true),
    };
    params.project_to_first_portal_plane = false;
    params.record_flags = TCRF_RECORD_ALL;
    TeleportChainResult result;

    /*
    * This is the position on the tick after the teleport according to the trace. It's a few units
    * off and I'm not really sure why. Gravity and floor portal exit velocity don't seem to account
    * for the full difference.
    */
    Entity target_ent = Entity::CreatePlayerFromOrigin(Vector{1050.953125f, 539.331055f, 563.865845f}, true);

    const int n_teleports_success = 6;

    for (int n_max_teleports = 0; n_max_teleports < n_teleports_success + 2; n_max_teleports++) {
        DYNAMIC_SECTION("teleport limit is " << n_max_teleports)
        {
            params.n_max_teleports = n_max_teleports;
            GenerateTeleportChain(params, result);
            int n_expected_teleports = n_max_teleports > n_teleports_success ? n_teleports_success : n_max_teleports;

            REQUIRE(result.total_n_teleports == n_expected_teleports);
            REQUIRE(result.max_tps_exceeded == (n_expected_teleports < n_teleports_success));
            REQUIRE(result.tp_dirs.size() == n_expected_teleports);
            REQUIRE(result.ents.size() == (n_expected_teleports + 1));
            REQUIRE(result.portal_plane_diffs.size() == (n_expected_teleports + 1));
            REQUIRE(result.cum_teleports == std::vector<int>{0, 1, 0, -1, 0, -1, -2}[n_expected_teleports]);
            REQUIRE(result.ents[0] == params.ent);
            REQUIRE(result.ents.back() == result.ent);

            switch (n_expected_teleports) {
                case 6:
                    REQUIRE(result.tp_dirs[5] == false);
                    REQUIRE_THAT(result.ents[6].GetCenter().DistTo(target_ent.GetCenter()),
                                 Catch::Matchers::WithinAbs(0, 5.f));
                    REQUIRE_FALSE(!!result.portal_plane_diffs[6].is_valid);
                    [[fallthrough]];
                case 5:
                    REQUIRE(result.tp_dirs[4] == false);
                    REQUIRE_THAT(result.ents[5].GetCenter().DistToSqr(result.ents[3].GetCenter()),
                                 Catch::Matchers::WithinAbs(0, .01f));
                    REQUIRE_FALSE(!!result.portal_plane_diffs[5].is_valid);
                    [[fallthrough]];
                case 4:
                    REQUIRE(result.tp_dirs[3] == true);
                    REQUIRE_THAT(result.ents[4].GetCenter().DistToSqr(params.ent.GetCenter()),
                                 Catch::Matchers::WithinAbs(0, .01f));
                    REQUIRE(!!result.portal_plane_diffs[4].is_valid);
                    REQUIRE(result.portal_plane_diffs[4].ax == 2);
                    [[fallthrough]];
                case 3:
                    REQUIRE(result.tp_dirs[2] == false);
                    REQUIRE_FALSE(!!result.portal_plane_diffs[3].is_valid);
                    [[fallthrough]];
                case 2:
                    REQUIRE(result.tp_dirs[1] == false);
                    REQUIRE_THAT(result.ents[2].GetCenter().DistToSqr(params.ent.GetCenter()),
                                 Catch::Matchers::WithinAbs(0, .01f));
                    REQUIRE(!!result.portal_plane_diffs[2].is_valid);
                    REQUIRE(result.portal_plane_diffs[2].ax == 2);
                    REQUIRE(!!result.portal_plane_diffs[2].pt_was_behind_portal);
                    [[fallthrough]];
                case 1:
                    REQUIRE(result.tp_dirs[0] == true);
                    REQUIRE_THAT(result.ents[1].GetCenter().DistTo(pp.orange.pos),
                                 Catch::Matchers::WithinAbs(0, 15.f));
                    REQUIRE(!!result.portal_plane_diffs[1].is_valid);
                    REQUIRE(result.portal_plane_diffs[1].ax == 0);
                    REQUIRE(!!result.portal_plane_diffs[1].pt_was_behind_portal);
                    [[fallthrough]];
                case 0:
                    REQUIRE(!!result.portal_plane_diffs[0].is_valid);
                    REQUIRE(result.portal_plane_diffs[0].ax == 2);
                    REQUIRE(!!result.portal_plane_diffs[0].pt_was_behind_portal);
                    break;
                default:
                    FAIL();
            }
        }
    }
}

TEST_CASE("Finite teleport chain results in free edicts")
{
    /*
    * chamber 09 - blue portal on opposite wall of orange, top left corner
    * setpos -127.96875385 -191.24299622 164.03125
    */
    PortalPair pp{
        Vector{255.96875f, -161.01295f, 201.96877f},
        QAngle{-0.f, 180.f, 0.f},
        Vector{-127.96875f, -191.24300f, 182.03125f},
        QAngle{0.f, 0.f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED);

    TeleportChainParams params{
        &pp,
        Entity::CreatePlayerFromCenter(Vector{-127.96876f, -191.24300f, 182.03125f}, true),
    };
    params.n_max_teleports = 200;
    params.record_flags = TCRF_RECORD_ALL;
    TeleportChainResult result;

    GenerateTeleportChain(params, result);

    REQUIRE(result.total_n_teleports == 82);
    REQUIRE(result.cum_teleports == 0);
    REQUIRE_FALSE(result.max_tps_exceeded);
    REQUIRE(result.tp_dirs.size() == 82);
    REQUIRE(result.portal_plane_diffs.size() == 83);
    REQUIRE(result.portal_plane_diffs.back().pt_was_behind_portal);
}

TEST_CASE("19 Lochness")
{
    PortalPair pp{
        Vector{-416.247498f, 735.368835f, 255.96875f},
        QAngle{90.f, -90.2385559f, 0.f},
        Vector{-394.776428f, -56.0312462f, 38.8377686f},
        QAngle{-44.9994202f, 180.f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION);

    TeleportChainParams params{
        &pp,
        Entity::CreatePlayerFromOrigin(Vector{-416.247498f, 735.368835f, 219.97f}, false),
    };
    params.project_to_first_portal_plane = false;

    TeleportChainResult result;
    GenerateTeleportChain(params, result);

    Entity roughExpectedEnt = Entity::CreatePlayerFromOrigin(Vector{398.634583f, 599.235168f, 400.928467f}, true);

    REQUIRE(result.total_n_teleports == 3);
    REQUIRE(result.cum_teleports == -1);
    REQUIRE_FALSE(result.max_tps_exceeded);
    REQUIRE(result.ent.is_player);
    REQUIRE(result.ent.player.crouched);
    REQUIRE_THAT(std::sqrtf(result.ent.GetCenter().DistToSqr(roughExpectedEnt.GetCenter())),
                 Catch::Matchers::WithinAbs(0, .01f));
}

TEST_CASE("E01 AAG")
{
    PortalPair pp{
        Vector{-141.904663f, 730.353394f, -191.417892f},
        QAngle{15.9453955f, 180.f, 0.f},
        Vector{-233.054321f, 652.257446f, -255.96875f},
        QAngle{-90.f, 155.909348f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION);

    TeleportChainParams params{
        &pp,
        Entity::CreatePlayerFromOrigin(Vector{-136.239883, 726.482483, -240.416977}, false),
    };
    params.project_to_first_portal_plane = false;

    TeleportChainResult result;
    GenerateTeleportChain(params, result);

    Entity roughExpectedEnt = Entity::CreatePlayerFromOrigin(Vector{-93.691399f, 636.419739f, -166.272324f}, true);

    REQUIRE(result.total_n_teleports == 3);
    REQUIRE(result.cum_teleports == -1);
    REQUIRE_FALSE(result.max_tps_exceeded);
    REQUIRE(result.ent.is_player);
    REQUIRE(result.ent.player.crouched);
    REQUIRE_THAT(std::sqrtf(result.ent.GetCenter().DistToSqr(roughExpectedEnt.GetCenter())),
                 Catch::Matchers::WithinAbs(0, 2.f));
}

TEST_CASE("00 AAG")
{
    /*
    * The bottom half of the diagonal surface in the 00 button room has a fucked up normal which
    * causes an AAG. This is very similar to why the PPNF (Portal Placement Never Fail) category
    * can abuse a lot of AAGs in random places - for some reason static prop normals are often
    * messed up in the same way.
    */
    PortalPair pp{
        Vector{-474.710541f, -1082.65906f, 182.03125f},
        QAngle{0.00538991531f, 135.f, 0.f},
        Vector{-735.139282f, -923.540344f, 128.03125f},
        QAngle{-90.f, 55.0390396f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION);

    TeleportChainParams params{
        &pp,
        Entity::CreatePlayerFromOrigin(Vector{-473.604370f, -1082.235498f, 128.031250f}, false),
    };
    params.project_to_first_portal_plane = false;

    TeleportChainResult result;
    GenerateTeleportChain(params, result);

    Entity roughExpectedEnt = Entity::CreatePlayerFromOrigin(Vector{-629.746887f, -1311.233398f, 138.827957f}, true);

    REQUIRE(result.total_n_teleports == 3);
    REQUIRE(result.cum_teleports == -1);
    REQUIRE_FALSE(result.max_tps_exceeded);
    REQUIRE(result.ent.is_player);
    REQUIRE(result.ent.player.crouched);
    REQUIRE_THAT(std::sqrtf(result.ent.GetCenter().DistToSqr(roughExpectedEnt.GetCenter())),
                 Catch::Matchers::WithinAbs(0, 2.f));
}

/*
* To use:
* - open the game and load a sufficiently recent version of SPT (anything after 03-2025 work)
* - run `spt_ipc 1`
* - create an orange and blue portal and set their name using `picker` & `ent_setname` to 'blue'/'orange'
* - noclip and crouch (with toggle duck)
* - run the tests
* 
* If this fails on the first iteration, try running again. Sometimes the relevant commands don't
* get sent to SPT quickly enough before host_timescale is set.
*/
TEST_CASE("SPT with IPC")
{
    const USHORT spt_port = 27182;

    // clang-format off
    struct _WinSock {
        int _ret;
        WSADATA wsadata;
        _WinSock() { _ret = WSAStartup(MAKEWORD(2, 2), &wsadata); if (_ret != 0) SKIP("WSAStartup failed");}
        ~_WinSock() { if (_ret == 0) WSACleanup(); }
    } winsock{};
    // clang-format on

    struct _TCPConnection {
        SOCKET sock = INVALID_SOCKET;
        bool _cleanup_sock = false;
        char buf[1024]{}; // send/recv
        int off = 0;
        int recvLen = 0;

        _TCPConnection()
        {
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) {
                SKIP("Socket creation failed");
                return;
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);
            server_addr.sin_port = htons(spt_port);

            _cleanup_sock = true;
            if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
                SKIP("Failed to connect to SPT");
        }

        void SendCmd(const char* fmt, ...)
        {
            recvLen = -1;
            va_list va;
            va_start(va, fmt);
            char cmdBuf[1024];
            int len = vsnprintf(cmdBuf, sizeof cmdBuf, fmt, va);
            if (len < 0 || len >= sizeof cmdBuf)
                FAIL("cmd format error");
            len = snprintf(buf, sizeof buf, "{\"type\":\"cmd\",\"cmd\":\"%s\"}", cmdBuf);
            if (len >= sizeof buf)
                FAIL("cmd format error");
            int ret = send(sock, buf, len + 1, 0);
            if (ret == SOCKET_ERROR)
                SKIP("send failed (" << ret << ")");
            va_end(va);
        }

        const char* BufPtr() const
        {
            return buf + off;
        }

        int BufLen() const
        {
            return recvLen - off;
        }

        void NextRecvMsg()
        {
            if (BufLen() > 0)
                off += strnlen(BufPtr(), BufLen()) + 1;
            if (BufLen() <= 0) {
                off = 0;
                recvLen = recv(sock, buf, sizeof buf, 0);
                if (recvLen < 0 || recvLen >= sizeof buf)
                    SKIP("recv failed (" << recvLen << ")");
            }
        }

        void RecvAck()
        {
            NextRecvMsg();
            REQUIRE_FALSE(strcmp(BufPtr(), "{\"type\":\"ack\"}"));
        }

        ~_TCPConnection()
        {
            if (_cleanup_sock)
                closesocket(sock);
        }
    } conn{};

    conn.SendCmd(
        "sv_cheats 1; spt_prevent_vag_crash 1; spt_focus_nosleep 1; spt_noclip_noslowfly 1; host_timescale 20");
    conn.RecvAck();
    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    small_prng rng;
    TeleportChainParams params;
    TeleportChainResult result;

    int iteration = 0;
    while (iteration < 1000) {
        Portal blue = RandomPortal(rng);
        Portal orange = RandomPortal(rng);
        /*
        * Don't use portals that are too close to each other - this can cause false positives with
        * the distance threshold to the teleport destination.
        */
        if (blue.pos.DistToSqr(orange.pos) < 500 * 500)
            continue;

        PortalPair pp{blue, orange};
        pp.CalcTpMatrices(PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION);

        params.pp = &pp;
        params.ent = Entity::CreatePlayerFromCenter(blue.pos, true);
        params.n_max_teleports = 3;

        GenerateTeleportChain(params, result);

        // only handle basic teleports and simple VAGs for now
        if (result.max_tps_exceeded || (result.cum_teleports != 1 && result.cum_teleports != -1))
            continue;

        /*
        * For some reason, some portals don't trigger the StartTouch/Touch call when we setpos on
        * the portal boundary. The solution is to setpos right in front of it first, then setpos
        * on the boundary. Using a single string with two setpos commands doesn't work (not sure
        * how that works under the hood), but sending two separate setpos commands works.
        * 
        * This is probably because SPT will process the two separate message on different frames,
        * and that will allow time for the first setpos to go through. The setpos on the boundary
        * probably fails to work on maps where the map origin is inbounds due to a portal
        * ownership bug.
        * 
        * Update: the reason some portals don't trigger StartTouch is probably because of the
        * origin of the map being inbounds in some cases.
        */

        // clang-format off
        Entity tmp_player = Entity::CreatePlayerFromCenter(blue.pos + blue.f, true);
        auto& bp = pp.blue.pos; auto& op = pp.orange.pos;
        auto& ba = pp.blue.ang; auto& oa = pp.orange.ang;
        conn.SendCmd(
            "ent_fire orange newlocation \\\"%.9g %.9g %.9g %.9g %.9g %.9g\\\"; "
            "ent_fire blue   newlocation \\\"%.9g %.9g %.9g %.9g %.9g %.9g\\\"; "
            "%s",
            op.x, op.y, op.z, oa.x, oa.y, oa.z,
            bp.x, bp.y, bp.z, ba.x, ba.y, ba.z,
            tmp_player.GetSetPosCmd().c_str()
        );
        // clang-format on
        conn.RecvAck();
        conn.SendCmd("%s", result.ents[0].GetSetPosCmd().c_str());
        conn.RecvAck();

        // timescale 1: sleep for 350ms, timescale 20: sleep for 10ms
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // TODO handle if player is standing
        conn.SendCmd("spt_ipc_properties 1 m_vecOrigin");
        conn.RecvAck();
        conn.NextRecvMsg();
        Vector actual_player_pos{};
        int n_args = _snscanf_s(conn.BufPtr(),
                                conn.BufLen(),
                                "{\"entity\":{"
                                "\"m_vecOrigin[0]\":%f,\"m_vecOrigin[1]\":%f,\"m_vecOrigin[2]\":%f"
                                "},\"exists\":true,\"type\":\"ent\"}",
                                &actual_player_pos.x,
                                &actual_player_pos.y,
                                &actual_player_pos.z);
        REQUIRE(n_args == 3);

        INFO("iteration " << iteration);
        INFO("expected " << (result.cum_teleports == -1 ? "VAG" : "normal teleport"));
        REQUIRE(actual_player_pos.DistToSqr(result.ent.player.origin) < 100 * 100);
        printf("iteration %d: %s", iteration, result.cum_teleports == -1 ? "VAG\n" : "Normal teleport\n");
        iteration++;
    }
}
