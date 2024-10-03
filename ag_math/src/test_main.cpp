#include <chrono>

#include "../../catch/src/catch_amalgamated.hpp"
#include "source_math.hpp"
#include "vag_logic.hpp"

#define CATCH_SEED ((uint32_t)42069)

int main(int argc, char* argv[])
{
    Catch::Session session;
    session.libIdentify();
    session.configData().rngSeed = CATCH_SEED;

    using namespace std::chrono;
    auto t = high_resolution_clock::now();
    int ret = session.run(argc, argv);
    if (ret == 0) {
        auto td = duration_cast<duration<double>>(high_resolution_clock::now() - t);
        printf("Tests completed after %.3fs\n", td.count());
    }
}

static Catch::Generators::RandomFloatingGenerator<float> posGen{-8000.f, 8000.f, CATCH_SEED};
static Catch::Generators::RandomFloatingGenerator<float> angGen{-180.f, 180.f, CATCH_SEED};

#define GEN_AND_NEXT(gen) (gen.next(), gen.get())

class PortalGenerator : public Catch::Generators::IGenerator<Portal> {

    Portal cur;

public:
    PortalGenerator() : cur{{}, {}}
    {
        static_cast<void>(next());
    }

    Portal const& get() const override
    {
        return cur;
    }

    bool next() override
    {
        cur = Portal{
            Vector{GEN_AND_NEXT(posGen), GEN_AND_NEXT(posGen), GEN_AND_NEXT(posGen)},
            QAngle{GEN_AND_NEXT(angGen), GEN_AND_NEXT(angGen), GEN_AND_NEXT(angGen)},
        };
        return true;
    }

    static Catch::Generators::GeneratorWrapper<Portal> make()
    {
        return Catch::Generators::GeneratorWrapper<Portal>(Catch::Detail::make_unique<PortalGenerator>());
    }
};

TEST_CASE("ShouldTeleport behaves as expected")
{
    auto p = GENERATE(take(10000, PortalGenerator::make()));
    Vector f;
    VPlane plane;
    p.CalcVectors(&f, nullptr, nullptr);
    p.CalcPlane(f, plane);
    REQUIRE(p.ShouldTeleport(plane, p.pos - f, false));
    REQUIRE_FALSE(p.ShouldTeleport(plane, p.pos + f, false));
}

TEST_CASE("Teleport transform behaves as expected")
{
    auto p1 = GENERATE(take(100, PortalGenerator::make()));
    auto p2 = GENERATE(take(100, PortalGenerator::make()));
    auto p1_teleporting = (bool)GENERATE(0, 1);

    PortalPair pp{p1, p2};

    matrix3x4_t p1m, p2m;
    p1.CalcMatrix(p1m);
    p2.CalcMatrix(p2m);
    Vector p1f, p1r, p1u, p2f, p2r, p2u;
    p1.CalcVectors(&p1f, &p1r, &p1u);
    p2.CalcVectors(&p2f, &p2r, &p2u);
    VMatrix tp_mat;
    pp.CalcTeleportMatrix(p1m, p2m, tp_mat, p1_teleporting);

    for (int translate_mask = 0; translate_mask < 16; translate_mask++) {
        Vector off1{0.f, 0.f, 0.f}, off2{0.f, 0.f, 0.f};
        if (translate_mask & 1) {
            off1 += p1f;
            off2 -= p2f;
        }
        if (translate_mask & 2) {
            off1 += p1r;
            off2 -= p2r;
        }
        if (translate_mask & 4) {
            off1 += p1u;
            off2 += p2u;
        }
        bool sign = translate_mask & 8;
        Vector pt1 = p1.pos + (sign ? off1 : -off1);
        Vector pt2 = p2.pos + (sign ? off2 : -off2);

        Vector new_pt = p1.TeleportNonPlayerEntity(tp_mat, p1_teleporting ? pt1 : pt2);
        Vector d = new_pt - (p1_teleporting ? pt2 : pt1);
        REQUIRE_THAT(d.x * d.x + d.y * d.y + d.z * d.z, Catch::Matchers::WithinAbs(0.0, 0.01));
    }
}

TEST_CASE("Nudging point towards portal plane")
{
    auto p = GENERATE(take(1000, PortalGenerator::make()));

    Vector f, r, u;
    p.CalcVectors(&f, &r, &u);
    VPlane plane;
    p.CalcPlane(f, plane);

    for (int translate_mask = 0; translate_mask < 8; translate_mask++) {
        Vector off{0.f, 0.f, 0.f};
        if (translate_mask & 1)
            off += r;
        if (translate_mask & 2)
            off += u;
        Vector pt = p.pos + ((translate_mask & 4) ? off : -off);
        // add a little "random" nudge
        pt[0] += pt[0] / 100000.f;

        for (int b_nudge_behind = 0; b_nudge_behind < 2; b_nudge_behind++) {
            Vector nudged_to;
            IntVector ulp_diff{};
            NudgePointTowardsPortalPlane(pt, plane, &nudged_to, &ulp_diff, b_nudge_behind);
            REQUIRE((bool)b_nudge_behind == p.ShouldTeleport(plane, nudged_to, false));

            if (ulp_diff[0] || ulp_diff[1] || ulp_diff[2]) {
                // now nudge across the portal boundary (requires an ulp diff from the previous step)
                for (int i = 0; i < 3; i++) {
                    if (ulp_diff[i]) {
                        float target = nudged_to[i] + plane.n[i] * (b_nudge_behind ? 1 : -1);
                        nudged_to[i] = std::nextafterf(nudged_to[i], target);
                        break;
                    }
                }
                REQUIRE_FALSE((bool)b_nudge_behind == p.ShouldTeleport(plane, nudged_to, false));
            }
        }
    }
}

TEST_CASE("Teleport with no VAG")
{
    // chamber 09 - blue portal on opposite wall, bottom right corner
    // blue portal is portal 1
    PortalPairCache cache{
        Vector{255.96875f, -161.01294f, 54.031242f},
        QAngle{-0.f, 180.f, 0.f},
        Vector{-127.96875f, -191.24300f, 182.03125f},
        QAngle{0.f, 0.f, 0.f},
    };
    TpInfo info;
    TryVag(cache, cache.p2.p.pos, false, info);
    // TODO implement the portal hole check, this should specifically return TpResult::VAG
    REQUIRE_FALSE(info.result == TpResult::Nothing);
}

TEST_CASE("Teleport with VAG")
{
    // chamber 09 - blue portal on opposite wall, bottom left corner
    // blue portal is portal 1
    PortalPairCache cache{
        Vector{255.96875f, -223.96875f, 54.031242f},
        QAngle{-0.f, 180.f, 0.f},
        Vector{-127.96875f, -191.24300f, 182.03125f},
        QAngle{0.f, 0.f, 0.f},
    };
    TpInfo info;
    TryVag(cache, cache.p2.p.pos, false, info);
    REQUIRE(info.result == TpResult::Nothing);
}

// TODO: set fp rounding mode and control word, get rid of the portal cache cringe

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
