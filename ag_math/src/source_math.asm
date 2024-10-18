.686
.model flat, C

Vector STRUCT
    x DWORD ?
    y DWORD ?
    z DWORD ?
Vector ENDS

VPlane STRUCT
    n Vector <>
    d DWORD ?
VPlane ENDS

.const

DEG2RAD_MUL_F REAL4 0.017453292 ; float(PI/180)
NEG1_F REAL4 -1.0

.code

; Some functions that I'm copying are __thiscall but MSVC doesn't allow declaring functions as
; __thiscall, so the usual trick around this is to declare it as __fastcall(this, dummy_edx, ...).
; I couldn't figure out how to link to the assembly file when declaring the function as __fastcall
; though - seems like MSVC still wrangles the name? And using the wrangled named didn't work. The
; workaround is to declare all the functions as __cdecl, then add a little preamble & postamble to
; fixup the calling convention differences.

; put at the very start of the function
ThiscallToCdeclPreamble MACRO
    POP  EDX  ; return address -> edx
    POP  ECX  ; last param on stack -> this
    PUSH EDX  ; edx -> return address
ENDM

; replace any RET instructions with this
ThiscallToCdeclPostamble MACRO
    POP  EDX  ; return address -> edx
    PUSH ECX  ; this -> last param on stack
    PUSH EDX  ; edx -> return address
    RET       ; caller cleans up stack
ENDM

; void AngleMatrix(const QAngle* angles, matrix3x4_t* matrix) : server.dll[0x451670]
AngleMatrix PROC PUBLIC
    SUB ESP, 20h
    LEA ECX, [ESP + 8]
    MOV [ESP + 14h], ECX
    MOV ECX, [ESP + 24h]
    FLD dword ptr [ECX + 4]
    LEA EAX, [ESP + 4]
    FMUL dword ptr DS:[DEG2RAD_MUL_F]
    MOV [ESP + 18h], EAX
    FSTP dword ptr [ESP + 1Ch]
    FLD dword ptr [ESP + 1Ch]
    FSINCOS
    MOV EDX, [ESP + 18h]
    MOV EAX, [ESP + 14h]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    FLD dword ptr [ECX]
    LEA EDX, [ESP]
    FMUL dword ptr DS:[DEG2RAD_MUL_F]
    LEA EAX, [ESP + 0Ch]
    MOV [ESP + 18h], EDX
    MOV [ESP + 1Ch], EAX
    FSTP dword ptr [ESP + 14h]
    FLD dword ptr [ESP + 14h]
    FSINCOS
    MOV EDX, [ESP + 18h]
    MOV EAX, [ESP + 1Ch]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    FLD dword ptr [ECX + 8]
    LEA EDX, [ESP + 10h]
    FMUL dword ptr DS:[DEG2RAD_MUL_F]
    LEA EAX, [ESP + 24h]
    MOV [ESP + 18h], EDX
    MOV [ESP + 1Ch], EAX
    FSTP dword ptr [ESP + 14h]
    FLD dword ptr [ESP + 14h]
    FSINCOS
    MOV EDX, [ESP + 18h]
    MOV EAX, [ESP + 1Ch]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    MOV EAX, [ESP + 28h]
    FLD dword ptr [ESP]
    FLD ST(0)
    FLD dword ptr [ESP + 4]
    FMUL ST(1), ST
    FXCH
    FSTP dword ptr [EAX]
    FLD ST(1)
    FLD dword ptr [ESP + 8]
    FMUL ST(1), ST
    FXCH
    FSTP dword ptr [EAX + 10h]
    FLD dword ptr [ESP + 0Ch]
    FLD ST(0)
    FCHS
    FSTP dword ptr [EAX + 20h]
    FLD dword ptr [ESP + 10h]
    FLD ST(0)
    FMUL ST, ST(4)
    FLD ST(1)
    FMUL ST, ST(4)
    FLD dword ptr [ESP + 24h]
    FMULP ST(6), ST
    FLD dword ptr [ESP + 24h]
    FMULP ST(5), ST
    FLD ST(5)
    FMUL ST, ST(4)
    FSUB ST, ST(1)
    FSTP dword ptr [EAX + 4]
    FLD ST(4)
    FMUL ST, ST(4)
    FADD ST, ST(2)
    FSTP dword ptr [EAX + 14h]
    FLD dword ptr [ESP + 24h]
    FMUL ST, ST(7)
    FSTP dword ptr [EAX + 24h]
    FXCH
    FMUL ST, ST(3)
    FADDP ST(4), ST
    FXCH ST(3)
    FSTP dword ptr [EAX + 8]
    FXCH ST(2)
    FMULP
    FSUBRP ST(2), ST
    FXCH
    FSTP dword ptr [EAX + 18h]
    FMULP
    FSTP dword ptr [EAX + 28h]
    FLDZ
    FST dword ptr [EAX + 0Ch]
    FST dword ptr [EAX + 1Ch]
    FSTP dword ptr [EAX + 2Ch]
    ADD ESP, 20h
    RET
AngleMatrix ENDP

; AngleVectors(const QAngle* angles, Vector* f, Vector* r, Vector* u) : server.dll[0x451130]
AngleVectors PROC PUBLIC
    SUB ESP, 20h
    LEA ECX, [ESP + 4]
    MOV [ESP + 14h], ECX
    MOV ECX, [ESP + 24h]
    FLD dword ptr [ECX + 4]
    LEA EAX, [ESP]
    FMUL dword ptr DS:[DEG2RAD_MUL_F]
    MOV [ESP + 18h], EAX
    FSTP dword ptr [ESP + 1Ch]
    FLD dword ptr [ESP + 1Ch]
    FSINCOS
    MOV EDX, [ESP + 18h]
    MOV EAX, [ESP + 14h]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    FLD dword ptr [ECX]
    LEA EDX, [ESP + 8]
    FMUL dword ptr DS:[DEG2RAD_MUL_F]
    LEA EAX, [ESP + 24h]
    MOV [ESP + 18h], EDX
    MOV [ESP + 1Ch], EAX
    FSTP dword ptr [ESP + 14h]
    FLD dword ptr [ESP + 14h]
    FSINCOS
    MOV EDX, [ESP + 18h]
    MOV EAX, [ESP + 1Ch]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    FLD dword ptr [ECX + 8]
    LEA EDX, [ESP + 0Ch]
    FMUL dword ptr DS:[DEG2RAD_MUL_F]
    LEA EAX, [ESP + 10h]
    MOV [ESP + 18h], EDX
    MOV [ESP + 1Ch], EAX
    FSTP dword ptr [ESP + 14h]
    FLD dword ptr [ESP + 14h]
    FSINCOS
    MOV EDX, [ESP + 18h]
    MOV EAX, [ESP + 1Ch]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    MOV EAX, [ESP + 28h]
    FLD dword ptr [ESP]
    TEST EAX, EAX
    FLD dword ptr [ESP + 4]
    FLD dword ptr [ESP + 8]
    FLD dword ptr [ESP + 24h]
    JE short no_f
    FLD ST(1)
    FMUL ST, ST(4)
    FSTP dword ptr [EAX]
    FLD ST(1)
    FMUL ST, ST(3)
    FSTP dword ptr [EAX + 4]
    FLD ST(0)
    FCHS
    FSTP dword ptr [EAX + 8]
no_f:
    MOV EAX, [ESP + 2Ch]
    FLD dword ptr [ESP + 0Ch]
    TEST EAX, EAX
    FLD dword ptr [ESP + 10h]
    JE short no_r
    FLD ST(0)
    FMULP ST(3), ST
    FLD ST(1)
    FMUL ST, ST(5)
    FLD ST(3)
    FMUL ST, ST(7)
    FSUBP
    FSTP dword ptr [EAX]
    FLD ST(1)
    FMUL ST, ST(6)
    FLD dword ptr DS:[NEG1_F]
    FMUL ST(1), ST
    FXCH ST(4)
    FMUL ST, ST(6)
    FSUBP
    FSTP dword ptr [EAX + 4]
    FLD ST(0)
    FMUL ST, ST(4)
    FMULP ST(3), ST
    FXCH ST(2)
    FSTP dword ptr [EAX + 8]
    FLD dword ptr [ESP + 24h]
    FXCH ST(2)
no_r:
    MOV EAX, [ESP + 30h]
    TEST EAX, EAX
    JE short no_u
    FLD ST(1)
    FMULP ST(3), ST
    FLD ST(2)
    FMUL ST, ST(6)
    FLD ST(1)
    FMUL ST, ST(6)
    FADDP
    FSTP dword ptr [EAX]
    FXCH ST(2)
    FMULP ST(4), ST
    FXCH
    FMULP ST(4), ST
    FXCH ST(2)
    FSUBRP ST(3), ST
    FXCH ST(2)
    FSTP dword ptr [EAX + 4]
    FMULP
    FSTP dword ptr [EAX + 8]
    ADD ESP, 20h
    RET
no_u:
    FSTP ST(5)
    FSTP ST(3)
    FSTP ST(2)
    FSTP ST(2)
    FSTP ST(0)
    FSTP ST(0)
    ADD ESP, 20h
    RET
AngleVectors ENDP

; MatrixInverseTR(const VMatrix* src, VMatrix* dst) : server.dll[0x4558e0]
MatrixInverseTR PROC PUBLIC
    SUB ESP, 18h
    MOV EAX, [ESP + 1Ch]
    FLD dword ptr [EAX]
    PUSH ESI
    MOV ESI, [ESP + 24h]
    FSTP dword ptr [ESI]
    LEA ECX, [ESP + 4]
    FLD dword ptr [EAX + 10h]
    FSTP dword ptr [ESI + 4]
    FLD dword ptr [EAX + 20h]
    FSTP dword ptr [ESI + 8]
    FLD dword ptr [EAX + 4]
    FSTP dword ptr [ESI + 10h]
    FLD dword ptr [EAX + 14h]
    FSTP dword ptr [ESI + 14h]
    FLD dword ptr [EAX + 24h]
    FSTP dword ptr [ESI + 18h]
    FLD dword ptr [EAX + 8]
    FSTP dword ptr [ESI + 20h]
    FLD dword ptr [EAX + 18h]
    FSTP dword ptr [ESI + 24h]
    FLD dword ptr [EAX + 28h]
    FSTP dword ptr [ESI + 28h]
    FLD dword ptr [EAX + 0Ch]
    FCHS
    FSTP dword ptr [ESP + 4]
    FLD dword ptr [EAX + 1Ch]
    FCHS
    FSTP dword ptr [ESP + 8]
    FLD dword ptr [EAX + 2Ch]
    LEA EAX, [ESP + 10h]
    PUSH EAX
    FCHS
    PUSH ECX
    FSTP dword ptr [ESP + 14h]
    PUSH ESI
    CALL Vector3DMultiply
    FLD dword ptr [ESP + 1Ch]
    FSTP dword ptr [ESI + 0Ch]
    ADD ESP, 0Ch
    FLD dword ptr [ESP + 14h]
    FSTP dword ptr [ESI + 1Ch]
    FLD dword ptr [ESP + 18h]
    FSTP dword ptr [ESI + 2Ch]
    FLDZ
    FST dword ptr [ESI + 38h]
    FST dword ptr [ESI + 34h]
    FSTP dword ptr [ESI + 30h]
    FLD1
    FSTP dword ptr [ESI + 3Ch]
    POP ESI
    ADD ESP, 18h
    RET
MatrixInverseTR ENDP

; Vector3DMultiply(const VMatrix* src1, const Vector* src2, Vector* dst) : server.dll[0x455780]
Vector3DMultiply PROC PUBLIC
    MOV EAX, [ESP + 8]
    MOV ECX, [ESP + 0Ch]
    SUB ESP, 18h
    CMP EAX, ECX
    JNE short src_not_dst
    MOV EAX, [ESP + 0Ch]
    MOV EDX, [ESP + 10h]
    MOV [ESP + 0Ch], EAX
    MOV EAX, [ESP + 14h]
    MOV [ESP + 14h], EAX
    MOV [ESP + 10h], EDX
    LEA EAX, [ESP + 0Ch]
src_not_dst:
    MOV EDX, [EAX]
    MOV [ESP], EDX
    MOV EDX, [EAX + 4]
    MOV EAX, [EAX + 8]
    MOV [ESP + 8], EAX
    MOV EAX, [ESP + 1Ch]
    FLD dword ptr [EAX + 4]
    MOV [ESP + 4], EDX
    FLD dword ptr [ESP + 4]
    FMUL ST(1), ST
    FLD dword ptr [EAX + 8]
    FLD dword ptr [ESP + 8]
    FMUL ST(1), ST
    FXCH ST(3)
    FADDP
    FLD dword ptr [EAX]
    FLD dword ptr [ESP]
    FMUL ST(1), ST
    FXCH ST(2)
    FADDP
    FSTP dword ptr [ECX]
    FLD dword ptr [EAX + 10h]
    FMUL ST, ST(1)
    FLD dword ptr [EAX + 14h]
    FMUL ST, ST(3)
    FADDP
    FLD dword ptr [EAX + 18h]
    FMUL ST, ST(4)
    FADDP
    FSTP dword ptr [ECX + 4]
    FMUL dword ptr [EAX + 20h]
    FLD dword ptr [EAX + 24h]
    FMULP ST(2), ST
    FADDP
    FLD dword ptr [EAX + 28h]
    FMULP ST(2), ST
    FADDP
    FSTP dword ptr [ECX + 8]
    ADD ESP, 18h
    RET
Vector3DMultiply ENDP

; __cdecl VMatrix__MatrixMul(const VMatrix* lhs, const VMatrix* rhs, VMatrix* out)
; This is our implementation of VMatrix::MatrixMul : server.dll[0x454e30],
; called by VMatrix::operator* : server.dll[0x4550a0].
VMatrix__MatrixMul PROC PUBLIC
    ThiscallToCdeclPreamble

    SUB ESP, 20h
    MOV EAX, [ESP + 24h]
    FLD dword ptr [EAX + 24h]
    FMUL dword ptr [ECX + 8]
    FLD dword ptr [EAX + 14h]
    FMUL dword ptr [ECX + 4]
    FADDP
    FLD dword ptr [ECX + 0Ch]
    FMUL dword ptr [EAX + 34h]
    FADDP
    FLD dword ptr [EAX + 4]
    FMUL dword ptr [ECX]
    FADDP
    FLD dword ptr [EAX + 18h]
    FMUL dword ptr [ECX + 4]
    FLD dword ptr [EAX + 28h]
    FMUL dword ptr [ECX + 8]
    FADDP
    FLD dword ptr [ECX]
    FMUL dword ptr [EAX + 8]
    FADDP
    FLD dword ptr [EAX + 38h]
    FMUL dword ptr [ECX + 0Ch]
    FADDP
    FLD dword ptr [EAX + 3Ch]
    FMUL dword ptr [ECX + 0Ch]
    FLD dword ptr [EAX + 1Ch]
    FMUL dword ptr [ECX + 4]
    FADDP
    FLD dword ptr [EAX + 2Ch]
    FMUL dword ptr [ECX + 8]
    FADDP
    FLD dword ptr [EAX + 0Ch]
    FMUL dword ptr [ECX]
    FADDP
    FLD dword ptr [EAX + 20h]
    FMUL dword ptr [ECX + 18h]
    FLD dword ptr [EAX + 10h]
    FMUL dword ptr [ECX + 14h]
    FADDP
    FLD dword ptr [ECX + 10h]
    FMUL dword ptr [EAX]
    FADDP
    FLD dword ptr [ECX + 1Ch]
    FMUL dword ptr [EAX + 30h]
    FADDP
    FLD dword ptr [EAX + 24h]
    FMUL dword ptr [ECX + 18h]
    FLD dword ptr [EAX + 14h]
    FMUL dword ptr [ECX + 14h]
    FADDP
    FLD dword ptr [ECX + 1Ch]
    FMUL dword ptr [EAX + 34h]
    FADDP
    FLD dword ptr [EAX + 4]
    FMUL dword ptr [ECX + 10h]
    FADDP
    FLD dword ptr [EAX + 18h]
    FMUL dword ptr [ECX + 14h]
    FLD dword ptr [ECX + 10h]
    FMUL dword ptr [EAX + 8]
    FADDP
    FLD dword ptr [EAX + 28h]
    FMUL dword ptr [ECX + 18h]
    FADDP
    FLD dword ptr [EAX + 38h]
    FMUL dword ptr [ECX + 1Ch]
    FADDP
    FLD dword ptr [EAX + 3Ch]
    FMUL dword ptr [ECX + 1Ch]
    FLD dword ptr [EAX + 1Ch]
    FMUL dword ptr [ECX + 14h]
    FADDP
    FLD dword ptr [EAX + 2Ch]
    FMUL dword ptr [ECX + 18h]
    FADDP
    FLD dword ptr [EAX + 0Ch]
    FMUL dword ptr [ECX + 10h]
    FADDP
    FSTP dword ptr [ESP + 24h]
    FLD dword ptr [EAX]
    FMUL dword ptr [ECX + 20h]
    FLD dword ptr [EAX + 10h]
    FMUL dword ptr [ECX + 24h]
    FADDP
    FLD dword ptr [ECX + 2Ch]
    FMUL dword ptr [EAX + 30h]
    FADDP
    FLD dword ptr [EAX + 20h]
    FMUL dword ptr [ECX + 28h]
    FADDP
    FSTP dword ptr [ESP]
    FLD dword ptr [EAX + 14h]
    FMUL dword ptr [ECX + 24h]
    FLD dword ptr [ECX + 2Ch]
    FMUL dword ptr [EAX + 34h]
    FADDP
    FLD dword ptr [EAX + 24h]
    FMUL dword ptr [ECX + 28h]
    FADDP
    FLD dword ptr [EAX + 4]
    FMUL dword ptr [ECX + 20h]
    FADDP
    FSTP dword ptr [ESP + 4]
    FLD dword ptr [EAX + 18h]
    FMUL dword ptr [ECX + 24h]
    FLD dword ptr [ECX + 2Ch]
    FMUL dword ptr [EAX + 38h]
    FADDP
    FLD dword ptr [ECX + 28h]
    FMUL dword ptr [EAX + 28h]
    FADDP
    FLD dword ptr [ECX + 20h]
    FMUL dword ptr [EAX + 8]
    FADDP
    FSTP dword ptr [ESP + 8]
    FLD dword ptr [EAX + 1Ch]
    FMUL dword ptr [ECX + 24h]
    FLD dword ptr [ECX + 2Ch]
    FMUL dword ptr [EAX + 3Ch]
    FADDP
    FLD dword ptr [ECX + 28h]
    FMUL dword ptr [EAX + 2Ch]
    FADDP
    FLD dword ptr [EAX + 0Ch]
    FMUL dword ptr [ECX + 20h]
    FADDP
    FSTP dword ptr [ESP + 0Ch]
    FLD dword ptr [EAX]
    FMUL dword ptr [ECX + 30h]
    FLD dword ptr [ECX + 3Ch]
    FMUL dword ptr [EAX + 30h]
    FADDP
    FLD dword ptr [EAX + 10h]
    FMUL dword ptr [ECX + 34h]
    FADDP
    FLD dword ptr [EAX + 20h]
    FMUL dword ptr [ECX + 38h]
    FADDP
    FSTP dword ptr [ESP + 10h]
    FLD dword ptr [ECX + 3Ch]
    FMUL dword ptr [EAX + 34h]
    FLD dword ptr [EAX + 14h]
    FMUL dword ptr [ECX + 34h]
    FADDP
    FLD dword ptr [EAX + 4]
    FMUL dword ptr [ECX + 30h]
    FADDP
    FLD dword ptr [EAX + 24h]
    FMUL dword ptr [ECX + 38h]
    FADDP
    FSTP dword ptr [ESP + 14h]
    FLD dword ptr [EAX + 18h]
    FMUL dword ptr [ECX + 34h]
    FLD dword ptr [ECX + 3Ch]
    FMUL dword ptr [EAX + 38h]
    FADDP
    FLD dword ptr [ECX + 38h]
    FMUL dword ptr [EAX + 28h]
    FADDP
    FLD dword ptr [ECX + 30h]
    FMUL dword ptr [EAX + 8]
    FADDP
    FSTP dword ptr [ESP + 18h]
    FLD dword ptr [EAX + 1Ch]
    FMUL dword ptr [ECX + 34h]
    FLD dword ptr [ECX + 3Ch]
    FMUL dword ptr [EAX + 3Ch]
    FADDP
    FLD dword ptr [ECX + 38h]
    FMUL dword ptr [EAX + 2Ch]
    FADDP
    FLD dword ptr [EAX + 0Ch]
    FMUL dword ptr [ECX + 30h]
    FADDP
    FSTP dword ptr [ESP + 1Ch]
    FLD dword ptr [EAX + 20h]
    FMUL dword ptr [ECX + 8]
    FLD dword ptr [EAX + 10h]
    FMUL dword ptr [ECX + 4]
    FADDP
    FLD dword ptr [EAX]
    FMUL dword ptr [ECX]
    FADDP
    FLD dword ptr [ECX + 0Ch]
    FMUL dword ptr [EAX + 30h]
    MOV EAX, [ESP + 28h]
    FADDP
    FSTP dword ptr [EAX]
    FXCH ST(5)
    FSTP dword ptr [EAX + 4]
    FXCH ST(3)
    FSTP dword ptr [EAX + 8]
    FXCH
    FSTP dword ptr [EAX + 0Ch]
    FSTP dword ptr [EAX + 10h]
    FSTP dword ptr [EAX + 14h]
    FSTP dword ptr [EAX + 18h]
    FLD dword ptr [ESP + 24h]
    FSTP dword ptr [EAX + 1Ch]
    FLD dword ptr [ESP]
    FSTP dword ptr [EAX + 20h]
    FLD dword ptr [ESP + 4]
    FSTP dword ptr [EAX + 24h]
    FLD dword ptr [ESP + 8]
    FSTP dword ptr [EAX + 28h]
    FLD dword ptr [ESP + 0Ch]
    FSTP dword ptr [EAX + 2Ch]
    FLD dword ptr [ESP + 10h]
    FSTP dword ptr [EAX + 30h]
    FLD dword ptr [ESP + 14h]
    FSTP dword ptr [EAX + 34h]
    FLD dword ptr [ESP + 18h]
    FSTP dword ptr [EAX + 38h]
    FLD dword ptr [ESP + 1Ch]
    FSTP dword ptr [EAX + 3Ch]
    ADD ESP, 20h

    ThiscallToCdeclPostamble
VMatrix__MatrixMul ENDP

; Vector VMatrix::operator*(Vector) : server.dll[0x3fe020]
; Same hack as above - function is actually __thiscall but we implement:
; Vector* __cdecl VMatrix__operatorVec(const VMatrix* lhs, Vector* out, const Vector* vVec)
VMatrix__operatorVec PROC PUBLIC
    ThiscallToCdeclPreamble

    MOV EDX, [ESP + 8]
    FLD dword ptr [ECX + 4]
    FMUL dword ptr [EDX + 4]
    MOV EAX, [ESP + 4]
    FLD dword ptr [ECX + 8]
    FMUL dword ptr [EDX + 8]
    FADDP
    FLD dword ptr [EDX]
    FMUL dword ptr [ECX]
    FADDP
    FADD dword ptr [ECX + 0Ch]
    FSTP dword ptr [EAX]
    FLD dword ptr [ECX + 14h]
    FMUL dword ptr [EDX + 4]
    FLD dword ptr [ECX + 10h]
    FMUL dword ptr [EDX]
    FADDP
    FLD dword ptr [ECX + 18h]
    FMUL dword ptr [EDX + 8]
    FADDP
    FADD dword ptr [ECX + 1Ch]
    FSTP dword ptr [EAX + 4]
    FLD dword ptr [ECX + 24h]
    FMUL dword ptr [EDX + 4]
    FLD dword ptr [ECX + 20h]
    FMUL dword ptr [EDX]
    FADDP
    FLD dword ptr [ECX + 28h]
    FMUL dword ptr [EDX + 8]
    FADDP
    FADD dword ptr [ECX + 2Ch]
    FSTP dword ptr [EAX + 8]

    ThiscallToCdeclPostamble
VMatrix__operatorVec ENDP

END
