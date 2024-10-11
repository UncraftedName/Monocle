#include <iostream>

#include "source_math.hpp"
#include "vag_logic.hpp"
#include "prng.hpp"

int main()
{
    SyncFloatingPointControlWord();

    {
        PortalPair pp{{100, -100, 10}, {40, 50, 60}, {300, -300, 40}, {-30, -20, -10}};
        pp.CalcTpMatrices(PlacementOrder::AFTER_LOAD);
        Vector pt = pp.blue.pos;
        pt -= pp.blue.f;
        printf("blue plane: ");
        pp.blue.plane.print();
        Entity ent{pt - pp.blue.f, 1.f};
        printf("\nshould teleport: %d\n", pp.blue.ShouldTeleport(ent, true));
        printf("teleported from ");
        ent.origin.print();
        pp.Teleport(ent, true);
        printf(" to ");
        ent.origin.print();
        printf("\nwhich should be close to ");
        (pp.orange.pos + pp.orange.f).print();
    }

    small_prng rng{1};
    TpChain chain;
    for (int i = 0; i < 100000; i++) {
        constexpr float p_min = -800.f, p_max = 800.f, a_min = -180.f, a_max = 180.f;
        PortalPair pp{
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
        };

        pp.CalcTpMatrices(PlacementOrder::AFTER_LOAD);
        Entity ent{pp.blue.pos, 1.f};
        GenerateTeleportChain(pp, ent, true, chain, 3);
        if (chain.max_tps_exceeded || chain.cum_primary_tps != -1)
            continue;

        pp.CalcTpMatrices(PlacementOrder::BLUE_OPEN_ORANGE_NEW_LOCATION);
        ent.origin = pp.blue.pos;
        GenerateTeleportChain(pp, ent, true, chain, 1);
        if (chain.max_tps_exceeded)
            continue;

        printf("\n\nFound VAG that's only possible after a saveload (iteration %d):\n", i + 1);
        pp.print_newlocation_cmd();
        printf("setpos ");
        (chain.pts[0] - Vector{0, 0, 18.f}).print();
        break;
    };

    /*int buckets[(int)TpResult::COUNT]{};
    int n_portals = 100000;
    for (int i = 0; i < n_portals; i++) {
        constexpr float p_min = -800.f, p_max = 800.f, a_min = -180.f, a_max = 180.f;
        PortalPair pp{
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
        };
        TpInfo info;
        pp.CalcTpMatrices(PlacementOrder::BLUE_OPEN_ORANGE_NEW_LOCATION);
        TryVag(pp, pp.blue.pos, true, info);
        buckets[(int)info.result]++;
    }
    printf("\n\n%d random pairs results: [%d, %d, %d]", n_portals, buckets[0], buckets[1], buckets[2]);*/
}
