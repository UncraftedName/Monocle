#include <iostream>
#include "source_math.hpp"

int main()
{
    QAngle q{0, 0, 0};
    matrix3x4_t m;
    AngleMatrix(&q, &m);
    m.print();
}
