#pragma once

#include "monocle_config.hpp"
#include "source_math.hpp"

#include <cmath>
#include <numbers>
#include <utility>

/*
* This file replicates portal creation code, but with doubles. I take this as ground truth to
* compare against the float code. If the code here is confusing, look at the floating point
* version for a reference.
* 
* All constructors that create values from the float versions are marked as explicit.
*/

namespace mon {

struct VectorD {
    double x, y, z;

    constexpr VectorD() {}
    constexpr VectorD(double x, double y, double z) : x{x}, y{y}, z{z} {}
    constexpr explicit VectorD(const Vector& v) : x{v.x}, y{v.y}, z{v.z} {}

    constexpr double Dot(const VectorD& v) const
    {
        return x * v.x + y * v.y + z * v.z;
    }

    constexpr double& operator[](int i)
    {
        MON_ASSERT(i >= 0 && i < 3);
        return ((double*)this)[i];
    }

    constexpr double operator[](int i) const
    {
        MON_ASSERT(i >= 0 && i < 3);
        return ((double*)this)[i];
    }

    constexpr VectorD operator+(const VectorD& v) const
    {
        return {x + v.x, y + v.y, z + v.z};
    }

    constexpr VectorD operator-(const VectorD& v) const
    {
        return {x - v.x, y - v.y, z - v.z};
    }

    constexpr VectorD operator-() const
    {
        return {-x, -y, -z};
    }
};

struct QAngleD {
    double x, y, z;

    constexpr explicit QAngleD(const QAngle& a) : x{a.x}, y{a.y}, z{a.z} {}

    double& operator[](int i)
    {
        MON_ASSERT(i >= 0 && i < 3);
        return ((double*)this)[i];
    }

    double operator[](int i) const
    {
        MON_ASSERT(i >= 0 && i < 3);
        return ((double*)this)[i];
    }
};

struct SinCosComponentsD {
    double sp, cp, sy, cy, sr, cr;

    SinCosComponentsD(const QAngleD& ang)
        : sp{std::sin(ang[0] * std::numbers::pi / 180.)},
          cp{std::cos(ang[0] * std::numbers::pi / 180.)},
          sy{std::sin(ang[1] * std::numbers::pi / 180.)},
          cy{std::cos(ang[1] * std::numbers::pi / 180.)},
          sr{std::sin(ang[2] * std::numbers::pi / 180.)},
          cr{std::cos(ang[2] * std::numbers::pi / 180.)}
    {}
};

inline void AngleVectorsD(const QAngleD& ang, VectorD* f, VectorD* r, VectorD* u)
{
    SinCosComponentsD c{ang};
    if (f)
        *f = {c.cp * c.cy, c.cp * c.sy, -c.sp};
    if (r)
        *r = {-1 * c.sr * c.sp * c.cy + -1 * c.cr * -c.sy,
              -1 * c.sr * c.sp * c.sy + -1 * c.cr * c.cy,
              -1 * c.sr * c.cp};
    if (u)
        *u = {c.cr * c.sp * c.cy + -c.sr * -c.sy, c.cr * c.sp * c.sy + -c.sr * c.cy, c.cr * c.cp};
}

struct matrix3x4_t_d {
    double m_flMatVal[3][4];

    double* operator[](int i)
    {
        MON_ASSERT(i >= 0 && i < 3);
        return m_flMatVal[i];
    }

    const double* operator[](int i) const
    {
        MON_ASSERT(i >= 0 && i < 3);
        return m_flMatVal[i];
    }
};

inline void AngleMatrixD(const QAngleD& ang, const VectorD& pos, matrix3x4_t_d& m)
{
    m[0][3] = pos.x;
    m[1][3] = pos.y;
    m[2][3] = pos.z;
    SinCosComponentsD c{ang};
    m[0][0] = c.cp * c.cy;
    m[1][0] = c.cp * c.sy;
    m[2][0] = -c.sp;
    m[0][1] = c.sp * c.sr * c.cy - c.cr * c.sy;
    m[1][1] = c.sp * c.sr * c.sy + c.cr * c.cy;
    m[2][1] = c.sr * c.cp;
    m[0][2] = c.sp * c.cr * c.cy + c.sr * c.sy;
    m[1][2] = c.sp * c.cr * c.sy - c.sr * c.cy;
    m[2][2] = c.cr * c.cp;
};

struct VMatrixD {
    double m[4][4];

    constexpr VMatrixD() {}

    constexpr VMatrixD(const VectorD& x, const VectorD& y, const VectorD& z, const VectorD& pos)
        : m{
              {x.x, y.x, z.x, pos.x},
              {x.y, y.y, z.y, pos.y},
              {x.z, y.z, z.z, pos.z},
              {0.f, 0.f, 0.f, 1.f},
          }
    {}

    double* operator[](int i)
    {
        return m[i];
    }

    const double* operator[](int i) const
    {
        return m[i];
    }

    VMatrixD operator*(const VMatrixD& vm) const
    {
        VMatrixD ret;
        std::fill(&ret[0][0], &ret[3][3] + 1, 0.);
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    ret[i][j] += m[i][k] * vm[k][j];
        return ret;
    }

    VectorD operator*(const VectorD& v) const
    {
        VectorD res{m[0][3], m[1][3], m[2][3]};
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                res[i] += m[i][j] * v[j];
        return res;
    }
};

inline void MatrixSetIdentityD(VMatrixD& m)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            m[i][j] = i == j ? 1. : 0.;
}

inline void Vector3DMultiplyDirD(const VMatrixD& src1, VectorD src2, VectorD& dst)
{
    dst = {0., 0., 0.};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            dst[i] += src1[i][j] * src2[j];
}

inline void MatrixInverseTR_D(const VMatrixD& src, VMatrixD& dst)
{
    VectorD vTrans, vNewTrans;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            dst.m[i][j] = src.m[j][i];
    vTrans = {-src.m[0][3], -src.m[1][3], -src.m[2][3]};
    Vector3DMultiplyDirD(dst, vTrans, vNewTrans);
    dst.m[0][3] = vNewTrans.x;
    dst.m[1][3] = vNewTrans.y;
    dst.m[2][3] = vNewTrans.z;
    dst.m[3][0] = dst.m[3][1] = dst.m[3][2] = 0.;
    dst.m[3][3] = 1.;
}

struct VPlaneD {
    VectorD n;
    double d;
};

struct EntityD {
    union {
        struct {
            VectorD origin;
            bool crouched;
        } player;

        struct {
            VectorD center;
            float radius;
        } ball;
    };

    bool is_player;

    EntityD() {};

    explicit EntityD(const Entity& ent)
    {
        *this = ent.is_player ? EntityD::CreatePlayerFromOrigin(VectorD{ent.player.origin}, ent.player.crouched)
                              : EntityD::CreateBall(VectorD{ent.ball.center}, ent.ball.radius);
    }

    static EntityD CreatePlayerFromOrigin(VectorD origin, bool crouched)
    {
        EntityD ent;
        ent.is_player = true;
        ent.player.origin = origin;
        ent.player.crouched = crouched;
        return ent;
    }

    static EntityD CreatePlayerFromCenter(VectorD center, bool crouched)
    {
        return CreatePlayerFromOrigin(center - VectorD{crouched ? PLAYER_CROUCH_HALF : PLAYER_STAND_HALF}, crouched);
    }

    static EntityD CreateBall(VectorD center, float radius)
    {
        EntityD ent;
        ent.is_player = false;
        ent.ball.center = center;
        ent.ball.radius = radius;
        return ent;
    }

    VectorD GetCenter() const
    {
        if (is_player)
            return player.origin + VectorD{player.crouched ? PLAYER_CROUCH_HALF : PLAYER_STAND_HALF};
        return ball.center;
    }
};

struct PortalD {
    VectorD pos;
    QAngleD ang;

    VectorD f, r, u;
    VPlaneD plane;
    matrix3x4_t_d mat;

    PortalD(const VectorD& pos, const QAngleD& ang) : pos{pos}, ang{ang}
    {
        AngleVectorsD(ang, &f, &r, &u);
        plane = {f, f.Dot(pos)};
        AngleMatrixD(ang, pos, mat);
    }

    explicit PortalD(const Portal& p) : PortalD{VectorD{p.pos}, QAngleD{p.ang}} {}
};

struct PortalPairD {
    PortalD blue, orange;
    VMatrixD b_to_o, o_to_b;

    PortalPairD(const PortalD& blue, const PortalD& orange) : blue{blue}, orange{orange}
    {
        CalcTpMats();
    }

    explicit PortalPairD(const PortalPair& pp) : PortalPairD{PortalD{pp.blue}, PortalD{pp.orange}} {}

    void CalcTpMats()
    {
        VMatrixD matLocalToWorldInv, matRotation, tmp;
        for (int i = 0; i < 2; i++) {
            auto& my_portal = i == 0 ? blue : orange;
            auto& other_portal = i == 1 ? blue : orange;
            VMatrixD portal_to_world{my_portal.f, -my_portal.r, my_portal.u, my_portal.pos};
            VMatrixD other_to_world{other_portal.f, -other_portal.r, other_portal.u, other_portal.pos};
            MatrixInverseTR_D(portal_to_world, matLocalToWorldInv);
            MatrixSetIdentityD(matRotation);
            matRotation[0][0] = -1.f;
            matRotation[1][1] = -1.f;
            (i == 0 ? b_to_o : o_to_b) = other_to_world * matRotation * matLocalToWorldInv;
        }
    }

    EntityD Teleport(const EntityD& ent, bool tp_from_blue) const
    {
        VectorD old_center = ent.GetCenter();
        const VectorD& old_player_origin = ent.player.origin;
        bool player_crouched = ent.player.crouched;

        if (ent.is_player) {
            const PortalD& p = tp_from_blue ? blue : orange;
            const PortalD& op = tp_from_blue ? orange : blue;

            if (!player_crouched && std::abs(p.f.z) > 0.f &&
                (std::abs(std::abs(p.f.z) - 1.f) >= .01f || std::abs(std::abs(op.f.z) - 1.f) >= .01f)) {
                if (p.f.z > 0.f)
                    old_center.z -= 16.f;
                else
                    old_center.z += 16.f;
                player_crouched = true;
            }
        }

        VectorD new_center = (tp_from_blue ? b_to_o : o_to_b) * old_center;
        if (ent.is_player)
            return EntityD::CreatePlayerFromOrigin(new_center + (old_player_origin - old_center), player_crouched);
        return EntityD::CreateBall(new_center, ent.ball.radius);
    }
};

} // namespace mon
