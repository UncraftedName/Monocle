#include <chrono>

#include "../../catch/src/catch_amalgamated.hpp"
#include "source_math.hpp"
#include "vag_logic.hpp"
#include "prng.hpp"

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
        constexpr float p_min = -8000.f, p_max = 8000.f, a_min = -180.f, a_max = 180.f;
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

    PortalPair pp{p1, p2, (PlacementOrder)order};

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
        Vector d = new_pt - (p1_teleporting ? pt2 : pt1);
        REQUIRE_THAT(d.x * d.x + d.y * d.y + d.z * d.z, Catch::Matchers::WithinAbs(0.0, 0.01));
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
            IntVector ulp_diff{};
            NudgePointTowardsPortalPlane(pt, p, &nudged_to, &ulp_diff, b_nudge_behind);
            REQUIRE((bool)b_nudge_behind == p.ShouldTeleport(nudged_to, false));
            int ulp_sign = ulp_diff[0] ? ulp_diff[0] : (ulp_diff[1] ? ulp_diff[1] : ulp_diff[2]);
            if (b_nudge_behind) {
                REQUIRE(p.ShouldTeleport(pt, false) == (ulp_sign >= 0));
            } else {
                REQUIRE(p.ShouldTeleport(pt, false) == (ulp_sign > 0));
            }

            if (ulp_diff[0] || ulp_diff[1] || ulp_diff[2]) {
                // now nudge across the portal boundary (requires an ulp diff from the previous step)
                for (int i = 0; i < 3; i++) {
                    if (ulp_diff[i]) {
                        float target = nudged_to[i] + p.plane.n[i] * (b_nudge_behind ? 1 : -1);
                        nudged_to[i] = std::nextafterf(nudged_to[i], target);
                        break;
                    }
                }
                REQUIRE_FALSE((bool)b_nudge_behind == p.ShouldTeleport(nudged_to, false));
            }
        }
    }
}

TEST_CASE("Teleport with VAG")
{
    // chamber 09 - blue portal on opposite wall, bottom left corner
    PortalPair pp{
        Vector{255.96875f, -161.01294f, 54.031242f},
        QAngle{-0.f, 180.f, 0.f},
        Vector{-127.96875f, -191.24300f, 182.03125f},
        QAngle{0.f, 0.f, 0.f},
        PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED,
    };
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
        PlacementOrder::ORANGE_WAS_CLOSED_BLUE_MOVED,
    };
    TpInfo info;
    TryVag(pp, {-127.96876f, -191.24300f, 168.f}, false, info);
    REQUIRE(info.result == TpResult::Nothing);
}

// TODO: add a test for checking exact matrices/plane/vectors

/*
* setpos -127.96875385 -191.24299622 150
* 
* VAG:
SPT: There's an open orange portal with index 46 at -127.96875000 -191.21875000 182.03125000.
SPT: There's an open blue portal with index 164 at 255.96875000 -161.00000000 54.00000000.


NOTHING:

SPT: There's an open orange portal with index 46 at -127.96875000 -191.21875000 182.03125000.
SPT: There's an open blue portal with index 164 at 255.96875000 -223.96875000 54.00000000.

RECURSIVE:


SPT: There's an open orange portal with index 46 at -127.96875000 -191.21875000 182.03125000.
SPT: There's an open blue portal with index 164 at 255.96875000 -161.00000000 201.96875000.


ORANGE ANGLES: 0, 0, 0 (in theory)
BLUE ANGLES: 0, 180.022, 0 (lol)
*/
