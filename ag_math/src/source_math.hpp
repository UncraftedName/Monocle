#pragma once

#include <float.h>
#include <charconv>
#include <array>

#include "math.h"
#include "stdio.h"
#include "assert.h"

#define F_TO_STR_BUF_SIZE 24

#define PORTAL_HALF_WIDTH 32.0f
#define PORTAL_HALF_HEIGHT 54.0f
#define PORTAL_HOLE_DEPTH 500.f

inline void SyncFloatingPointControlWord()
{
    // 0x9001f (default msvc settings) - mask all exceptions, near rounding, 53 bit mantissa precision, projective infinity
    errno_t err =
        _controlfp_s(nullptr,
                     (_EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID | _EM_DENORMAL) |
                         (_RC_NEAR | _PC_53 | _IC_PROJECTIVE),
                     ~0);
    assert(!err);
}

#ifdef _DEBUG
#define DEBUG_NAN_CTORS
#endif

template <size_t R, size_t C>
static void PrintMatrix(const float (&arr)[R][C])
{
    char buf[F_TO_STR_BUF_SIZE];
    int fmtLens[R][C]{};
    int maxLens[C]{};
    for (int i = 0; i < C; i++) {
        for (int j = 0; j < R; j++) {
            fmtLens[j][i] = std::to_chars(buf, buf + sizeof buf, arr[j][i]).ptr - buf;
            if (fmtLens[j][i] > maxLens[i])
                maxLens[i] = fmtLens[j][i];
        }
    }
    for (int j = 0; j < R; j++) {
        for (int i = 0; i < C; i++) {
            auto end = std::to_chars(buf, buf + sizeof buf, arr[j][i]).ptr;
            printf("%*s%.*s%s",
                   maxLens[i] - fmtLens[j][i],
                   "",
                   end - buf,
                   buf,
                   i == C - 1 ? (j == R - 1 ? "" : "\n") : ", ");
        }
    }
}

struct Vector {
    float x, y, z;

#ifdef DEBUG_NAN_CTORS
    Vector() : x{NAN}, y{NAN}, z{NAN} {}
#else
    Vector() {}
#endif
    constexpr Vector(float x, float y, float z) : x{x}, y{y}, z{z} {}

    void print() const
    {
        char buf[(F_TO_STR_BUF_SIZE + 2) * 3];
        auto cur = buf;
        auto end = buf + sizeof buf;

        cur++[0] = '<';
        cur = std::to_chars(cur, end, x).ptr;
        cur++[0] = ',';
        cur++[0] = ' ';
        cur = std::to_chars(cur, end, y).ptr;
        cur++[0] = ',';
        cur++[0] = ' ';
        cur = std::to_chars(cur, end, z).ptr;
        cur++[0] = '>';

        printf("%.*s", cur - buf, buf);
    }

    Vector& operator+=(const Vector& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    constexpr Vector operator+(const Vector& v) const
    {
        return Vector{x + v.x, y + v.y, z + v.z};
    }

    Vector& operator-=(const Vector& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    constexpr Vector operator-(const Vector& v) const
    {
        return Vector{x - v.x, y - v.y, z - v.z};
    }

    constexpr Vector operator-() const
    {
        return Vector{-x, -y, -z};
    }

    float& operator[](int i)
    {
        assert(i >= 0 && i < 3);
        return ((float*)this)[i];
    }

    float operator[](int i) const
    {
        assert(i >= 0 && i < 3);
        return ((float*)this)[i];
    }

    float constexpr Dot(const Vector& v) const
    {
        // hopefully this is enough to force the intermediate ops to be 53 bit, seems like (x+y)+z == x+(y+z)
        return (float)((double)x * v.x + (double)y * v.y + (double)z * v.z);
    }

    float DistToSqr(const Vector& v) const
    {
        Vector d = *this - v;
        return d.Dot(d);
    }

    constexpr Vector operator*(float f) const
    {
        return Vector{x * f, y * f, z * f};
    }
};

struct QAngle {
    float x, y, z;

#ifdef DEBUG_NAN_CTORS
    QAngle() : x{NAN}, y{NAN}, z{NAN} {}
#else
    QAngle() {}
#endif
    constexpr QAngle(float x, float y, float z) : x{x}, y{y}, z{z} {}

    void print() const
    {
        Vector{x, y, z}.print();
    }
};

struct matrix3x4_t {
    float m_flMatVal[3][4];

#ifdef DEBUG_NAN_CTORS
    matrix3x4_t() : m_flMatVal{NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN} {}
#else
    matrix3x4_t() {}
#endif

    float* operator[](int i)
    {
        assert((i >= 0) && (i < 3));
        return m_flMatVal[i];
    }

    void print() const
    {
        PrintMatrix(m_flMatVal);
    }
};

struct VMatrix {
    float m[4][4];

#ifdef DEBUG_NAN_CTORS
    VMatrix() : m{NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN} {}
#else
    VMatrix() {}
#endif
    constexpr VMatrix(const Vector& x, const Vector& y, const Vector& z, const Vector& pos)
        : m{
              {x.x, y.x, z.x, pos.x},
              {x.y, y.y, z.y, pos.y},
              {x.z, y.z, z.z, pos.z},
              {0.f, 0.f, 0.f, 1.f},
          }
    {}

    inline float* operator[](int i)
    {
        return m[i];
    }

    void print() const
    {
        PrintMatrix(m);
    }
};

struct VPlane {
    Vector n;
    float d;

#ifdef DEBUG_NAN_CTORS
    VPlane() : n{}, d{NAN} {}
#else
    VPlane() {}
#endif
    constexpr VPlane(const Vector& n, float d) : n{n}, d{d} {}

    void print() const
    {
        char buf[F_TO_STR_BUF_SIZE];
        auto end = std::to_chars(buf, buf + sizeof buf, d).ptr;
        printf("(n=(");
        n.print();
        printf("), d=%.*s", end - buf, buf);
    }
};

static constexpr Vector PLAYER_CROUCH_MINS{-16.f, -16.f, 0.f};
static constexpr Vector PLAYER_CROUCH_MAXS{16.f, 16.f, 36.f};

struct Entity {

    bool player;
    Vector origin;
    // if player, use AABB; if non-player, treat as ball (game uses actually mesh but I don't need that)
    float radius;

    Entity() : radius{NAN} {};
    // player ctor
    explicit Entity(const Vector& center)
        : player{true}, origin{center - (PLAYER_CROUCH_MAXS + PLAYER_CROUCH_MINS) * .5f}, radius{NAN}
    {}
    // non-player ctor (assume center == origin)
    explicit Entity(const Vector& center, float r) : player{false}, origin{center}, radius{r} {}

    Vector GetCenter() const
    {
        return player ? origin + (PLAYER_CROUCH_MAXS + PLAYER_CROUCH_MINS) * .5f : origin;
    }

    void PrintSetposCmd() const
    {
        assert(player);
        printf("setpos %.9g %.9g %.9g\n", origin.x, origin.y, origin.z);
    }
};

struct plane_bits {
    // 0:x, 1:y, 2:z, 3:non-axial
    uint8_t type : 3;
    uint8_t sign : 3;
};

// TODO - these fields might be generated a different way if the portal loaded in?
struct Portal {
    Vector pos; // m_vecOrigin/m_vecAbsOrigin
    QAngle ang; // m_angAngles/m_angAbsAngles

    Vector f, r, u;      // m_PortalSimulator.m_InternalData.Placement.vForward/vRight/vUp
    VPlane plane;        // m_PortalSimulator.m_InternalData.Placement.PortalPlane == CProp_portal::m_plane_Origin
    matrix3x4_t mat;     // m_rgflCoordinateFrame

    // fHolePlanes as calculated in CPortalSimulator::MoveTo, not guaranteed to match game's values exactly
    VPlane hole_planes[6];
    plane_bits hole_planes_bits[6]; // for fast box plane tests

    Portal(const Vector& v, const QAngle& q);

    // follows the logic in ShouldTeleportTouchingEntity
    bool ShouldTeleport(const Entity& ent, bool check_portal_hole) const;

    void print() const
    {
        printf("pos: ");
        pos.print();
        printf(", ang: ");
        ang.print();
        printf("\nf: ");
        f.print();
        printf("\nr: ");
        r.print();
        printf("\nu: ");
        u.print();
        printf("\nplane: ");
        plane.print();
        printf("\nmat:\n");
        mat.print();
    }
};

enum class PlacementOrder {

    /*
    * Teleport matrices are calculated in UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix.
    * Color here is the "primary" matrix and the other is the inverse.
    */
    _BLUE_UPTM,
    _ORANGE_UPTM,

    // Teleport matrices are calculated in UpdateLinkMatrix; both matrices are calculated the same way.
    _ULM,

    /*
    * Both portals are open, one of them was moved (either with portal gun or newlocation).
    * callstack:
    * blue NewLocation -> UpdatePortalLinkage -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix
    *                                         -> MoveTo -> UpdateLinkMatrix
    *                                                                       -> orange UpdateLinkMatrix
    *                  -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix ***
    */
    ORANGE_OPEN_BLUE_NEW_LOCATION = _BLUE_UPTM,
    BLUE_OPEN_ORANGE_NEW_LOCATION = _ORANGE_UPTM,

    /*
    * Both portals are open and you shot one or used newlocation, but its position and angles didn't change.
    * callstack:
    * blue NewLocation -> UpdatePortalLinkage -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix
    *                  -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix ***
    */
    ORANGE_OPEN_BLUE_NEW_LOCATION_NO_MOVE = _BLUE_UPTM,
    BLUE_OPEN_ORANGE_NEW_LOCATION_NO_MOVE = _ORANGE_UPTM,

    /*
    * Orange exists (closed & activated), blue is not activated and you shot blue somewhere.
    * callstack:
    * blue NewLocation -> UpdatePortalLinkage -> orange UpdatePortalLinkage -> UpdatePortalTransformationMatrix
    *                                                                       -> AttachTo -> UpdateLinkMatrix
    *                                                                                                       -> blue UpdateLinkMatrix
    *                                         -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix
    *                                         -> MoveTo -> UpdateLinkMatrix
    *                                                                       -> orange UpdateLinkMatrix
    *                  -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix ***
    * 
    * Note: during the call to blue's UpdateLinkMatrix, the simulator's position hasn't been updated yet.
    */
    ORANGE_WAS_CLOSED_BLUE_MOVED = _BLUE_UPTM,
    BLUE_WAS_CLOSED_ORANGE_MOVED = _ORANGE_UPTM,

    /*
    * Orange is closed and blue did exist but was just placed.
    * callstack:
    * blue NewLocation -> UpdatePortalLinkage -> orange UpdatePortalLinkage -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix
    *                                         -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix
    *                                         -> MoveTo -> UpdateLinkMatrix
    *                                                                       -> orange UpdateLinkMatrix
    *                  -> UpdatePortalTeleportMatrix -> UpdatePortalTransformationMatrix ***
    */
    ORANGE_WAS_CLOSED_BLUE_CREATED = _BLUE_UPTM,
    BLUE_WAS_CLOSED_ORANGE_CREATED = _ORANGE_UPTM,

    /*
    * Orange exists (closed & activated), blue is not activated and it opened.
    * The callstack is the same as the created case except there's an additional input fired on the opened
    * portal which calls UpdatePortalTransformationMatrix a couple times.
    * *callstack might not be the same if the portal existed already? whatever, doesn't change the result.
    */
    ORANGE_WAS_CLOSED_BLUE_OPENED = ORANGE_WAS_CLOSED_BLUE_CREATED,
    BLUE_WAS_CLOSED_ORANGE_OPENED = BLUE_WAS_CLOSED_ORANGE_CREATED,

    AFTER_LOAD = _ULM,

    COUNT,
};

static std::array<const char*, (int)PlacementOrder::COUNT> PlacementOrderStrs{
    "BLUE_UpdatePortalTransformationMatrix",
    "ORANGE_UpdatePortalTransformationMatrix",
    "UpdateLinkMatrix",
};

struct PortalPair {
    Portal blue, orange;
    VMatrix b_to_o, o_to_b;

    PortalPair(const Portal& blue, const Portal& orange) : blue{blue}, orange{orange} {};
    PortalPair(const Vector& blue_pos, const QAngle& blue_ang, const Vector& orange_pos, const QAngle& orange_ang)
        : blue{blue_pos, blue_ang}, orange{orange_pos, orange_ang}
    {}

    // sets b_to_o & o_to_b
    void CalcTpMatrices(PlacementOrder order);

    // TeleportTouchingEntity (if player, assumes they are crouched)
    void Teleport(Entity& ent, bool tp_from_blue) const;
    Vector Teleport(const Vector& pt, bool tp_from_blue) const;

    void print() const
    {
        printf("blue:\n");
        blue.print();
        printf("\nmat to linked:\n");
        b_to_o.print();
        printf("\n\n----------------------------------------\n\norange:\n");
        orange.print();
        printf("\n\nmat to linked:\n");
        o_to_b.print();
    }

    void PrintNewlocationCmd() const
    {
        for (int i = 0; i < 2; i++) {
            auto& p = i ? blue : orange;
            printf("ent_fire %s newlocation \"", i ? "blue" : "orange");
            for (int j = 0; j < 2; j++) {
                const Vector& v = j ? *(Vector*)&p.ang : p.pos;
                for (int k = 0; k < 3; k++) {
                    char buf[F_TO_STR_BUF_SIZE];
                    auto end = std::to_chars(buf, buf + sizeof buf, v[k]).ptr;
                    printf("%.*s%s", end - buf, buf, j == 1 && k == 2 ? "\"\n" : " ");
                }
            }
        }
    }
};

enum PlaneSideResult {
    PSR_BACK = 1,
    PSR_FRONT = 2,
    PSR_ON = PSR_BACK | PSR_FRONT,
};

int BoxOnPlaneSide(const Vector& mins, const Vector& maxs, const VPlane& p, plane_bits bits);
int BallOnPlaneSide(const Vector& c, float r, const VPlane& p);
