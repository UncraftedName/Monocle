#include <iostream>
#include "source_math.hpp"

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
}
