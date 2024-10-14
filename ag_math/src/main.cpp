#include <iostream>
#include <algorithm>

#include "source_math.hpp"
#include "vag_logic.hpp"
#include "prng.hpp"

#include <vector>
#include <matplot/matplot.h>

int main()
{
    SyncFloatingPointControlWord();

    small_prng rng{7};
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
    };

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
