#include "catch_amalgamated.hpp"
#include "game/source_math.hpp"
#include "game/source_math_double.hpp"
#include "teleport_chain/generate.hpp"
#include "teleport_chain/tp_ring_queue.hpp"
#include "teleport_chain/ulp_diff.hpp"
#include "prng.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <format>
#include <queue>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define CATCH_SEED ((uint32_t)42069)

/*
* TODO: apparently DYNAMIC_SECTION will cause the test to get run once for each generated section.
* This seems to be slowing down the tests a lot, and definitely messes with debugging and rng
* state, maybe there's a workaround?
*/

int main(int argc, char* argv[])
{
    mon::MonocleFloatingPointScope scope{};
    Catch::Session session;
    session.libIdentify();
    session.configData().rngSeed = CATCH_SEED;
    // session.configData().shouldDebugBreak = true;

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

static mon::Portal RandomPortal(small_prng& rng)
{
    constexpr float p_min = -3000.f, p_max = 3000.f, a_min = -180.f, a_max = 180.f;
    return mon::Portal{
        mon::Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
        mon::QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
    };
}

using tp_queue = mon::RingQueue<mon::TeleportChainInternalState::queue_entry, 8>;
using ref_std_queue = std::deque<tp_queue::value_type>;

TEST_CASE("Teleport queue push and pop all")
{
    tp_queue tpq;
    ref_std_queue tpq_ref;

    REQUIRE(tpq.empty());

    size_t num_elems = GENERATE(1, 2, 4, 100, 5000);

    for (size_t n_iters = 0; n_iters < 3; n_iters++) {
        DYNAMIC_SECTION("push iteration " << n_iters)
        {
            for (size_t i = 0; i < num_elems; i++) {
                tp_queue::value_type val = i * 69420;
                tpq.push_back(val);
                tpq_ref.push_back(val);

                REQUIRE(tpq.size() == tpq_ref.size());
            }

            for (size_t i = 0; i < num_elems; i++) {
                REQUIRE(tpq.front() == tpq_ref.front());
                tpq.pop_front();
                tpq_ref.pop_front();
                REQUIRE(tpq.size() == tpq_ref.size());
            }

            REQUIRE(tpq.empty());
        }
    }
}

TEST_CASE("Teleport queue push and pop some")
{
    tp_queue tpq;
    ref_std_queue tpq_ref;
    REQUIRE(tpq.empty());

    size_t num_elems_push = GENERATE(3, 4, 100, 5000);
    size_t num_elems_pop = num_elems_push / 3;
    bool clear_elements_after_pops = GENERATE(0, 1);

    for (size_t n_iters = 0; n_iters < 3; n_iters++) {
        for (size_t i = 0; i < num_elems_push; i++) {
            tp_queue::value_type val = i * 69420;
            tpq.push_back(val);
            tpq_ref.push_back(val);
            REQUIRE(tpq.size() == tpq_ref.size());
        }

        for (size_t i = 0; i < num_elems_pop; i++) {
            REQUIRE(tpq.front() == tpq_ref.front());
            tpq.pop_front();
            tpq_ref.pop_front();
            REQUIRE(tpq.size() == tpq_ref.size());
        }

        if (clear_elements_after_pops) {
            tpq.clear();
            tpq_ref.clear();
            REQUIRE(tpq.empty());
        }
    }

    tpq.clear();
    REQUIRE(tpq.empty());
}

TEST_CASE("Teleport queue iterators")
{
    tp_queue tpq;
    REQUIRE(tpq.empty());
    REQUIRE_FALSE(tpq.begin() != tpq.end());

    size_t n_elemspush = GENERATE(1, 4, 5, 20, 100);
    size_t n_elemspop = n_elemspush / 3;

    for (size_t i = 0; i < n_elemspush; i++)
        tpq.push_back((int)(i * 69420));

    for (size_t i = 0; i < n_elemspop; i++)
        tpq.pop_front();

    size_t i = n_elemspop;
    for (auto it = tpq.begin(); it != tpq.end(); ++it, ++i)
        REQUIRE(*it == (int)(i * 69420));
}

TEST_CASE("Teleport queue move constructor")
{
    tp_queue tpq;
    REQUIRE(tpq.empty());

    size_t n_elemspush = GENERATE(0, 1, 4, 8, 24, 1000);
    size_t n_elemspop = n_elemspush / 3;

    for (size_t i = 0; i < n_elemspush; i++)
        tpq.push_back((int)(i * 69420));

    for (size_t i = 0; i < n_elemspop; i++)
        tpq.pop_front();

    size_t n_elems = tpq.size();
    tp_queue tpq2{std::move(tpq)};
    REQUIRE(tpq2.size() == n_elems);

    size_t i = n_elemspop;
    for (auto it = tpq2.begin(); it != tpq2.end(); ++it, ++i)
        REQUIRE(*it == (int)(i * 69420));
}

TEST_CASE("Teleport queue move assignment")
{
    tp_queue tpq;
    REQUIRE(tpq.empty());

    size_t n_elemspush = GENERATE(0, 1, 4, 8, 24, 1000);
    size_t n_elemspop = n_elemspush / 3;

    for (size_t i = 0; i < n_elemspush; i++)
        tpq.push_back((int)(i * 69420));

    for (size_t i = 0; i < n_elemspop; i++)
        tpq.pop_front();

    size_t n_elems = tpq.size();
    tp_queue tpq2;
    tpq2.push_back(5);
    tpq2 = std::move(tpq);

    REQUIRE(tpq2.size() == n_elems);

    size_t i = n_elemspop;
    for (auto it = tpq2.begin(); it != tpq2.end(); ++it, ++i)
        REQUIRE(*it == (int)(i * 69420));
}

TEST_CASE("ShouldTeleport (no portal hole check)")
{
    REPEAT_TEST(1000);
    static small_prng rng;
    mon::Portal p = RandomPortal(rng);
    bool is_player = rng.next_bool();
    INFO("entity is " << (is_player ? "player" : "non-player"));

    if (is_player) {
        mon::Entity ent1 = mon::Entity::CreatePlayerFromCenter(p.pos - p.f, true);
        REQUIRE(p.ShouldTeleport(ent1, false));
        mon::Entity ent2 = mon::Entity::CreatePlayerFromCenter(p.pos + p.f, true);
        REQUIRE_FALSE(p.ShouldTeleport(ent2, false));
    } else {
        mon::Entity ent1 = mon::Entity::CreateBall(p.pos - p.f, 0.f);
        REQUIRE(p.ShouldTeleport(ent1, false));
        mon::Entity ent2 = mon::Entity::CreateBall(p.pos + p.f, 0.f);
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

    mon::Vector pos{rng.next_float(-3000.f, 3000.f), rng.next_float(-3000.f, 3000.f), rng.next_float(-3000.f, 3000.f)};
    mon::QAngle ang;
    if (axial) {
        // BoxOnPlaneSide has special axial cases, test those too
        float roll = rng.next_bool() ? rng.next_float(-180.f, 180.f) : 0.f;
        switch (rng.next_int(0, 6)) {
            case 0:
                ang = {0, 0, roll};
                break;
            case 1:
                ang = {0, 90, roll};
                break;
            case 2:
                ang = {0, 180, roll};
                break;
            case 3:
                ang = {0, -90, roll};
                break;
            case 4:
                ang = {90, 0, roll};
                break;
            case 5:
                ang = {-90, 0, roll};
                break;
            default:
                FAIL();
        }
    } else {
        ang = mon::QAngle{rng.next_float(-180.f, 180.f), rng.next_float(-180.f, 180.f), rng.next_float(-180.f, 180.f)};
    }

    mon::Portal p{pos, ang};

    INFO("corner: " << ((corner_bits & 1) ? "-f " : "+f ") << ((corner_bits & 2) ? "-r " : "+r ")
                    << ((corner_bits & 4) ? "-u" : "+u"));

    mon::Vector corner_off = p.f * ((corner_bits & 1) ? -mon::PORTAL_HOLE_DEPTH : 0.f) +
                             p.r * ((corner_bits & 2) ? -mon::PORTAL_HALF_WIDTH : mon::PORTAL_HALF_WIDTH) +
                             p.u * ((corner_bits & 4) ? -mon::PORTAL_HALF_HEIGHT : mon::PORTAL_HALF_HEIGHT);

    constexpr float SPACING = 20.f;

    INFO("grid offset: (f*" << (-1.5f + ((grid_bits >> 0) & 0b11)) * ((corner_bits & 1) ? -1.f : 1.f) << " + r*"
                            << (-1.5f + ((grid_bits >> 2) & 0b11)) * ((corner_bits & 2) ? -1.f : 1.f) << " + u*"
                            << (-1.5f + ((grid_bits >> 4) & 0b11)) * ((corner_bits & 4) ? -1.f : 1.f) << ") * "
                            << SPACING);

    // the grid offset is set so that bigger grid bits point are further away from the portal center, this makes the should_teleport condition simplier
    mon::Vector grid_off = p.f * (-1.5f + ((grid_bits >> 0) & 0b11)) * ((corner_bits & 1) ? -1.f : 1.f) +
                           p.r * (-1.5f + ((grid_bits >> 2) & 0b11)) * ((corner_bits & 2) ? -1.f : 1.f) +
                           p.u * (-1.5f + ((grid_bits >> 4) & 0b11)) * ((corner_bits & 4) ? -1.f : 1.f);

    mon::Vector ent_pos = p.pos + corner_off + grid_off * SPACING;
    mon::Entity ent;
    switch (ent_type) {
        case 0: {
            UNSCOPED_INFO("entity is small ball");
            ent = mon::Entity::CreateBall(ent_pos, 1.f);
            break;
        }
        case 1: {
            UNSCOPED_INFO("entity is ball w/ radius " << SPACING);
            ent = mon::Entity::CreateBall(ent_pos, SPACING);
            break;
        }
        case 2: {
            UNSCOPED_INFO("entity is crouched player");
            ent = mon::Entity::CreatePlayerFromCenter(ent_pos, true);
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

TEST_CASE("Portal from string (newlocation)")
{
    REPEAT_TEST(100);
    small_prng rng{(uint32_t)_test_it};

    mon::Portal p_ref = RandomPortal(rng);
    std::string cmd = p_ref.NewLocationCmd("blue");

    auto res = mon::Portal::FromString(cmd);
    REQUIRE(res.has_value());
    REQUIRE(res->second.ptr == &*--cmd.end());
    REQUIRE(res->first.pos == p_ref.pos);
    REQUIRE(res->first.ang == p_ref.ang);
}

TEST_CASE("Portal from string (fail)")
{
    auto res = mon::Portal::FromString("1 2 3 4 5");
    REQUIRE_FALSE(res.has_value());
}

TEST_CASE("Teleport")
{
    REPEAT_TEST(10000);
    static small_prng rng;
    mon::Portal p1 = RandomPortal(rng);
    mon::Portal p2 = RandomPortal(rng);
    int translate_mask = rng.next_int(0, 16);
    int order = rng.next_int(0, (int)mon::PlacementOrder::COUNT);
    bool p1_teleporting = rng.next_bool();
    bool is_player = rng.next_bool();
    INFO("entity is " << (is_player ? "player" : "non-player"));

    mon::PortalPair pp{p1, p2, (mon::PlacementOrder)order};

    mon::Vector off1{0.f, 0.f, 0.f}, off2{0.f, 0.f, 0.f};
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
    mon::Vector pt1 = p1.pos + (sign ? off1 : -off1);
    mon::Vector pt2 = p2.pos + (sign ? off2 : -off2);

    mon::Vector center = p1_teleporting ? pt1 : pt2;
    mon::Entity ent =
        is_player ? mon::Entity::CreatePlayerFromCenter(center, true) : mon::Entity::CreateBall(center, 0.f);
    ent = pp.Teleport(ent, p1_teleporting);
    REQUIRE_THAT(ent.GetCenter().DistToSqr(p1_teleporting ? pt2 : pt1), Catch::Matchers::WithinAbs(0.0, 0.01));
}

TEST_CASE("Nudging point towards portal plane (close)")
{
    REPEAT_TEST(10000);
    static small_prng rng;
    mon::Portal p = RandomPortal(rng);
    for (int i = 0; i < 3; i++)
        if (fabsf(p.pos[i]) < 0.5f)
            return;
    int translate_mask = rng.next_int(0, 8);
    bool is_player = rng.next_bool();
    INFO("entity is " << (is_player ? "player" : "non-player"));

    mon::Vector off{0.f, 0.f, 0.f};
    if (translate_mask & 1)
        off += p.r;
    if (translate_mask & 2)
        off += p.u;
    mon::Vector pt = p.pos + ((translate_mask & 4) ? off : -off);
    // add a little "random" nudge
    pt[0] += pt[0] / 100000.f;

    mon::Entity ent = is_player ? mon::Entity::CreatePlayerFromCenter(pt, true) : mon::Entity::CreateBall(pt, 0.f);
    auto [nudged_ent, plane_dist] = ProjectEntityToPortalPlane(ent, p);
    REQUIRE(!!plane_dist.is_valid);
    REQUIRE(p.ShouldTeleport(ent, false) == !!plane_dist.pt_was_behind_portal);
    // nudged entity is guaranteed to be behind the portal
    REQUIRE(p.ShouldTeleport(nudged_ent, false));
    // nudge the entity across the portal boundary
    mon::Vector& ent_pos = nudged_ent.GetPosRef();
    float target = ent_pos[plane_dist.ax] + p.plane.n[plane_dist.ax];
    ent_pos[plane_dist.ax] = std::nextafterf(ent_pos[plane_dist.ax], target);
    REQUIRE_FALSE(p.ShouldTeleport(nudged_ent, false));
}

// basically this test shouldn't take forever (the old version of this would take ages)
TEST_CASE("Nudging point towards portal plane (far)")
{
    static small_prng rng;
    mon::Portal p = RandomPortal(rng);
    mon::Entity ent = mon::Entity::CreatePlayerFromCenter(p.pos + p.r * 2.5f - p.u * 1.5f + p.f * 12345.f, false);
    auto [nudged_ent, plane_dist] = mon::ProjectEntityToPortalPlane(ent, p);
    REQUIRE(!!plane_dist.is_valid);
    REQUIRE_FALSE(p.ShouldTeleport(ent, false));
    // nudged entity is guaranteed to be behind the portal
    REQUIRE(p.ShouldTeleport(nudged_ent, false));
    // nudge the entity across the portal boundary
    mon::Vector& ent_pos = nudged_ent.GetPosRef();
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
    mon::PortalPair pp{
        {255.96875f, -161.01294f, 54.031242f},
        {-0.f, 180.f, 0.f},
        {-127.96875f, -191.24300f, 182.03125f},
        {0.f, 0.f, 0.f},
        mon::PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED,
    };

    mon::TeleportChainParams params{
        &pp,
        mon::Entity::CreatePlayerFromCenter({-127.96876f, -191.24300f, 182.03125f}, true),
    };
    params.record_flags = mon::TCRF_RECORD_ALL;
    mon::TeleportChainResult result;

    mon::Vector target_vag_pos = pp.Teleport(params.ent.GetCenter(), true);

    const int n_teleports_success = 3;

    for (int n_max_teleports = 0; n_max_teleports < n_teleports_success + 2; n_max_teleports++) {
        DYNAMIC_SECTION("teleport limit is " << n_max_teleports)
        {
            params.n_max_teleports = n_max_teleports;
            mon::GenerateTeleportChain(params, result);

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
    mon::PortalPair pp{
        {255.96875f, -223.96875f, 54.031242f},
        {-0.f, 180.f, 0.f},
        {-127.96875f, -191.24300f, 182.03125f},
        {0.f, 0.f, 0.f},
        mon::PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED,
    };

    mon::TeleportChainParams params{
        &pp,
        mon::Entity::CreatePlayerFromCenter({-127.96876f, -191.24300f, 182.03125f}, true),
    };
    params.record_flags = mon::TCRF_RECORD_ALL;
    mon::TeleportChainResult result;

    mon::Vector target_vag_pos = pp.Teleport(params.ent.GetCenter(), true);

    const int n_teleports_success = 5;

    for (int n_max_teleports = 0; n_max_teleports < n_teleports_success + 2; n_max_teleports++) {
        DYNAMIC_SECTION("teleport limit is " << n_max_teleports)
        {
            params.n_max_teleports = n_max_teleports;
            mon::GenerateTeleportChain(params, result);
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
    mon::PortalPair pp{
        {1001.2641f, 40.5883064f, 64.03125f},
        {-90.f, -92.752083f, 0.f},
        {511.96875f, 25.968760f, 54.031242f},
        {-0.f, 180.f, 0.f},
        mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
    };

    mon::TeleportChainParams params{
        &pp,
        mon::Entity::CreatePlayerFromOrigin({1000.405640f, 51.970764f, 46.03125f}, true),
    };
    params.project_to_first_portal_plane = false;
    params.record_flags = mon::TCRF_RECORD_ALL;
    mon::TeleportChainResult result;

    /*
    * This is the position on the tick after the teleport according to the trace. It's a few units
    * off and I'm not really sure why. Gravity and floor portal exit velocity don't seem to account
    * for the full difference.
    */
    mon::Entity target_ent = mon::Entity::CreatePlayerFromOrigin({1050.953125f, 539.331055f, 563.865845f}, true);

    const int n_teleports_success = 6;

    for (int n_max_teleports = 0; n_max_teleports < n_teleports_success + 2; n_max_teleports++) {
        DYNAMIC_SECTION("teleport limit is " << n_max_teleports)
        {
            params.n_max_teleports = n_max_teleports;
            mon::GenerateTeleportChain(params, result);
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
                    REQUIRE_THAT(result.ents[1].GetCenter().DistTo(pp.orange.pos), Catch::Matchers::WithinAbs(0, 15.f));
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
    mon::PortalPair pp{
        {255.96875f, -161.01295f, 201.96877f},
        {-0.f, 180.f, 0.f},
        {-127.96875f, -191.24300f, 182.03125f},
        {0.f, 0.f, 0.f},
        mon::PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED,
    };

    mon::TeleportChainParams params{
        &pp,
        mon::Entity::CreatePlayerFromCenter({-127.96876f, -191.24300f, 182.03125f}, true),
    };
    params.n_max_teleports = 200;
    params.record_flags = mon::TCRF_RECORD_ALL;
    mon::TeleportChainResult result;

    mon::GenerateTeleportChain(params, result);

    REQUIRE(result.total_n_teleports == 82);
    REQUIRE(result.cum_teleports == 0);
    REQUIRE_FALSE(result.max_tps_exceeded);
    REQUIRE(result.tp_dirs.size() == 82);
    REQUIRE(result.portal_plane_diffs.size() == 83);
    REQUIRE(result.portal_plane_diffs.back().pt_was_behind_portal);
}

TEST_CASE("19 Lochness")
{
    mon::PortalPair pp{
        {-416.247498f, 735.368835f, 255.96875f},
        {90.f, -90.2385559f, 0.f},
        {-394.776428f, -56.0312462f, 38.8377686f},
        {-44.9994202f, 180.f, 0.f},
        mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
    };

    mon::TeleportChainParams params{
        &pp,
        mon::Entity::CreatePlayerFromOrigin({-416.247498f, 735.368835f, 219.97f}, false),
    };
    params.project_to_first_portal_plane = false;

    mon::TeleportChainResult result;
    mon::GenerateTeleportChain(params, result);

    mon::Entity roughExpectedEnt = mon::Entity::CreatePlayerFromOrigin({398.634583f, 599.235168f, 400.928467f}, true);

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
    mon::PortalPair pp{
        {-141.904663f, 730.353394f, -191.417892f},
        {15.9453955f, 180.f, 0.f},
        {-233.054321f, 652.257446f, -255.96875f},
        {-90.f, 155.909348f, 0.f},
        mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
    };

    mon::TeleportChainParams params{
        &pp,
        mon::Entity::CreatePlayerFromOrigin({-136.239883, 726.482483, -240.416977}, false),
    };
    params.project_to_first_portal_plane = false;

    mon::TeleportChainResult result;
    mon::GenerateTeleportChain(params, result);

    mon::Entity roughExpectedEnt = mon::Entity::CreatePlayerFromOrigin({-93.691399f, 636.419739f, -166.272324f}, true);

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
    mon::PortalPair pp{
        {-474.710541f, -1082.65906f, 182.03125f},
        {0.00538991531f, 135.f, 0.f},
        {-735.139282f, -923.540344f, 128.03125f},
        {-90.f, 55.0390396f, 0.f},
        mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
    };

    mon::TeleportChainParams params{
        &pp,
        mon::Entity::CreatePlayerFromOrigin({-473.604370f, -1082.235498f, 128.031250f}, false),
    };
    params.project_to_first_portal_plane = false;

    mon::TeleportChainResult result;
    mon::GenerateTeleportChain(params, result);

    mon::Entity roughExpectedEnt =
        mon::Entity::CreatePlayerFromOrigin({-629.746887f, -1311.233398f, 138.827957f}, true);

    REQUIRE(result.total_n_teleports == 3);
    REQUIRE(result.cum_teleports == -1);
    REQUIRE_FALSE(result.max_tps_exceeded);
    REQUIRE(result.ent.is_player);
    REQUIRE(result.ent.player.crouched);
    REQUIRE_THAT(std::sqrtf(result.ent.GetCenter().DistToSqr(roughExpectedEnt.GetCenter())),
                 Catch::Matchers::WithinAbs(0, 2.f));
}

class SptIpcConn {

    const USHORT spt_port = 27182;
    const char* ip_addr = "127.0.0.1";

    // clang-format off
    struct _WinSock {
        int _ret;
        WSADATA wsadata;
        _WinSock() { _ret = WSAStartup(MAKEWORD(2, 2), &wsadata); if (_ret != 0) SKIP("WSAStartup failed");}
        ~_WinSock() { if (_ret == 0) WSACleanup(); }
    } winsock{};
    // clang-format on

    SOCKET sock = INVALID_SOCKET;
    bool _cleanup_sock = false;
    char recv_buf[1024]{};
    int off = 0;
    int recvLen = 0;

    // TODO there's seems to be a bug in Catch2 when skipping from a class fixture ctor, update Catch2 and see if that's fixed!
    std::optional<std::string> defer_skip_message;

public:
    SptIpcConn()
    {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            defer_skip_message = "Socket creation failed";
            return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip_addr, &server_addr.sin_addr.s_addr);
        server_addr.sin_port = htons(spt_port);

        _cleanup_sock = true;
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
            defer_skip_message = "Failed to connect to SPT";
    }

    ~SptIpcConn()
    {
        if (_cleanup_sock)
            closesocket(sock);
    }

    void SendCmd(const std::string& s)
    {
        if (defer_skip_message.has_value())
            SKIP(*defer_skip_message);

        recvLen = -1;
        std::string send_str = std::format("{{\"type\":\"cmd\",\"cmd\":\"{}\"}}", s);
        int ret = send(sock, send_str.c_str(), send_str.size() + 1, 0);
        if (ret == SOCKET_ERROR)
            SKIP("send failed (" << ret << ")");
    }

    const char* BufPtr() const
    {
        return recv_buf + off;
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
            recvLen = recv(sock, recv_buf, sizeof recv_buf, 0);
            if (recvLen < 0 || recvLen >= sizeof recv_buf)
                SKIP("recv failed (" << recvLen << ")");
        }
    }

    void RecvAck()
    {
        NextRecvMsg();
        REQUIRE_FALSE(!!strcmp(BufPtr(), "{\"type\":\"ack\"}"));
    }
};

struct SptIpcConnFixture {
    mutable SptIpcConn conn;
};

/*
* To use:
* - open the game and load a sufficiently recent version of SPT (anything after 03-2025 work)
* - run `spt_ipc 1`
* - create an orange and blue portal and set their name using `picker` & `ent_setname` to 'blue'/'orange'
* - noclip and crouch (with toggle duck)
* - run the tests
* 
* TODO: figure out how to handle the standing case. It's possible for us to end up with portals
* that give us exit velocity and teleport us back into the entry portal. Now that I think about it,
* this is probably possible for the crouched case as well.
*/
TEST_CASE_PERSISTENT_FIXTURE(SptIpcConnFixture, "SPT with IPC")
{
    conn.SendCmd(
        "sv_cheats 1; spt_prevent_vag_crash 1; spt_focus_nosleep 1; spt_noclip_noslowfly 1; host_timescale 20; "
        "spt_ipc_properties 1 m_fFlags");
    conn.RecvAck();
    conn.NextRecvMsg();

    int player_flags;
    int n_args = _snscanf_s(conn.BufPtr(),
                            conn.BufLen(),
                            "{\"entity\":{\"m_fFlags\":%d},\"exists\":true,\"type\":\"ent\"}",
                            &player_flags);
    REQUIRE(n_args == 1);

    bool player_crouched = player_flags & 2;

    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    small_prng rng;
    mon::TeleportChainParams params;
    mon::TeleportChainResult result;

    int iteration = 0;
    while (iteration < 1000) {
        mon::Portal blue = RandomPortal(rng);
        mon::Portal orange = RandomPortal(rng);
        /*
        * Don't use portals that are too close to each other - this can cause false positives with
        * the distance threshold to the teleport destination.
        */
        if (blue.pos.DistToSqr(orange.pos) < 500 * 500)
            continue;

        mon::PortalPair pp{blue, orange, mon::PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION};

        params.pp = &pp;
        params.ent = mon::Entity::CreatePlayerFromCenter(blue.pos, player_crouched);
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

        mon::Entity tmp_player = mon::Entity::CreatePlayerFromCenter(blue.pos + blue.f, player_crouched);
        conn.SendCmd(std::format("{}; {}", pp.NewLocationCmd("; ", true), tmp_player.SetPosCmd()));
        conn.RecvAck();
        conn.SendCmd(result.ents[0].SetPosCmd());
        conn.RecvAck();

        // timescale 1: sleep for 350ms, timescale 20: sleep for 10ms
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        conn.SendCmd("spt_ipc_properties 1 m_vecOrigin");
        conn.RecvAck();
        conn.NextRecvMsg();
        mon::Vector actual_player_pos{};
        n_args = _snscanf_s(conn.BufPtr(),
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
        if (player_crouched || !result.ent.player.crouched) {
            REQUIRE(actual_player_pos.DistToSqr(result.ent.player.origin) < 100 * 100);
        } else {
            // the AG force crouched the player and the above estimate may be off
            mon::Entity better_ent_estimate = mon::Entity::CreatePlayerFromCenter(result.ent.GetCenter(), false);
            REQUIRE(actual_player_pos.DistToSqr(better_ent_estimate.player.origin) < 100 * 100);
        }
        printf("iteration %d: %s", iteration, result.cum_teleports == -1 ? "VAG\n" : "Normal teleport\n");
        iteration++;
    }
}

class UlpDiffCompareTest {
public:
    template <typename FT, typename DT>
    static void RequireStructTolerance(const char* unique_struct_name,
                                       const FT& f,
                                       const DT& d,
                                       double max_rel_off,
                                       double max_abs_off)
    {
        static_assert(sizeof(FT) / sizeof(float) == sizeof(DT) / sizeof(double));
        DYNAMIC_SECTION("comparing '" << unique_struct_name << "': '" << typeid(FT).name() << "' with '"
                                      << typeid(DT).name() << "'")
        {
            for (size_t i = 0; i < sizeof(FT) / sizeof(float); i++) {
                DYNAMIC_SECTION("struct field: " << i)
                {
                    float fv = ((const float*)&f)[i];
                    double dv = ((const double*)&d)[i];
                    double ulp_diff = mon::ulp::UlpDiffD(dv, fv);
                    INFO("float val: " << std::format(MON_F_FMT, fv) << ", double val: " << std::format(MON_D_FMT, dv)
                                       << ", ulp diff: " << std::format("{:.2f}", ulp_diff));

                    bool do_abs = std::fabsf(fv) <= 0.1f;
                    if ((do_abs && max_abs_off == 0.0) || (!do_abs && max_rel_off == 0.0)) {
                        INFO("using ulp comparison");
                        REQUIRE(ulp_diff <= 0.5);
                    } else if (do_abs) {
                        INFO("using absolute comparison");
                        REQUIRE_THAT(fv, Catch::Matchers::WithinAbs(dv, max_abs_off));
                    } else {
                        INFO("using relative comparison");
                        REQUIRE_THAT(fv, Catch::Matchers::WithinRel(dv, max_rel_off));
                    }
                }
            }
        }
    }

    void TestCase()
    {
        REPEAT_TEST(1000);
        small_prng rng{(uint32_t)_test_it};
        mon::PortalPair pp{
            RandomPortal(rng),
            RandomPortal(rng),
            (mon::PlacementOrder)rng.next_int(0, (int)mon::PlacementOrder::COUNT),
        };

        mon::PortalPairD ppd{pp};

        INFO(pp.blue.NewLocationCmd("blue"));
        INFO(pp.orange.NewLocationCmd("orange"));

        for (int i = 0; i < 2; i++) {
            DYNAMIC_SECTION("portal color: " << (i == 0 ? "blue" : "orange"))
            {
                const mon::Portal& p = i == 0 ? pp.blue : pp.orange;
                const mon::PortalD& pd = i == 0 ? ppd.blue : ppd.orange;

                RequireStructTolerance("portal pos", p.pos, pd.pos, 0, 0);
                RequireStructTolerance("portal f", p.f, pd.f, 1e-4, 1e-5);
                RequireStructTolerance("portal r", p.r, pd.r, 1e-4, 1e-5);
                RequireStructTolerance("portal u", p.u, pd.u, 1e-4, 1e-5);
                RequireStructTolerance("portal plane", p.plane, pd.plane, 0.001, 0.001);
                RequireStructTolerance("world to portal", p.mat, pd.mat, 0.001, 0.001);

                const mon::VMatrix& tp = i == 0 ? pp.b_to_o : pp.o_to_b;
                const mon::VMatrixD& tpd = i == 0 ? ppd.b_to_o : ppd.o_to_b;
                RequireStructTolerance("teleport matrix", tp, tpd, 0.002, 0.002);

                // now teleport an entity

                bool create_player = rng.next_bool();
                bool player_crouched = rng.next_bool();
                mon::Entity ent = create_player ? mon::Entity::CreatePlayerFromCenter(p.pos, player_crouched)
                                                : mon::Entity::CreateBall(p.pos, 1.f);

                auto [proj_ent, _] = mon::ProjectEntityToPortalPlane(ent, p);
                mon::EntityD ent_d{proj_ent};
                RequireStructTolerance("entity center", proj_ent.GetCenter(), ent_d.GetCenter(), 0, 0);

                mon::Entity tp_ent = pp.Teleport(proj_ent, i == 0);
                mon::EntityD tp_ent_d = ppd.Teleport(ent_d, i == 0);
                RequireStructTolerance("entity center after teleport",
                                       tp_ent.GetCenter(),
                                       tp_ent_d.GetCenter(),
                                       0.005,
                                       0.005);
            }
        }
    }
};

METHOD_AS_TEST_CASE(UlpDiffCompareTest::TestCase, "Double math");
