#include <iostream>
#include "source_math.hpp"

int main()
{
    PortalPair pp{{100, -100, 10}, {40, 50, 60}, {300, -300, 40}, {-30, -20, -10}};
    matrix3x4_t p1_mat, p2_mat;
    pp.p1.CalcMatrix(&p1_mat);
    pp.p2.CalcMatrix(&p2_mat);
    Vector p1_f;
    pp.p1.CalcVectors(&p1_f, nullptr, nullptr);
    Vector pt = pp.p1.pos;
    pt -= p1_f;
    VPlane p1_plane;
    pp.p1.CalcPlane(&p1_f, &p1_plane);
    printf("p1 plane: ");
    p1_plane.print();
    bool should_tp = pp.p1.ShouldTeleport(&p1_plane, &pt, false);
    printf("\nshould teleport: %d\n", should_tp);
    VMatrix p1_to_p2;
    pp.CalcTeleportMatrix(&p1_mat, &p2_mat, &p1_to_p2, true);
    Vector new_pt;
    VMatrix__operatorVec(&p1_to_p2, &new_pt, &pt);
    printf("teleported from ");
    pt.print();
    printf(" to ");
    new_pt.print();
    printf("\nwhich should be close to ");
    Vector p2_f;
    pp.p2.CalcVectors(&p2_f, nullptr, nullptr);
    (pp.p2.pos + p2_f).print();
}
