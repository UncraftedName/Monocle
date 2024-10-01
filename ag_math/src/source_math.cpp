#include "source_math.hpp"

void SinCos(float rad, float* s, float* c)
{
    _asm
    {
        fld DWORD PTR [rad]
        fsincos
        mov edx, DWORD PTR [c]
        mov eax, DWORD PTR [s]
        fstp DWORD PTR [edx]
        fstp DWORD PTR [eax]
    }
}
