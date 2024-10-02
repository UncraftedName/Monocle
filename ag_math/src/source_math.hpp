#include "math.h"
#include "stdio.h"
#include "assert.h"

#define M_PI 3.14159265358979323846
constexpr float M_PI_F = ((float)(M_PI));
constexpr float _DEG2RAD_MUL = (float)(M_PI_F / 180.f);
// #define DEG2RAD(x) ((float)(x) * _DEG2RAD_MUL)

#ifdef _DEBUG
#define DEBUG_NAN_CTORS
#endif

struct Vector {
    float x, y, z;

#ifdef DEBUG_NAN_CTORS
    Vector() : x{NAN}, y{NAN}, z{NAN} {}
#else
    Vector() {}
#endif
    Vector(float x, float y, float z) : x{x}, y{y}, z{z} {}

    void print() const
    {
        printf("< %g, %g, %g >", x, y, z);
    }

    Vector& operator+=(const Vector& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    Vector operator+(const Vector& v) const
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

    Vector operator-(const Vector& v) const
    {
        return Vector{x - v.x, y - v.y, z - v.z};
    }

    Vector operator-() const
    {
        return Vector{-x, -y, -z};
    }
};

struct QAngle {
    float x, y, z;

#ifdef DEBUG_NAN_CTORS
    QAngle() : x{NAN}, y{NAN}, z{NAN} {}
#else
    QAngle() {}
#endif
    QAngle(float x, float y, float z) : x{x}, y{y}, z{z} {}

    void print() const
    {
        printf("<%g, %g, %g>", x, y, z);
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
        auto m = m_flMatVal;
        // clang-format off
        printf("%g, %g, %g, %g\n%g, %g, %g, %g\n%g, %g, %g, %g",
               m[0][0], m[0][1], m[0][2], m[0][3],
               m[1][0], m[1][1], m[1][2], m[1][3],
               m[2][0], m[2][1], m[2][2], m[2][3]);
        // clang-format on
    }
};

struct VMatrix {
    float m[4][4];

#ifdef DEBUG_NAN_CTORS
    VMatrix() : m{NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN} {}
#else
    VMatrix() {}
#endif

    inline float* operator[](int i)
    {
        return m[i];
    }

    void print() const
    {
        // clang-format off
        printf("%g, %g, %g, %g\n%g, %g, %g, %g\n%g, %g, %g, %g\n%g, %g, %g, %g",
               m[0][0], m[0][1], m[0][2], m[0][3],
               m[1][0], m[1][1], m[1][2], m[1][3],
               m[2][0], m[2][1], m[2][2], m[2][3],
               m[3][0], m[3][1], m[3][2], m[3][3]);
        // clang-format on
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
    VPlane(const Vector& n, float d) : n{n}, d{d} {}

    void print() const
    {
        printf("(f=<%g, %g, %g> D=%g)", n.x, n.y, n.z, d);
    }
};

struct Portal {
    Vector pos;
    QAngle ang;

    Portal(const Vector& v, const QAngle& q) : pos{v}, ang{q} {}

    // computes m_rgflCoordinateFrame
    void CalcMatrix(matrix3x4_t& out) const;
    // computes m_PortalSimulator.m_InternalData.Placement.vForward/vRight/vUp
    void CalcVectors(Vector* f, Vector* r, Vector* u) const;
    // computes m_PortalSimulator.m_InternalData.Placement.PortalPlane
    void CalcPlane(const Vector& f, VPlane& out_plane) const;
    // follows the logic in ShouldTeleportTouchingEntity
    bool ShouldTeleport(const VPlane& portal_plane, const Vector& ent_center, bool check_portal_hole) const;
    // TeleportTouchingEntity for a non-player entity
    Vector TeleportNonPlayerEntity(const VMatrix& mat, const Vector& pt) const;
};

struct PortalPair {
    Portal p1, p2;

    PortalPair(const Vector& v1, const QAngle& q1, const Vector& v2, const QAngle& q2) : p1{v1, q1}, p2{v2, q2} {}
    PortalPair(const Portal& p1, const Portal& p2) : p1{p1}, p2{p2} {}

    void CalcTeleportMatrix(const matrix3x4_t& p1_mat, const matrix3x4_t& p2_mat, VMatrix& out, bool p1_to_p2);
};

extern "C" void __cdecl AngleMatrix(const QAngle* angles, matrix3x4_t* matrix);
void AngleMatrix(const QAngle* angles, const Vector* position, matrix3x4_t* matrix);
extern "C" void __cdecl AngleVectors(const QAngle* angles, Vector* f, Vector* r, Vector* u);
extern "C" void __cdecl MatrixInverseTR(const VMatrix* src, VMatrix* dst);
extern "C" void __cdecl Vector3DMultiply(const VMatrix* src1, const Vector* src2, Vector* dst);
extern "C" void __cdecl VMatrix__MatrixMul(const VMatrix* lhs, const VMatrix* rhs, VMatrix* out);
extern "C" Vector* __cdecl VMatrix__operatorVec(const VMatrix* lhs, Vector* out, const Vector* vVec);
void MatrixSetIdentity(VMatrix& dst);
void UpdatePortalTransformationMatrix(const matrix3x4_t* localToWorld,
                                      const matrix3x4_t* remoteToWorld,
                                      VMatrix* pMatrix);
extern "C" void __cdecl Portal_CalcPlane(const Vector* portal_pos, const Vector* portal_f, VPlane* out_plane);
extern "C" bool __cdecl Portal_EntBehindPlane(const VPlane* portal_plane, const Vector* ent_center);
