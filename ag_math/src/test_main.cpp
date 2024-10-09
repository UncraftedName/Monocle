#include "../../catch/src/catch_amalgamated.hpp"
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

class PortalGenerator : public Catch::Generators::IGenerator<Portal> {

    small_prng& rng;
    Portal cur;

public:
    PortalGenerator(small_prng& rng) : cur{{}, {}}, rng{rng}
    {
        static_cast<void>(next());
    }

    Portal const& get() const override
    {
        return cur;
    }

    bool next() override
    {
        constexpr float p_min = -3000.f, p_max = 3000.f, a_min = -180.f, a_max = 180.f;
        cur = Portal{
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
        };
        return true;
    }

    static Catch::Generators::GeneratorWrapper<Portal> make(small_prng& rng)
    {
        return Catch::Generators::GeneratorWrapper<Portal>(Catch::Detail::make_unique<PortalGenerator>(rng));
    }
};

TEST_CASE("ShouldTeleport")
{
    static small_prng rng;
    auto p = GENERATE(take(10000, PortalGenerator::make(rng)));
    REQUIRE(p.ShouldTeleport(p.pos - p.f, false));
    REQUIRE_FALSE(p.ShouldTeleport(p.pos + p.f, false));
}

TEST_CASE("TeleportNonPlayerEntity")
{
    static small_prng rng;
    auto p1 = GENERATE(take(100, PortalGenerator::make(rng)));
    auto p2 = GENERATE(take(100, PortalGenerator::make(rng)));
    auto order = GENERATE_COPY(Catch::Generators::range(0, (int)PlacementOrder::COUNT));
    auto p1_teleporting = (bool)GENERATE(0, 1);

    PortalPair pp{p1, p2};
    pp.CalcTpMatrices((PlacementOrder)order);

    for (int translate_mask = 0; translate_mask < 16; translate_mask++) {
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

        Vector new_pt = pp.TeleportNonPlayerEntity(p1_teleporting ? pt1 : pt2, p1_teleporting);
        REQUIRE_THAT(new_pt.DistToSqr(p1_teleporting ? pt2 : pt1), Catch::Matchers::WithinAbs(0.0, 0.01));
    }
}

TEST_CASE("Nudging point towards portal plane")
{
    static small_prng rng;
    auto p = GENERATE(take(1000, PortalGenerator::make(rng)));

    for (int translate_mask = 0; translate_mask < 8; translate_mask++) {
        Vector off{0.f, 0.f, 0.f};
        if (translate_mask & 1)
            off += p.r;
        if (translate_mask & 2)
            off += p.u;
        Vector pt = p.pos + ((translate_mask & 4) ? off : -off);
        // add a little "random" nudge
        pt[0] += pt[0] / 100000.f;

        for (int b_nudge_behind = 0; b_nudge_behind < 2; b_nudge_behind++) {
            Vector nudged_to;
            VecUlpDiff ulp_diff;
            NudgePointTowardsPortalPlane(pt, p, b_nudge_behind, &nudged_to, &ulp_diff);
            REQUIRE((bool)b_nudge_behind == p.ShouldTeleport(nudged_to, false));
            if (b_nudge_behind) {
                REQUIRE(p.ShouldTeleport(pt, false) == (ulp_diff.diff >= 0));
            } else {
                REQUIRE(p.ShouldTeleport(pt, false) == (ulp_diff.diff > 0));
            }

            if (ulp_diff.ax != -1) {
                // now nudge across the portal boundary (requires an ulp diff from the previous step)
                float target = nudged_to[ulp_diff.ax] + p.plane.n[ulp_diff.ax] * (b_nudge_behind ? 1 : -1);
                nudged_to[ulp_diff.ax] = std::nextafterf(nudged_to[ulp_diff.ax], target);
                REQUIRE_FALSE((bool)b_nudge_behind == p.ShouldTeleport(nudged_to, false));
            }
        }
    }
}

// setpos -127.96875385 -191.24299622 150

TEST_CASE("Teleport with VAG")
{
    // chamber 09 - blue portal on opposite wall, bottom left corner
    PortalPair pp{
        Vector{255.96875f, -161.01294f, 54.031242f},
        QAngle{-0.f, 180.f, 0.f},
        Vector{-127.96875f, -191.24300f, 182.03125f},
        QAngle{0.f, 0.f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED);
    TpInfo info;
    TryVag(pp, {-127.96876f, -191.24300f, 168.f}, false, info);
    // TODO implement the portal hole check, this should specifically return TpResult::VAG
    REQUIRE_FALSE(info.result == TpResult::Nothing);
}

/*
* HAHA this is cursed... This placement in 09 is actually 5 teleports: 21122=2
*/

TEST_CASE("Teleport with no VAG")
{
    // chamber 09 - blue portal on opposite wall, bottom right corner
    PortalPair pp{
        Vector{255.96875f, -223.96875f, 54.031242f},
        QAngle{-0.f, 180.f, 0.f},
        Vector{-127.96875f, -191.24300f, 182.03125f},
        QAngle{0.f, 0.f, 0.f},
    };
    pp.CalcTpMatrices(PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED);
    TpInfo info;
    TryVag(pp, {-127.96876f, -191.24300f, 168.f}, false, info);
    REQUIRE(info.result == TpResult::Nothing);
}

// TODO: add a test for checking exact matrices/plane/vectors

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
            server_addr.sin_port = htons(27182);

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

        void NextRecvMsg()
        {
            if (off < recvLen) {
                off += strnlen(buf + off, recvLen - off) + 1;
            } else {
                off = 0;
                recvLen = recv(sock, buf, sizeof buf, 0);
                if (recvLen == SOCKET_ERROR)
                    SKIP("recv failed (" << recvLen << ")");
            }
        }

        void RecvAck()
        {
            NextRecvMsg();
            REQUIRE_FALSE(strcmp(buf + off, "{\"type\":\"ack\"}"));
            NextRecvMsg();
        }

        ~_TCPConnection()
        {
            if (_cleanup_sock)
                closesocket(sock);
        }
    } conn{};

    conn.SendCmd("spt_prevent_vag_crash 1; spt_focus_nosleep 1; spt_noclip_noslowfly 1");
    conn.RecvAck();

    small_prng rng;
    PortalGenerator pg{rng};
    TpInfo tp_info;

    int iteration = 0;
    while (iteration < 1000) {
        Portal blue = (pg.next(), pg.get());
        Portal orange = (pg.next(), pg.get());
        /*
        * Don't use portals that are too close to each other - this can cause false positives with
        * the distance threshold to the teleport destination.
        */
        if (blue.pos.DistToSqr(orange.pos) < 500 * 500)
            continue;
        /*
        * Don't use portals that point roughly in the same direction - this can sometimes cause
        * recursive teleports because the VAG destination is in one of the portal holes. I haven't
        * implemented this yet - I'm only doing regular teleports & simple VAGs here.
        */
        if (abs(blue.f.x * orange.f.x + blue.f.y * orange.f.y + blue.f.z * orange.f.z) > 0.8)
            continue;

        PortalPair pp{blue, orange};

        pp.CalcTpMatrices(PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION);
        TryVag(pp, pp.blue.pos, true, tp_info);
        // only handle basic teleports and simple VAGs for now
        if (tp_info.result != TpResult::Nothing && tp_info.result != TpResult::VAG)
            continue;

        /*
        * 1) I have an entity center
        * 2) I offset that by {0, 0, -18} to get the player origin
        * 3) I use setpos and the game adds {0, 0, 18} to the origin to get the center again
        * 
        * But '(a - b) + b == a' for b=18 is not always true :). This is because the subtraction
        * may increase the exponent and drop the least significant mantissa bits.
        */
        int u = (int)fabs(tp_info.tp_from.z);
        unsigned long idx1, idx2;
        _BitScanReverse(&idx1, u);
        _BitScanReverse(&idx2, u + 18);
        if (u == 0 || idx1 != idx2)
            continue;

        // clang-format off
        auto& bp = pp.blue.pos; auto& op = pp.orange.pos;
        auto& ba = pp.blue.ang; auto& oa = pp.orange.ang;
        Vector sp = tp_info.tp_from - Vector{0, 0, 18.f};
        conn.SendCmd(
            "ent_fire orange newlocation \\\"%.9g %.9g %.9g %.9g %.9g %.9g\\\"; "
            "ent_fire blue   newlocation \\\"%.9g %.9g %.9g %.9g %.9g %.9g\\\"; "
            "setpos %.9g %.9g %.9g",
            op.x, op.y, op.z, oa.x, oa.y, oa.z,
            bp.x, bp.y, bp.z, ba.x, ba.y, ba.z,
            sp.x, sp.y, sp.z
        );
        conn.RecvAck();
        // clang-format on

        // timescale 1: sleep for 350ms, timescale 20: sleep for 1ms lol
        // std::this_thread::sleep_for(std::chrono::milliseconds(350));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        conn.SendCmd("spt_ipc_properties 0 m_vecOrigin");
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); // everything breaks without this :)
        conn.RecvAck();

        Vector player_pos{};
        int n_args = _snscanf_s(conn.buf + conn.off,
                                conn.recvLen - conn.off,
                                "{\"entity\":{"
                                "\"m_vecOrigin[0]\":%f,\"m_vecOrigin[1]\":%f,\"m_vecOrigin[2]\":%f"
                                "},\"exists\":true,\"type\":\"ent\"}",
                                &player_pos.x,
                                &player_pos.y,
                                &player_pos.z);
        REQUIRE(n_args == 3);

        INFO("iteration " << iteration);
        if (tp_info.result == TpResult::Nothing)
            REQUIRE(player_pos.DistToSqr(pp.orange.pos) < 100 * 100);
        else
            REQUIRE(player_pos.DistToSqr(tp_info.tp_to_final) < 100 * 100);
        printf("iteration %d: %s", iteration, (bool)tp_info.result ? "Normal teleport\n" : "VAG\n");
        iteration++;
    }
}
