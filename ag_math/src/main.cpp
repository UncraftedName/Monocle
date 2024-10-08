#include <iostream>

#include "source_math.hpp"
#include "vag_logic.hpp"
#include "prng.hpp"

int main()
{
    SyncFloatingPointControlWord();
    PortalPair pp{{100, -100, 10}, {40, 50, 60}, {300, -300, 40}, {-30, -20, -10}, PlacementOrder::AFTER_LOAD};
    Vector pt = pp.blue.pos;
    pt -= pp.blue.f;
    printf("blue plane: ");
    pp.blue.plane.print();
    printf("\nshould teleport: %d\n", pp.blue.ShouldTeleport(pt, false));
    Vector new_pt = pp.TeleportNonPlayerEntity(pt, true);
    printf("teleported from ");
    pt.print();
    printf(" to ");
    new_pt.print();
    printf("\nwhich should be close to ");
    (pp.orange.pos + pp.orange.f).print();

    small_prng rng{1};
    for (int i = 0; i < 100000; i++) {
        constexpr float p_min = -800.f, p_max = 800.f, a_min = -180.f, a_max = 180.f;
        PortalPair pp{
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
            Vector{rng.next_float(p_min, p_max), rng.next_float(p_min, p_max), rng.next_float(p_min, p_max)},
            QAngle{rng.next_float(a_min, a_max), rng.next_float(a_min, a_max), rng.next_float(a_min, a_max)},
            PlacementOrder::ORANGE_OPEN_BLUE_NEW_LOCATION,
        };
        TpInfo info;
        TryVag(pp, pp.blue.pos, true, info);
        if (info.result == TpResult::VAG) {
            printf("\n\n\nFound random VAG (iteration %d):\n", i);
            pp.print_newlocation_cmd();
            printf("setpos ");
            (info.tp_from - Vector{0, 0, 18.f}).print();
            break;
        }
    };
}
