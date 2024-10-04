#pragma once

#include "math.h"
#include "stdio.h"
#include "assert.h"

#define FLOAT_SPEC "%g"
#define M_PI 3.14159265358979323846
constexpr float M_PI_F = ((float)(M_PI));
constexpr float _DEG2RAD_MUL = (float)(M_PI_F / 180.f);
// #define DEG2RAD(x) ((float)(x) * _DEG2RAD_MUL)

#ifdef _DEBUG
#define DEBUG_NAN_CTORS
#endif

template <size_t R, size_t C>
static void PrintMatrix(const float (&arr)[R][C])
{
    int fmtLens[R][C]{};
    int maxLens[C]{};
    for (int i = 0; i < C; i++) {
        for (int j = 0; j < R; j++) {
            fmtLens[j][i] = snprintf(nullptr, 0, FLOAT_SPEC, arr[j][i]);
            if (fmtLens[j][i] > maxLens[i])
                maxLens[i] = fmtLens[j][i];
        }
    }
    for (int j = 0; j < R; j++) {
        for (int i = 0; i < C; i++) {
            printf("%*s" FLOAT_SPEC "%s",
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
    Vector(float x, float y, float z) : x{x}, y{y}, z{z} {}

    void print() const
    {
        printf("<%g, %g, %g>", x, y, z);
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

    float& Vector::operator[](int i)
    {
        assert(i >= 0 && i < 3);
        return ((float*)this)[i];
    }

    float Vector::operator[](int i) const
    {
        assert(i >= 0 && i < 3);
        return ((float*)this)[i];
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
    VPlane(const Vector& n, float d) : n{n}, d{d} {}

    void print() const
    {
        printf("(n=<%g, %g, %g> d=%g)", n.x, n.y, n.z, d);
    }
};

struct Portal {
    Vector pos;
    QAngle ang;

    Vector f, r, u;  // m_PortalSimulator.m_InternalData.Placement.vForward/vRight/vUp
    VPlane plane;    // m_PortalSimulator.m_InternalData.Placement.PortalPlane
    matrix3x4_t mat; // m_rgflCoordinateFrame

    Portal(const Vector& v, const QAngle& q);

    // follows the logic in ShouldTeleportTouchingEntity
    bool ShouldTeleport(const Vector& ent_center, bool check_portal_hole) const;

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

struct PortalPair {
    // p1 is the one that calculates the "primary" matrix, i.e. the portal that's placed second
    // TODO VERIFY THAT THE SECOND PLACED PORTAL IS THE PRIMARY ONE
    Portal p1;
    Portal p2;
    VMatrix p1_to_p2, p2_to_p1;

    PortalPair(const Portal& p1, const Portal& p2);
    PortalPair(const Vector& v1, const QAngle& q1, const Vector& v2, const QAngle& q2) : PortalPair{{v1, q1}, {v2, q2}}
    {}

    // TeleportTouchingEntity for a non-player entity
    Vector TeleportNonPlayerEntity(const Vector& pt, bool tp_with_p1) const;

    void print() const
    {
        printf("p1:\n");
        p1.print();
        printf("\np2:\n");
        p2.print();
        printf("\np1_to_p2:\n");
        p1_to_p2.print();
        printf("\np2_to_p1:\n");
        p2_to_p1.print();
    }
};
