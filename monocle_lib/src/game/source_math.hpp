#pragma once

#include "monocle_config.hpp"

#include <float.h>
#include <array>
#include <string>
#include <math.h>
#include <optional>
#include <utility>
#include <charconv>

/*
* This file replicates common source-engine types. Unless otherwise noted, the functions here
* should copy the game to floating point precision.
*/

namespace mon {

// older versions of the game used x87 ops instead of SSE
enum GameVersion : uint16_t {
    GV_5135,
    GV_9862575,
};

constexpr float PORTAL_HALF_WIDTH = 32.f;
constexpr float PORTAL_HALF_HEIGHT = 54.f;
constexpr float PORTAL_HOLE_DEPTH = 500.f;

// sets FPU flags to what the game uses, setup before using any monocle code
class MonocleFloatingPointScope {
    unsigned int old_control;

public:
    MonocleFloatingPointScope()
    {
        // 0x9001f (default msvc settings) - mask all exceptions, near rounding, 53 bit mantissa precision, projective infinity
        unsigned int new_control =
            (_EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID | _EM_DENORMAL) |
            (_RC_NEAR | _PC_53 | _IC_PROJECTIVE);
        errno_t err = _controlfp_s(&old_control, new_control, ~0u);
        (void)err;
        MON_ASSERT(!err);
    }

    ~MonocleFloatingPointScope()
    {
        errno_t err = _controlfp_s(nullptr, old_control, ~0u);
        (void)err;
        MON_ASSERT(!err);
    }
};

#ifndef NDEBUG
#define DEBUG_NAN_CTORS
#endif

struct Vector {
    float x, y, z;

#ifdef DEBUG_NAN_CTORS
    Vector() : x{NAN}, y{NAN}, z{NAN} {}
#else
    Vector() {}
#endif
    constexpr explicit Vector(float v) : x{v}, y{v}, z{v} {}
    constexpr Vector(float x, float y, float z) : x{x}, y{y}, z{z} {}

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
        MON_ASSERT(i >= 0 && i < 3);
        return ((float*)this)[i];
    }

    float operator[](int i) const
    {
        MON_ASSERT(i >= 0 && i < 3);
        return ((float*)this)[i];
    }

    // not accurate to game code
    float Dot(const Vector& v) const
    {
        return x * v.x + y * v.y + z * v.z;
    }

    // not accurate to game code
    float DistToSqr(const Vector& v) const
    {
        Vector d = *this - v;
        return (float)d.Dot(d);
    }

    // not accurate to game code
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

    std::string ToString(std::string_view delim = ", ") const;
};

struct QAngle {
    float x, y, z;

#ifdef DEBUG_NAN_CTORS
    QAngle() : x{NAN}, y{NAN}, z{NAN} {}
#else
    QAngle() {}
#endif
    constexpr QAngle(float x, float y, float z) : x{x}, y{y}, z{z} {}

    std::string ToString(std::string_view delim = ", ") const
    {
        return ((Vector*)this)->ToString(delim);
    }

    constexpr bool operator==(const QAngle& o) const
    {
        return x == o.x && y == o.y && z == o.z;
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
        MON_ASSERT(i >= 0 && i < 3);
        return m_flMatVal[i];
    }

    const float* operator[](int i) const
    {
        MON_ASSERT(i >= 0 && i < 3);
        return m_flMatVal[i];
    }

    std::string DebugToString() const;
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

    float* operator[](int i)
    {
        return m[i];
    }

    const float* operator[](int i) const
    {
        return m[i];
    }
public:
    // replacements for operator*
    VMatrix Multiply(const VMatrix& vm, GameVersion gv) const;
    Vector Multiply(const Vector& v, GameVersion gv) const;

    std::string DebugToString() const;
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

    // not accurate to game code
    float DistTo(const Vector& v) const
    {
        return (float)(n.Dot(v) - d);
    }

    std::string ToString() const;
};

// g_DefaultViewVectors
static constexpr Vector PLAYER_CROUCH_MINS{-16.f, -16.f, 0.f};
static constexpr Vector PLAYER_CROUCH_MAXS{16.f, 16.f, 36.f};
static constexpr Vector PLAYER_CROUCH_HALF = (PLAYER_CROUCH_MINS + PLAYER_CROUCH_MAXS) * .5f;
static constexpr Vector PLAYER_STAND_MINS{-16.f, -16.f, 0.f};
static constexpr Vector PLAYER_STAND_MAXS{16.f, 16.f, 72.f};
static constexpr Vector PLAYER_STAND_HALF = (PLAYER_STAND_MINS + PLAYER_STAND_MAXS) * .5f;

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

public:
#ifdef DEBUG_NAN_CTORS
    Entity() : is_player(true), player(Vector{}, true) {}
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

    std::string SetPosCmd() const;
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

    GameVersion gv;

    Portal(const Vector& v, const QAngle& q, GameVersion gv);

    // (slow) parse first 6 numbers as pos.x, pos.y, pos.z, ang.x, ang.y, ang.z with any delimeters
    static std::optional<std::pair<Portal, std::from_chars_result>> FromString(std::string_view sv, GameVersion gv);

    // follows the logic in ShouldTeleportTouchingEntity
    bool ShouldTeleport(const Entity& ent, bool check_portal_hole) const;

    std::string NewLocationCmd(std::string_view portal_name, bool escape_quotes = false) const;
    std::string DebugToString(std::string_view portal_name) const;
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
               PlacementOrder order,
               GameVersion gv)
        : blue{blue_pos, blue_ang, gv}, orange{orange_pos, orange_ang, gv}, order{order}
    {
        RecalcTpMatrices(order);
    }

    // sets b_to_o & o_to_b
    void RecalcTpMatrices(PlacementOrder order);

    Entity Teleport(const Entity& ent, bool tp_from_blue) const;
    Vector Teleport(const Vector& pt, bool tp_from_blue) const;

    std::string NewLocationCmd(std::string_view delim = "\n", bool escape_quotes = false) const;
    std::string DebugToString() const;
};

enum PlaneSideResult {
    PSR_BACK = 1,
    PSR_FRONT = 2,
    PSR_ON = PSR_BACK | PSR_FRONT,
};

int BoxOnPlaneSide(const Vector& mins, const Vector& maxs, const VPlane& p, plane_bits bits);
int BallOnPlaneSide(const Vector& c, float r, const VPlane& p);

} // namespace mon
