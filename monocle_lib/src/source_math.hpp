#pragma once

#include <float.h>
#include <array>
#include <string>

#include "math.h"
#include "stdio.h"
#include "assert.h"

#define F_FMT "%.9g"

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
                     ~0u);
    assert(!err);
}

#ifndef NDEBUG
#define DEBUG_NAN_CTORS
#endif

template <size_t R, size_t C>
static void PrintMatrix(const float (&arr)[R][C])
{
    int fmtLens[R][C]{};
    int maxLens[C]{};
    for (int i = 0; i < C; i++) {
        for (int j = 0; j < R; j++) {
            fmtLens[j][i] = snprintf(nullptr, 0, F_FMT, arr[j][i]);
            if (fmtLens[j][i] > maxLens[i])
                maxLens[i] = fmtLens[j][i];
        }
    }
    for (int j = 0; j < R; j++) {
        for (int i = 0; i < C; i++) {
            printf("%*s" F_FMT "%s",
                   maxLens[i] - fmtLens[j][i],
                   "",
                   arr[j][i],
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
    constexpr explicit Vector(float v) : x{v}, y{v}, z{v} {}
    constexpr Vector(float x, float y, float z) : x{x}, y{y}, z{z} {}

    void print() const
    {
        printf(F_FMT " " F_FMT " " F_FMT, x, y, z);
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

    double constexpr Dot(const Vector& v) const
    {
        /*
        * Since vs2005 uses x87 ops for everything, the result of the dot product is secretly a
        * double. This behavior is annoying to replicate here since newer versions of vs don't
        * like to use the x87 ops and the result is truncated when it's stored in the st0 reg.
        * Hopefully we can replicate this by returning a double. So long as the result is compared
        * with floats I think it should be fine, otherwise we might have to worry about the order
        * of the addition: (x+y)+z != x+(y+z) in general, but equal if converting double->float.
        */
        return (double)x * v.x + (double)y * v.y + (double)z * v.z;
    }

    // probably not accurate to game code (double -> float truncation issue probably)
    float DistToSqr(const Vector& v) const
    {
        Vector d = *this - v;
        return (float)d.Dot(d);
    }

    // probably not accurate to game code
    float DistTo(const Vector& v) const
    {
        return sqrtf(DistToSqr(v));
    }

    constexpr Vector operator*(float f) const
    {
        return Vector{x * f, y * f, z * f};
    }

    constexpr bool operator==(const Vector& o) const
    {
        return x == o.x && y == o.y && z == o.z;
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

    // not accurate to what game does
    float DistTo(const Vector& v) const
    {
        return (float)(n.Dot(v) - d);
    }

    void print() const
    {
        printf("n=(");
        n.print();
        printf("), d=" F_FMT, d);
    }
};

// g_DefaultViewVectors
static constexpr Vector PLAYER_CROUCH_MINS{-16.f, -16.f, 0.f};
static constexpr Vector PLAYER_CROUCH_MAXS{16.f, 16.f, 36.f};
static constexpr Vector PLAYER_CROUCH_HALF = (PLAYER_CROUCH_MINS + PLAYER_CROUCH_MAXS) * .5f;
static constexpr Vector PLAYER_STAND_MINS{-16.f, -16.f, 0.f};
static constexpr Vector PLAYER_STAND_MAXS{16.f, 16.f, 72.f};
static constexpr Vector PLAYER_STAND_HALF = (PLAYER_STAND_MINS + PLAYER_STAND_MAXS) * .5f;

#define N_CHILDREN_PLAYER_WITHOUT_PORTAL_GUN 1
#define N_CHILDREN_PLAYER_WITH_PORTAL_GUN 2

struct Entity {

    union {
        // player - only store origin and do math to get center
        struct {
            Vector origin;
            bool crouched;
        } player;

        // non-player - treat as ball (game uses mesh but I'm approximating)
        struct {
            Vector center;
            float radius;
        } ball;
    };

    bool is_player;

    /*
    * The number of children the entity has. Usually this is N_CHILDREN_PLAYER_WITH_PORTAL_GUN for
    * the player and 0 otherwise.
    * 
    * This *is* used in the chain generation code, and it's possible that it may be relevant in
    * some niche cases, although I haven't found any (yet).
    */
    unsigned short n_children;

public:
#ifdef DEBUG_NAN_CTORS
    Entity() : is_player(true), player(Vector{}, true), n_children(N_CHILDREN_PLAYER_WITH_PORTAL_GUN) {}
#else
    Entity() {}
#endif

    // named constructors
    static Entity CreatePlayerFromOrigin(Vector origin, bool crouched);
    static Entity CreatePlayerFromCenter(Vector center, bool crouched);
    static Entity CreateBall(Vector center, float radius);

    Vector GetWorldMins() const;
    Vector GetWorldMaxs() const;
    Vector GetCenter() const;
    bool operator==(const Entity& other) const;

    Vector& GetPosRef()
    {
        return is_player ? player.origin : ball.center;
    }

    std::string GetSetPosCmd() const;
};

struct plane_bits {
    // 0:x, 1:y, 2:z, 3:non-axial
    uint8_t type : 3;
    uint8_t sign : 3;
};

struct Portal {
    Vector pos; // m_vecOrigin/m_vecAbsOrigin
    QAngle ang; // m_angAngles/m_angAbsAngles

    Vector f, r, u;  // m_PortalSimulator.m_InternalData.Placement.vForward/vRight/vUp
    VPlane plane;    // m_PortalSimulator.m_InternalData.Placement.PortalPlane == CProp_portal::m_plane_Origin
    matrix3x4_t mat; // m_rgflCoordinateFrame

    // fHolePlanes as calculated in CPortalSimulator::MoveTo, not guaranteed to match game's values exactly
    VPlane hole_planes[6];
    plane_bits hole_planes_bits[6]; // for fast box plane tests

    Portal(const Vector& v, const QAngle& q);

    // follows the logic in ShouldTeleportTouchingEntity
    bool ShouldTeleport(const Entity& ent, bool check_portal_hole) const;

    std::string CreateNewLocationCmd(std::string_view portal_name, bool escape_quotes = false) const;

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

/*
* When a portal is placed, it calculates its teleport matrix "from scratch" and sets the other
* portal's matrix to the inverse. When portals are loaded in via a save, they both calculate their
* matrix "from scratch". Of course due to floating point shenanigans, these 3 cases *may* change
* the exact values in the teleport matrices. At the end of the day, those are the only 3 cases:
* 
* - blue is last placed portal
* - orange is last placed portal
* - portals loaded in from save
* 
* The relevant game functions for setting these matrices is ridiculously obtuse - not math-wise,
* but because the functions are sometimes called multiple times which overwrites the old values.
* I've written out the rough call stack for a bunch of different scenarios with '***' denoting
* where the matrices are written for the final time.
*/
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
    PlacementOrder order;

    PortalPair(const Portal& blue, const Portal& orange, PlacementOrder order)
        : blue{blue}, orange{orange}, order{order}
    {
        RecalcTpMatrices(order);
    };

    PortalPair(const Vector& blue_pos,
               const QAngle& blue_ang,
               const Vector& orange_pos,
               const QAngle& orange_ang,
               PlacementOrder order)
        : blue{blue_pos, blue_ang}, orange{orange_pos, orange_ang}, order{order}
    {
        RecalcTpMatrices(order);
    }

    // sets b_to_o & o_to_b
    void RecalcTpMatrices(PlacementOrder order);

    Entity Teleport(const Entity& ent, bool tp_from_blue) const;
    Vector Teleport(const Vector& pt, bool tp_from_blue) const;

    std::string CreateNewLocationCmd(std::string_view delim = "\n", bool escape_quotes = false) const;

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
};

enum PlaneSideResult {
    PSR_BACK = 1,
    PSR_FRONT = 2,
    PSR_ON = PSR_BACK | PSR_FRONT,
};

int BoxOnPlaneSide(const Vector& mins, const Vector& maxs, const VPlane& p, plane_bits bits);
int BallOnPlaneSide(const Vector& c, float r, const VPlane& p);
