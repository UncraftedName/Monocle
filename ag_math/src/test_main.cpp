#include "../../catch/src/catch_amalgamated.hpp"
#include "source_math.hpp"

#define CATCH_SEED ((uint32_t)42069)

int main(int argc, char* argv[])
{
    Catch::Session session;
    session.libIdentify();
    session.configData().rngSeed = CATCH_SEED;
    return session.run(argc, argv);
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
