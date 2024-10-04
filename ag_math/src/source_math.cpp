#include <cstring>
#include "source_math.hpp"

extern "C" void __cdecl AngleMatrix(const QAngle* angles, matrix3x4_t* matrix);
extern "C" void __cdecl AngleVectors(const QAngle* angles, Vector* f, Vector* r, Vector* u);
extern "C" void __cdecl MatrixInverseTR(const VMatrix* src, VMatrix* dst);
// TODO unused
extern "C" void __cdecl Vector3DMultiply(const VMatrix* src1, const Vector* src2, Vector* dst);
extern "C" void __cdecl VMatrix__MatrixMul(const VMatrix* lhs, const VMatrix* rhs, VMatrix* out);
extern "C" Vector* __cdecl VMatrix__operatorVec(const VMatrix* lhs, Vector* out, const Vector* vVec);
extern "C" void __cdecl Portal_CalcPlane(const Vector* portal_pos, const Vector* portal_f, VPlane* out_plane);
extern "C" bool __cdecl Portal_EntBehindPlane(const VPlane* portal_plane, const Vector* ent_center);

static void AngleMatrix(const QAngle* angles, const Vector* position, matrix3x4_t* matrix)
{
    AngleMatrix(angles, matrix);
    (*matrix)[0][3] = position->x;
    (*matrix)[1][3] = position->y;
    (*matrix)[2][3] = position->z;
}

static void MatrixSetIdentity(VMatrix& dst)
{
    // clang-format off
    dst[0][0] = 1.0f; dst[0][1] = 0.0f; dst[0][2] = 0.0f; dst[0][3] = 0.0f;
    dst[1][0] = 0.0f; dst[1][1] = 1.0f; dst[1][2] = 0.0f; dst[1][3] = 0.0f;
    dst[2][0] = 0.0f; dst[2][1] = 0.0f; dst[2][2] = 1.0f; dst[2][3] = 0.0f;
    dst[3][0] = 0.0f; dst[3][1] = 0.0f; dst[3][2] = 0.0f; dst[3][3] = 1.0f;
    // clang-format on
}

Portal::Portal(const Vector& v, const QAngle& q) : pos{v}, ang{q}
{
    AngleVectors(&ang, &f, &r, &u);
    Portal_CalcPlane(&pos, &f, &plane);
    AngleMatrix(&ang, &pos, &mat);
}

bool Portal::ShouldTeleport(const Vector& ent_center, bool check_portal_hole) const
{
    assert(!check_portal_hole);
    return Portal_EntBehindPlane(&plane, &ent_center);
}

PortalPair::PortalPair(const Portal& p1, const Portal& p2) : p1{p1}, p2{p2}
{
    // CProp_Portal_Shared::UpdatePortalTransformationMatrix
    VMatrix matPortal1ToWorldInv, matPortal2ToWorld, matRotation;
    MatrixInverseTR(reinterpret_cast<const VMatrix*>(&p1.mat), &matPortal1ToWorldInv);
    MatrixSetIdentity(matRotation);
    matRotation[0][0] = -1.0f;
    matRotation[1][1] = -1.0f;
    memcpy(&matPortal2ToWorld, &p2.mat, sizeof matrix3x4_t);
    matPortal2ToWorld[3][0] = matPortal2ToWorld[3][1] = matPortal2ToWorld[3][2] = 0.0f;
    matPortal2ToWorld[3][3] = 1.0f;
    VMatrix__MatrixMul(&matPortal2ToWorld, &matRotation, &p2_to_p1);
    VMatrix__MatrixMul(&p2_to_p1, &matPortal1ToWorldInv, &p1_to_p2);

    // the bit right after in CProp_Portal::UpdatePortalTeleportMatrix
    MatrixInverseTR(&p1_to_p2, &p2_to_p1);

    /*
    * TODO: is this really the correct flow control path? Maybe the flow goes through
    * CPortalSimulator::UpdateLinkMatrix instead?
    */
}

Vector PortalPair::TeleportNonPlayerEntity(const Vector& pt, bool tp_with_p1) const
{
    Vector v;
    VMatrix__operatorVec(tp_with_p1 ? &p1_to_p2 : &p2_to_p1, &v, &pt);
    return v;
}
