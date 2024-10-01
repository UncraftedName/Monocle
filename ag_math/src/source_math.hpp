#include "math.h"
#include "stdio.h"

#define M_PI 3.14159265358979323846
constexpr float M_PI_F = ((float)(M_PI));
constexpr float _DEG2RAD_MUL = (float)(M_PI_F / 180.f);
// #define DEG2RAD(x) ((float)(x) * _DEG2RAD_MUL)

struct Vector {
    float x, y, z;

    Vector() : x{NAN}, y{NAN}, z{NAN} {}
    Vector(float x, float y, float z) : x{x}, y{y}, z{z} {}

    void print() const
    {
        printf("< %g, %g, %g >", x, y, z);
    }
};

struct QAngle {
    float x, y, z;

    QAngle() : x{NAN}, y{NAN}, z{NAN} {}
    QAngle(float x, float y, float z) : x{x}, y{y}, z{z} {}

    void print() const
    {
        printf("< %g, %g, %g >", x, y, z);
    }
};

struct matrix3x4_t {
    float m_flMatVal[3][4];

    matrix3x4_t() : m_flMatVal{NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN} {}

    void print() const
    {
        auto a = m_flMatVal;
        // clang-format off
        printf("%g, %g, %g, %g\n%g, %g, %g, %g\n%g, %g, %g, %g",
               a[0][0], a[0][1], a[0][2], a[0][3],
               a[1][0], a[1][1], a[1][2], a[1][3],
               a[2][0], a[2][1], a[2][2], a[2][3]);
        // clang-format on
    }
};


void SinCos(float rad, float* s, float* c);
extern "C" void AngleMatrix(const QAngle* angles, matrix3x4_t* matrix);
void AngleMatrix(const QAngle* angles, const Vector* position, matrix3x4_t* matrix);
