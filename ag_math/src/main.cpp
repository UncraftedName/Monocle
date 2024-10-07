#include <iostream>
#include "source_math.hpp"

int main()
{
    SyncFloatingPointControlWord();
    PortalPair pp{{100, -100, 10}, {40, 50, 60}, {300, -300, 40}, {-30, -20, -10}};
    Vector pt = pp.p1.pos;
    pt -= pp.p1.f;
    printf("p1 plane: ");
    pp.p1.plane.print();
    printf("\nshould teleport: %d\n", pp.p1.ShouldTeleport(pt, false));
    Vector new_pt = pp.TeleportNonPlayerEntity(pt, true);
    printf("teleported from ");
    pt.print();
    printf(" to ");
    new_pt.print();
    printf("\nwhich should be close to ");
    (pp.p2.pos + pp.p2.f).print();
}
