#include <cstring>
#include <cmath>

#include "source_math.hpp"

extern "C" {
void __cdecl AngleMatrix(const QAngle* angles, matrix3x4_t* matrix);
void __cdecl AngleVectors(const QAngle* angles, Vector* f, Vector* r, Vector* u);
void __cdecl MatrixInverseTR(const VMatrix* src, VMatrix* dst);
void __cdecl Vector3DMultiply(const VMatrix* src1, const Vector* src2, Vector* dst);
void __cdecl VMatrix__MatrixMul(const VMatrix* lhs, const VMatrix* rhs, VMatrix* out);
Vector* __cdecl VMatrix__operatorVec(const VMatrix* lhs, Vector* out, const Vector* vVec);
void __cdecl Portal_CalcPlane(const Vector* portal_pos, const Vector* portal_f, VPlane* out_plane);
bool __cdecl Portal_EntBehindPlane(const VPlane* portal_plane, const Vector* ent_center);
}

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
    plane = VPlane{f, (float)f.Dot(pos)};
    AngleMatrix(&ang, &pos, &mat);

    // CPortalSimulator::MoveTo
    // outward facing placnes: forward, backward, up, down, left, right
    hole_planes[0].n = f;
    hole_planes[0].d = plane.d - 0.5f;
    hole_planes[1].n = -f;
    hole_planes[1].d = -plane.d + PORTAL_HOLE_DEPTH;
    hole_planes[2].n = u;
    hole_planes[2].d = u.Dot(pos + u * (PORTAL_HALF_HEIGHT * .98f));
    hole_planes[3].n = -u;
    hole_planes[3].d = -u.Dot(pos - u * (PORTAL_HALF_HEIGHT * .98f));
    hole_planes[4].n = -r;
    hole_planes[4].d = -r.Dot(pos - r * (PORTAL_HALF_WIDTH * .98f));
    hole_planes[5].n = r;
    hole_planes[5].d = r.Dot(pos + r * (PORTAL_HALF_WIDTH * .98f));
    // SignbitsForPlane & setting the type; game uses .999f for the type, but I need tighter tolerances
    for (int i = 0; i < 6; i++) {
        hole_planes_bits[i].sign = 0;
        hole_planes_bits[i].type = 3;
        for (int j = 0; j < 3; j++) {
            if (hole_planes[i].n[j] < 0)
                hole_planes_bits[i].sign |= 1 << j;
            else if (fabsf(hole_planes[i].n[j]) >= 0.999999f)
                hole_planes_bits[i].type = j;
        }
    }
}

bool Portal::ShouldTeleport(const Entity& ent, bool check_portal_hole) const
{
    if (plane.n.Dot(ent.GetCenter()) >= plane.d)
        return false;
    if (!check_portal_hole)
        return true;
    /*
    * This portal hole check is an approximation and isn't meant to exactly replicate what the game
    * does. The game actually uses vphys to check if the entity or player collides with the portal
    * hole mesh, but I only care about high precision on the portal boundary. For portal hole
    * checks, I assume the entity is a crouched player or a ball.
    */
    if (ent.player) {
        Vector world_mins = ent.origin + PLAYER_CROUCH_MINS;
        Vector world_maxs = ent.origin + PLAYER_CROUCH_MAXS;
        for (int i = 0; i < 6; i++)
            if (BoxOnPlaneSide(world_mins, world_maxs, hole_planes[i], hole_planes_bits[i]) == PSR_FRONT)
                return false;
    } else {
        assert(("entities that are too small will fail portal hole check", ent.radius >= 1.f));
        for (int i = 0; i < 6; i++)
            if (BallOnPlaneSide(ent.origin, ent.radius, hole_planes[i]) == PSR_FRONT)
                return false;
    }
    return true;
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

void PortalPair::Teleport(Entity& ent, bool tp_from_blue) const
{
    // you haven't called CalcTpMatrices yet!!!
    assert(!std::isnan(b_to_o.m[3][3]) && !std::isnan(o_to_b.m[3][3]));
    Vector new_origin;
    Vector center = ent.GetCenter();
    VMatrix__operatorVec(tp_from_blue ? &b_to_o : &o_to_b, &new_origin, &center);
    if (ent.player)
        new_origin += ent.origin - center;
    ent.origin = new_origin;
}

Vector PortalPair::Teleport(const Vector& pt, bool tp_from_blue) const
{
    // you haven't called CalcTpMatrices yet!!!
    assert(!std::isnan(b_to_o.m[3][3]) && !std::isnan(o_to_b.m[3][3]));
    Vector v;
    VMatrix__operatorVec(tp_from_blue ? &b_to_o : &o_to_b, &v, &pt);
    return v;
}

int BoxOnPlaneSide(const Vector& mins, const Vector& maxs, const VPlane& p, plane_bits bits)
{
    // this optmization can be error-prone for slightly angled planes far away from the origin
    if (bits.type < 3) {
        if (p.d <= mins[bits.type])
            return PSR_FRONT;
        if (p.d >= maxs[bits.type])
            return PSR_BACK;
        return PSR_ON;
    }
    float d1, d2;
    switch (bits.sign) {
        case 0:
            d1 = p.n[0] * maxs[0] + p.n[1] * maxs[1] + p.n[2] * maxs[2];
            d2 = p.n[0] * mins[0] + p.n[1] * mins[1] + p.n[2] * mins[2];
            break;
        case 1:
            d1 = p.n[0] * mins[0] + p.n[1] * maxs[1] + p.n[2] * maxs[2];
            d2 = p.n[0] * maxs[0] + p.n[1] * mins[1] + p.n[2] * mins[2];
            break;
        case 2:
            d1 = p.n[0] * maxs[0] + p.n[1] * mins[1] + p.n[2] * maxs[2];
            d2 = p.n[0] * mins[0] + p.n[1] * maxs[1] + p.n[2] * mins[2];
            break;
        case 3:
            d1 = p.n[0] * mins[0] + p.n[1] * mins[1] + p.n[2] * maxs[2];
            d2 = p.n[0] * maxs[0] + p.n[1] * maxs[1] + p.n[2] * mins[2];
            break;
        case 4:
            d1 = p.n[0] * maxs[0] + p.n[1] * maxs[1] + p.n[2] * mins[2];
            d2 = p.n[0] * mins[0] + p.n[1] * mins[1] + p.n[2] * maxs[2];
            break;
        case 5:
            d1 = p.n[0] * mins[0] + p.n[1] * maxs[1] + p.n[2] * mins[2];
            d2 = p.n[0] * maxs[0] + p.n[1] * mins[1] + p.n[2] * maxs[2];
            break;
        case 6:
            d1 = p.n[0] * maxs[0] + p.n[1] * mins[1] + p.n[2] * mins[2];
            d2 = p.n[0] * mins[0] + p.n[1] * maxs[1] + p.n[2] * maxs[2];
            break;
        case 7:
            d1 = p.n[0] * mins[0] + p.n[1] * mins[1] + p.n[2] * mins[2];
            d2 = p.n[0] * maxs[0] + p.n[1] * maxs[1] + p.n[2] * maxs[2];
            break;
        default:
            assert(0);
    }
    int r = 0;
    if (d1 >= p.d)
        r |= PSR_FRONT;
    if (d2 < p.d)
        r |= PSR_BACK;
    assert(r);
    return r;
}

// VPlane::GetPointSide
int BallOnPlaneSide(const Vector& c, float r, const VPlane& p)
{
    float d = c.Dot(p.n) - p.d;
    if (d >= r)
        return PSR_FRONT;
    if (d <= -r)
        return PSR_BACK;
    return PSR_ON;
}
