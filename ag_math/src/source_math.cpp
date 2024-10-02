#include <cstring>
#include "source_math.hpp"

void AngleMatrix(const QAngle* angles, const Vector* position, matrix3x4_t* matrix)
{
    AngleMatrix(angles, matrix);
    (*matrix)[0][3] = position->x;
    (*matrix)[1][3] = position->y;
    (*matrix)[2][3] = position->z;
}

void MatrixSetIdentity(VMatrix& dst)
{
    // clang-format off
    dst[0][0] = 1.0f; dst[0][1] = 0.0f; dst[0][2] = 0.0f; dst[0][3] = 0.0f;
    dst[1][0] = 0.0f; dst[1][1] = 1.0f; dst[1][2] = 0.0f; dst[1][3] = 0.0f;
    dst[2][0] = 0.0f; dst[2][1] = 0.0f; dst[2][2] = 1.0f; dst[2][3] = 0.0f;
    dst[3][0] = 0.0f; dst[3][1] = 0.0f; dst[3][2] = 0.0f; dst[3][3] = 1.0f;
    // clang-format on
}

void UpdatePortalTransformationMatrix(const matrix3x4_t* localToWorld,
                                      const matrix3x4_t* remoteToWorld,
                                      VMatrix* pMatrix)
{
    VMatrix matPortal1ToWorldInv, matPortal2ToWorld, matRotation;
    MatrixInverseTR(reinterpret_cast<const VMatrix*>(localToWorld), &matPortal1ToWorldInv);
    MatrixSetIdentity(matRotation);
    matRotation[0][0] = -1.0f;
    matRotation[1][1] = -1.0f;
    memcpy(&matPortal2ToWorld, remoteToWorld, sizeof matrix3x4_t);
    matPortal2ToWorld[3][0] = 0.0f;
    matPortal2ToWorld[3][1] = 0.0f;
    matPortal2ToWorld[3][2] = 0.0f;
    matPortal2ToWorld[3][3] = 1.0f;
    VMatrix tmp;
    VMatrix__MatrixMul(&matPortal2ToWorld, &matRotation, &tmp);
    VMatrix__MatrixMul(&tmp, &matPortal1ToWorldInv, pMatrix);
}

void Portal::CalcMatrix(matrix3x4_t& out) const
{
    AngleMatrix(&ang, &pos, &out);
}

void Portal::CalcVectors(Vector* f, Vector* r, Vector* u) const
{
    AngleVectors(&ang, f, r, u);
}

void Portal::CalcPlane(const Vector& f, VPlane& out_plane) const
{
    Portal_CalcPlane(&pos, &f, &out_plane);
}

bool Portal::ShouldTeleport(const VPlane& portal_plane, const Vector& ent_center, bool check_portal_hole) const
{
    assert(!check_portal_hole);
    return Portal_EntBehindPlane(&portal_plane, &ent_center);
}

Vector Portal::TeleportNonPlayerEntity(const VMatrix& mat, const Vector& pt) const
{
    Vector v;
    VMatrix__operatorVec(&mat, &v, &pt);
    return v;
}

void PortalPair::CalcTeleportMatrix(const matrix3x4_t& p1_mat, const matrix3x4_t& p2_mat, VMatrix& out, bool p1_to_p2)
{
    if (p1_to_p2) {
        UpdatePortalTransformationMatrix(&p1_mat, &p2_mat, &out);
    } else {
        VMatrix p1_to_p2_mat;
        UpdatePortalTransformationMatrix(&p1_mat, &p2_mat, &p1_to_p2_mat);
        MatrixInverseTR(&p1_to_p2_mat, &out);
    }
}
