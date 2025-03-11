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
        Entity ent1{p.pos - p.f};
        REQUIRE(p.ShouldTeleport(ent1, false));
        Entity ent2{p.pos + p.f};
        REQUIRE_FALSE(p.ShouldTeleport(ent2, false));
    } else {
        Entity ent1{p.pos - p.f, 0.f};
        REQUIRE(p.ShouldTeleport(ent1, false));
        Entity ent2{p.pos + p.f, 0.f};
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
            ent = Entity{ent_pos, 1.f};
            break;
        }
        case 1: {
            UNSCOPED_INFO("entity is ball w/ radius " << SPACING);
            ent = Entity(ent_pos, SPACING);
            break;
        }
        case 2: {
            UNSCOPED_INFO("entity is crouched player");
            ent = Entity{ent_pos};
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
    Entity ent = is_player ? Entity{center} : Entity{center, 0.f};
    pp.Teleport(ent, p1_teleporting);
    REQUIRE_THAT(ent.GetCenter().DistToSqr(p1_teleporting ? pt2 : pt1), Catch::Matchers::WithinAbs(0.0, 0.01));
}

TEST_CASE("Nudging point towards portal plane")
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

    Entity ent = is_player ? Entity{pt} : Entity{pt, 0.f};
    VecUlpDiff ulp_diff;
    Entity new_ent = NudgeEntityBehindPortalPlane(ent, p, &ulp_diff);
    REQUIRE(p.ShouldTeleport(new_ent, false));
    REQUIRE(p.ShouldTeleport(ent, false) == ulp_diff.PtWasBehindPlane());

    if (ulp_diff.ax > 0) {
        // now nudge across the portal boundary (requires an ulp diff from the previous step)
        float target = new_ent.origin[ulp_diff.ax] + p.plane.n[ulp_diff.ax];
        new_ent.origin[ulp_diff.ax] = std::nextafterf(new_ent.origin[ulp_diff.ax], target);
        REQUIRE_FALSE(p.ShouldTeleport(new_ent, false));
    }
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
    Entity player{Vector{-127.96876f, -191.24300f, 182.03125f}};

    const int n_teleports_success = 3;

    for (int n_max_teleports = 0; n_max_teleports < n_teleports_success + 2; n_max_teleports++) {
        DYNAMIC_SECTION("teleport limit is " << n_max_teleports)
        {
            Vector target_vag_pos = pp.Teleport(player.GetCenter(), true);
            EntityInfo ent_info{
                .n_ent_children = N_CHILDREN_PLAYER_WITH_PORTAL_GUN,
                .origin_inbounds = false,
            };
            TeleportChain chain;
            chain.Generate(pp, false, player, ent_info, n_max_teleports);
            Entity new_player = chain.transformed_ent;
            int n_actual_teleports = n_max_teleports > n_teleports_success ? n_teleports_success : n_max_teleports;

            REQUIRE(chain.max_tps_exceeded == (n_actual_teleports < n_teleports_success));
            REQUIRE(chain.tp_dirs.size() == n_actual_teleports);
            REQUIRE(chain.tps_queued.size() == (n_actual_teleports + 1));
            REQUIRE(chain.pts.size() == (n_actual_teleports + 1));
            REQUIRE(chain.ulp_diffs.size() == (n_actual_teleports + 1));
            REQUIRE(chain.cum_primary_tps == std::vector<int>{0, 1, 0, -1}[n_actual_teleports]);

            switch (n_actual_teleports) {
                case 3:
                    REQUIRE(chain.tp_dirs[2] == false);
                    REQUIRE(chain.tps_queued[3] == 0);
                    REQUIRE_THAT(chain.pts[3].DistToSqr(target_vag_pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE_THAT(new_player.GetCenter().DistToSqr(target_vag_pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE_FALSE(chain.ulp_diffs[3].Valid());
                    [[fallthrough]];
                case 2:
                    REQUIRE(chain.tp_dirs[1] == false);
                    REQUIRE(chain.tps_queued[2] == 0);
                    REQUIRE_THAT(chain.pts[2].DistToSqr(pp.orange.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(chain.ulp_diffs[2].ax == 0);
                    REQUIRE_FALSE(chain.ulp_diffs[2].PtWasBehindPlane());
                    [[fallthrough]];
                case 1:
                    REQUIRE(chain.tp_dirs[0] == true);
                    REQUIRE(chain.tps_queued[1] == -2);
                    REQUIRE_THAT(chain.pts[1].DistToSqr(pp.blue.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(chain.ulp_diffs[1].ax == 0);
                    REQUIRE(chain.ulp_diffs[1].PtWasBehindPlane());
                    [[fallthrough]];
                case 0:
                    REQUIRE(chain.tps_queued[0] == 1);
                    REQUIRE_THAT(chain.pts[0].DistToSqr(pp.orange.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(chain.ulp_diffs[0].ax == 0);
                    REQUIRE(chain.ulp_diffs[0].PtWasBehindPlane());
                    break;
                default:
                    FAIL();
            }
        }
    }
}

TEST_CASE("Teleport chain results in 5 teleports")
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
    Entity player{Vector{-127.96876f, -191.24300f, 182.03125f}};

    const int n_teleports_success = 5;

    for (int n_max_teleports = 0; n_max_teleports < n_teleports_success + 2; n_max_teleports++) {
        DYNAMIC_SECTION("teleport limit is " << n_max_teleports)
        {
            Vector target_vag_pos = pp.Teleport(player.GetCenter(), true);

            EntityInfo ent_info{
                .n_ent_children = N_CHILDREN_PLAYER_WITH_PORTAL_GUN,
                .origin_inbounds = false,
            };
            TeleportChain chain;
            chain.Generate(pp, false, player, ent_info, n_max_teleports);
            Entity new_player = chain.transformed_ent;
            int n_actual_teleports = n_max_teleports > n_teleports_success ? n_teleports_success : n_max_teleports;

            REQUIRE(chain.max_tps_exceeded == (n_actual_teleports < n_teleports_success));
            REQUIRE(chain.tp_dirs.size() == n_actual_teleports);
            REQUIRE(chain.tps_queued.size() == (n_actual_teleports + 1));
            REQUIRE(chain.pts.size() == (n_actual_teleports + 1));
            REQUIRE(chain.ulp_diffs.size() == (n_actual_teleports + 1));
            REQUIRE(chain.cum_primary_tps == std::vector<int>{0, 1, 0, -1, 0, 1}[n_actual_teleports]);

            switch (n_actual_teleports) {
                case 5:
                    REQUIRE(chain.tp_dirs[4] == true);
                    REQUIRE(chain.tps_queued[5] == 0);
                    REQUIRE_THAT(chain.pts[5].DistToSqr(pp.blue.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE_THAT(new_player.GetCenter().DistToSqr(pp.blue.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(chain.ulp_diffs[5].ax == 0);
                    REQUIRE_FALSE(chain.ulp_diffs[5].PtWasBehindPlane());
                    [[fallthrough]];
                case 4:
                    REQUIRE(chain.tp_dirs[3] == true);
                    REQUIRE(chain.tps_queued[4] == 1);
                    REQUIRE_THAT(chain.pts[4].DistToSqr(pp.orange.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(chain.ulp_diffs[4].PtWasBehindPlane());
                    [[fallthrough]];
                case 3:
                    REQUIRE(chain.tp_dirs[2] == false);
                    REQUIRE(chain.tps_queued[3] == 0);
                    REQUIRE_THAT(chain.pts[3].DistToSqr(target_vag_pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE_FALSE(chain.ulp_diffs[3].Valid());
                    [[fallthrough]];
                case 2:
                    REQUIRE(chain.tp_dirs[1] == false);
                    REQUIRE(chain.tps_queued[2] == 1);
                    REQUIRE_THAT(chain.pts[2].DistToSqr(pp.orange.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(chain.ulp_diffs[2].ax == 0);
                    REQUIRE(chain.ulp_diffs[2].PtWasBehindPlane());
                    [[fallthrough]];
                case 1:
                    REQUIRE(chain.tp_dirs[0] == true);
                    REQUIRE(chain.tps_queued[1] == -2);
                    REQUIRE_THAT(chain.pts[1].DistToSqr(pp.blue.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(chain.ulp_diffs[1].ax == 0);
                    REQUIRE(chain.ulp_diffs[1].PtWasBehindPlane());
                    [[fallthrough]];
                case 0:
                    REQUIRE(chain.tps_queued[0] == 1);
                    REQUIRE_THAT(chain.pts[0].DistToSqr(pp.orange.pos), Catch::Matchers::WithinAbs(0, .5f));
                    REQUIRE(chain.ulp_diffs[0].ax == 0);
                    REQUIRE(chain.ulp_diffs[0].PtWasBehindPlane());
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
    Entity player{Vector{-127.96876f, -191.24300f, 182.03125f}};
    EntityInfo ent_info{
        .n_ent_children = N_CHILDREN_PLAYER_WITH_PORTAL_GUN,
        .origin_inbounds = false,
    };
    TeleportChain chain;
    chain.Generate(pp, false, player, ent_info, 200);
    REQUIRE_FALSE(chain.max_tps_exceeded);
    REQUIRE(chain.tp_dirs.size() == 82);
    REQUIRE(chain.cum_primary_tps == 0);
    REQUIRE(chain.ulp_diffs.back().PtWasBehindPlane());
}

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
    TeleportChain chain;

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
        bool closeToZero = false;
        for (int i = 0; i < 2 && !closeToZero; i++)
            for (int j = 0; j < 3 && closeToZero; j++)
                if (fabsf((i == 0 ? blue.pos : orange.pos)[j]) < 0.5f)
                    closeToZero = true;
        if (closeToZero)
            continue; // don't use portals where the nudges might take a really long time

        PortalPair pp{blue, orange};

        pp.CalcTpMatrices(PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION);
        Entity player{blue.pos};
        EntityInfo ent_info{
            .n_ent_children = N_CHILDREN_PLAYER_WITH_PORTAL_GUN,
            .origin_inbounds = false,
        };
        chain.Generate(pp, true, player, ent_info, 3);
        // only handle basic teleports and simple VAGs for now
        if (chain.max_tps_exceeded || (chain.cum_primary_tps != 1 && chain.cum_primary_tps != -1))
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
        */

        // clang-format off
        Entity tmp_player{blue.pos + blue.f};
        auto& bp = pp.blue.pos; auto& op = pp.orange.pos;
        auto& ba = pp.blue.ang; auto& oa = pp.orange.ang;
        conn.SendCmd(
            "ent_fire orange newlocation \\\"%.9g %.9g %.9g %.9g %.9g %.9g\\\"; "
            "ent_fire blue   newlocation \\\"%.9g %.9g %.9g %.9g %.9g %.9g\\\"; "
            "setpos %.9g %.9g %.9g",
            op.x, op.y, op.z, oa.x, oa.y, oa.z,
            bp.x, bp.y, bp.z, ba.x, ba.y, ba.z,
            tmp_player.origin.x, tmp_player.origin.y, tmp_player.origin.z
        );
        // clang-format on
        conn.RecvAck();
        Vector boundary_setpos = chain.pre_teleported_ent.origin;
        conn.SendCmd("setpos %.9g %.9g %.9g", boundary_setpos.x, boundary_setpos.y, boundary_setpos.z);
        conn.RecvAck();

        // timescale 1: sleep for 350ms, timescale 20: sleep for 10ms
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        conn.SendCmd("spt_ipc_properties 0 m_vecOrigin");
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
        INFO("expected " << (chain.cum_primary_tps == CUM_TP_VAG ? "VAG" : "normal teleport"));
        Entity expected_player_pos = chain.transformed_ent;
        REQUIRE(actual_player_pos.DistToSqr(expected_player_pos.origin) < 100 * 100);
        printf("iteration %d: %s", iteration, chain.cum_primary_tps == CUM_TP_VAG ? "VAG\n" : "Normal teleport\n");
        iteration++;
    }
}
