#include <cstring>
#include <cmath>

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

void PortalPair::CalcTpMatrices(PlacementOrder order)
{
    switch (order) {
        case PlacementOrder::_BLUE_UPTM:
        case PlacementOrder::_ORANGE_UPTM: {
            bool ob = order == PlacementOrder::_BLUE_UPTM;
            auto& p1_mat = ob ? blue.mat : orange.mat;
            auto& p2_mat = ob ? orange.mat : blue.mat;
            auto& p1_to_p2 = ob ? b_to_o : o_to_b;
            auto& p2_to_p1 = ob ? o_to_b : b_to_o;

            // CProp_Portal_Shared::UpdatePortalTransformationMatrix
            VMatrix matPortal1ToWorldInv, matPortal2ToWorld, matRotation;
            MatrixInverseTR(reinterpret_cast<const VMatrix*>(&p1_mat), &matPortal1ToWorldInv);
            MatrixSetIdentity(matRotation);
            matRotation[0][0] = -1.0f;
            matRotation[1][1] = -1.0f;
            memcpy(&matPortal2ToWorld, &p2_mat, sizeof matrix3x4_t);
            matPortal2ToWorld[3][0] = matPortal2ToWorld[3][1] = matPortal2ToWorld[3][2] = 0.0f;
            matPortal2ToWorld[3][3] = 1.0f;
            VMatrix__MatrixMul(&matPortal2ToWorld, &matRotation, &p2_to_p1);
            VMatrix__MatrixMul(&p2_to_p1, &matPortal1ToWorldInv, &p1_to_p2);
            // the bit right after in CProp_Portal::UpdatePortalTeleportMatrix
            MatrixInverseTR(&p1_to_p2, &p2_to_p1);
            break;
        }
        case PlacementOrder::_ULM: {
            // CPortalSimulator::UpdateLinkMatrix for both portals
            VMatrix blue_to_world{blue.f, -blue.r, blue.u, blue.pos};
            VMatrix orange_to_world{orange.f, -orange.r, orange.u, orange.pos};
            for (int i = 0; i < 2; i++) {
                auto& p_to_world = i ? blue_to_world : orange_to_world;
                auto& other_to_world = i ? orange_to_world : blue_to_world;
                auto& p_to_other = i ? b_to_o : o_to_b;

                VMatrix matLocalToWorldInv, matRotation, tmp;
                MatrixInverseTR(&p_to_world, &matLocalToWorldInv);
                MatrixSetIdentity(matRotation);
                matRotation[0][0] = -1.0f;
                matRotation[1][1] = -1.0f;
                VMatrix__MatrixMul(&other_to_world, &matRotation, &tmp);
                VMatrix__MatrixMul(&tmp, &matLocalToWorldInv, &p_to_other);
            }
            break;
        }
        default:
            assert(0);
    }
}

Vector PortalPair::TeleportNonPlayerEntity(const Vector& pt, bool tp_from_blue) const
{
    // you haven't called CalcTpMatrices yet!!!
    assert(!std::isnan(b_to_o.m[3][3]) && !std::isnan(o_to_b.m[3][3]));
    Vector v;
    VMatrix__operatorVec(tp_from_blue ? &b_to_o : &o_to_b, &v, &pt);
    return v;
}
