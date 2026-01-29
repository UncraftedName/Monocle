INCLUDE math_interface.inc

.code

; void __cdecl MatrixInverseTR(const VMatrix* src, VMatrix* dst) : server.dll[0x494390]
_MonAsm_MatrixInverseTR_9862575 PROC PUBLIC
    PUSH EBP
    MOV EBP, ESP
    MOV ECX, [EBP + 0Ch]
    MOV EAX, [EBP + 8]
    MOVSS XMM0, dword ptr [SIGNMASK_16F]
    FLD dword ptr [EAX]
    FSTP dword ptr [ECX]
    FLD dword ptr [EAX + 10h]
    FSTP dword ptr [ECX + 4]
    FLD dword ptr [EAX + 20h]
    FSTP dword ptr [ECX + 8]
    FLD dword ptr [EAX + 4]
    FSTP dword ptr [ECX + 10h]
    FLD dword ptr [EAX + 14h]
    MOVSS XMM5, dword ptr [ECX]
    FSTP dword ptr [ECX + 14h]
    FLD dword ptr [EAX + 24h]
    FSTP dword ptr [ECX + 18h]
    FLD dword ptr [EAX + 8]
    FSTP dword ptr [ECX + 20h]
    FLD dword ptr [EAX + 18h]
    MOVSS XMM2, dword ptr [ECX + 14h]
    FSTP dword ptr [ECX + 24h]
    MOVSS XMM7, dword ptr [EAX + 28h]
    MOVSS dword ptr [ECX + 28h], XMM7
    MOVSS XMM3, dword ptr [EAX + 1Ch]
    MOVSS XMM6, dword ptr [EAX + 0Ch]
    XORPS XMM3, XMM0
    MOVSS XMM4, dword ptr [EAX + 2Ch]
    XORPS XMM6, XMM0
    XORPS XMM4, XMM0
    MOVSS XMM1, dword ptr [ECX + 24h]
    MOVAPS XMM0, XMM3
    MULSS XMM5, XMM6
    MOV dword ptr [ECX + 38h], 0
    MULSS XMM0, dword ptr [ECX + 4]
    MOV dword ptr [ECX + 34h], 0
    MULSS XMM2, XMM3
    MOV dword ptr [ECX + 30h], 0
    ADDSS XMM5, XMM0
    MULSS XMM1, XMM3
    MOV dword ptr [ECX + 3Ch], 3F800000h
    MOVSS XMM0, dword ptr [ECX + 8]
    MULSS XMM0, XMM4
    MULSS XMM7, XMM4
    ADDSS XMM5, XMM0
    MOVSS XMM0, dword ptr [ECX + 10h]
    MULSS XMM0, XMM6
    ADDSS XMM2, XMM0
    MOVSS dword ptr [ECX + 0Ch], XMM5
    MOVAPS XMM0, XMM4
    MULSS XMM0, dword ptr [ECX + 18h]
    ADDSS XMM2, XMM0
    MOVSS XMM0, dword ptr [ECX + 20h]
    MULSS XMM0, XMM6
    ADDSS XMM1, XMM0
    MOVSS dword ptr [ECX + 1Ch], XMM2
    ADDSS XMM1, XMM7
    MOVSS dword ptr [ECX + 2Ch], XMM1
    POP EBP
    RET
_MonAsm_MatrixInverseTR_9862575 ENDP

; void __thiscall VMatrix::MatrixMul(VMatrix *this, VMatrix *vm, VMatrix *out) : server.dll[0x494480]
@MonAsm_MatrixMul_9862575@16 PROC PUBLIC
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 38h
    MOV EAX, [EBP + 8]
    MOVSS XMM6, dword ptr [ECX + 4]
    MOVSS XMM2, dword ptr [ECX]
    MOVAPS XMM0, XMM6
    MOVSS XMM4, dword ptr [ECX + 8]
    MOVAPS XMM1, XMM2
    MULSS XMM0, dword ptr [EAX + 14h]
    MULSS XMM1, dword ptr [EAX + 4]
    MOVSS XMM3, dword ptr [ECX + 0Ch]
    MOVSS dword ptr [EBP - 0Ch], XMM4
    ADDSS XMM1, XMM0
    MOVSS dword ptr [EBP - 10h], XMM3
    MOVAPS XMM0, XMM4
    MOVSS dword ptr [EBP - 8], XMM6
    MULSS XMM0, dword ptr [EAX + 24h]
    ADDSS XMM1, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 34h]
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EAX + 18h]
    MULSS XMM0, XMM6
    MOVSS dword ptr [EBP - 14h], XMM1
    MOVSS XMM1, dword ptr [EAX + 8]
    MULSS XMM1, XMM2
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EAX + 28h]
    MULSS XMM0, XMM4
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EAX + 38h]
    MULSS XMM0, XMM3
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EAX + 1Ch]
    MULSS XMM0, XMM6
    MOVSS dword ptr [EBP - 18h], XMM1
    MOVSS XMM1, dword ptr [EAX + 0Ch]
    MULSS XMM1, XMM2
    MOVSS XMM2, dword ptr [ECX + 18h]
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EAX + 2Ch]
    MULSS XMM0, XMM4
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EAX + 3Ch]
    MULSS XMM0, XMM3
    MOVSS XMM3, dword ptr [ECX + 14h]
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [ECX + 10h]
    MOVSS dword ptr [EBP + 8], XMM0
    MOVSS XMM0, dword ptr [EAX]
    MOVAPS XMM4, XMM0
    MOVSS dword ptr [EBP - 4], XMM0
    MULSS XMM4, dword ptr [EBP + 8]
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 10h]
    MOVSS dword ptr [EBP - 1Ch], XMM1
    MOVSS XMM1, dword ptr [ECX + 1Ch]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 20h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM1
    MULSS XMM0, dword ptr [EAX + 30h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 14h]
    MOVSS dword ptr [EBP - 20h], XMM4
    MOVSS XMM4, dword ptr [EBP + 8]
    MULSS XMM4, dword ptr [EAX + 4]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 24h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM1
    MULSS XMM0, dword ptr [EAX + 34h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 18h]
    MOVSS dword ptr [EBP - 24h], XMM4
    MOVSS XMM4, dword ptr [EBP + 8]
    MULSS XMM4, dword ptr [EAX + 8]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 28h]
    MULSS XMM3, dword ptr [EAX + 1Ch]
    ADDSS XMM4, XMM0
    MULSS XMM2, dword ptr [EAX + 2Ch]
    MOVAPS XMM0, XMM1
    MOVSS XMM5, dword ptr [ECX + 20h]
    MULSS XMM0, dword ptr [EAX + 38h]
    MULSS XMM1, dword ptr [EAX + 3Ch]
    ADDSS XMM4, XMM0
    MOVSS XMM7, dword ptr [ECX + 30h]
    MOVSS XMM0, dword ptr [EBP + 8]
    MOVAPS XMM6, XMM7
    MULSS XMM0, dword ptr [EAX + 0Ch]
    MULSS XMM6, dword ptr [EBP - 4]
    ADDSS XMM0, XMM3
    MOVSS dword ptr [EBP - 28h], XMM4
    MOVSS XMM3, dword ptr [ECX + 2Ch]
    MOVAPS XMM4, XMM5
    MULSS XMM4, dword ptr [EBP - 4]
    ADDSS XMM0, XMM2
    MOVSS XMM2, dword ptr [ECX + 24h]
    ADDSS XMM0, XMM1
    MOVSS XMM1, dword ptr [ECX + 28h]
    MOVSS dword ptr [EBP + 8], XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 10h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM1
    MULSS XMM0, dword ptr [EAX + 20h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 30h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 14h]
    MOVSS dword ptr [EBP - 2Ch], XMM4
    MOVAPS XMM4, XMM5
    MULSS XMM4, dword ptr [EAX + 4]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM1
    MULSS XMM0, dword ptr [EAX + 24h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 34h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 18h]
    MULSS XMM2, dword ptr [EAX + 1Ch]
    MOVSS dword ptr [EBP - 30h], XMM4
    MOVAPS XMM4, XMM5
    MULSS XMM4, dword ptr [EAX + 8]
    MULSS XMM5, dword ptr [EAX + 0Ch]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM1
    MULSS XMM0, dword ptr [EAX + 28h]
    MULSS XMM1, dword ptr [EAX + 2Ch]
    ADDSS XMM5, XMM2
    MOVSS XMM2, dword ptr [ECX + 34h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM3, dword ptr [EAX + 3Ch]
    MULSS XMM0, dword ptr [EAX + 38h]
    ADDSS XMM5, XMM1
    MOVSS XMM1, dword ptr [ECX + 38h]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 10h]
    ADDSS XMM5, XMM3
    MOVSS XMM3, dword ptr [ECX + 3Ch]
    ADDSS XMM6, XMM0
    MOVSS dword ptr [EBP - 34h], XMM4
    MOVAPS XMM0, XMM1
    MOVSS dword ptr [EBP - 38h], XMM5
    MULSS XMM0, dword ptr [EAX + 20h]
    MOVAPS XMM5, XMM7
    MULSS XMM5, dword ptr [EAX + 4]
    ADDSS XMM6, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 30h]
    ADDSS XMM6, XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 14h]
    MOVAPS XMM4, XMM7
    MULSS XMM4, dword ptr [EAX + 8]
    ADDSS XMM5, XMM0
    MULSS XMM7, dword ptr [EAX + 0Ch]
    MOVAPS XMM0, XMM1
    MULSS XMM0, dword ptr [EAX + 24h]
    ADDSS XMM5, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 34h]
    ADDSS XMM5, XMM0
    MOVAPS XMM0, XMM2
    MULSS XMM0, dword ptr [EAX + 18h]
    MULSS XMM2, dword ptr [EAX + 1Ch]
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM1
    MULSS XMM0, dword ptr [EAX + 28h]
    MULSS XMM1, dword ptr [EAX + 2Ch]
    ADDSS XMM7, XMM2
    ADDSS XMM4, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr [EAX + 38h]
    MULSS XMM3, dword ptr [EAX + 3Ch]
    ADDSS XMM7, XMM1
    MOVSS XMM1, dword ptr [EBP - 4]
    ADDSS XMM4, XMM0
    MOVSS XMM0, dword ptr [EAX + 10h]
    MULSS XMM1, dword ptr [ECX]
    MULSS XMM0, dword ptr [EBP - 8]
    ADDSS XMM7, XMM3
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EAX + 20h]
    MULSS XMM0, dword ptr [EBP - 0Ch]
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EAX + 30h]
    MOV EAX, [EBP + 0Ch]
    MULSS XMM0, dword ptr [EBP - 10h]
    MOVSS dword ptr [EAX + 30h], XMM6
    ADDSS XMM1, XMM0
    MOVSS dword ptr [EAX + 34h], XMM5
    MOVSS XMM0, dword ptr [EBP - 14h]
    MOVSS dword ptr [EAX + 4], XMM0
    MOVSS XMM0, dword ptr [EBP - 18h]
    MOVSS dword ptr [EAX + 8], XMM0
    MOVSS XMM0, dword ptr [EBP - 1Ch]
    MOVSS dword ptr [EAX + 0Ch], XMM0
    MOVSS XMM0, dword ptr [EBP - 20h]
    MOVSS dword ptr [EAX + 10h], XMM0
    MOVSS XMM0, dword ptr [EBP - 24h]
    MOVSS dword ptr [EAX + 14h], XMM0
    MOVSS XMM0, dword ptr [EBP - 28h]
    MOVSS dword ptr [EAX + 18h], XMM0
    MOVSS XMM0, dword ptr [EBP + 8]
    MOVSS dword ptr [EAX + 1Ch], XMM0
    MOVSS XMM0, dword ptr [EBP - 2Ch]
    MOVSS dword ptr [EAX + 20h], XMM0
    MOVSS XMM0, dword ptr [EBP - 30h]
    MOVSS dword ptr [EAX + 24h], XMM0
    MOVSS XMM0, dword ptr [EBP - 34h]
    MOVSS dword ptr [EAX + 28h], XMM0
    MOVSS XMM0, dword ptr [EBP - 38h]
    MOVSS dword ptr [EAX], XMM1
    MOVSS dword ptr [EAX + 2Ch], XMM0
    MOVSS dword ptr [EAX + 38h], XMM4
    MOVSS dword ptr [EAX + 3Ch], XMM7
    MOV ESP, EBP
    POP EBP
    RET 8
@MonAsm_MatrixMul_9862575@16 ENDP

; void __cdecl AngleMatrix(const QAngle* angles, matrix3x4_t* matrix) : server.dll[0x48d990]
_MonAsm_AngleMatrix_9862575 PROC PUBLIC
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 20h
    MOV ECX, [EBP + 8]
    LEA EAX, [EBP - 14h]
    MOVSS XMM1, dword ptr DS:[DEG2RAD_MUL_F]
    MOV [EBP - 0Ch], EAX
    LEA EAX, [EBP - 18h]
    MOV [EBP - 10h], EAX
    MOVSS XMM0, dword ptr [ECX + 4]
    MULSS XMM0, XMM1
    MOVSS dword ptr [EBP - 8], XMM0
    FLD dword ptr [EBP - 8]
    FSINCOS
    MOV EDX, [EBP - 0Ch]
    MOV EAX, [EBP - 10h]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    MOVSS XMM0, dword ptr [ECX]
    LEA EAX, [EBP + 8]
    MULSS XMM0, XMM1
    MOV [EBP - 0Ch], EAX
    LEA EAX, [EBP - 1Ch]
    MOV [EBP - 8], EAX
    MOVSS dword ptr [EBP - 10h], XMM0
    FLD dword ptr [EBP - 10h]
    FSINCOS
    MOV EDX, [EBP - 0Ch]
    MOV EAX, [EBP - 8]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    MOVSS XMM0, dword ptr [ECX + 8]
    LEA EAX, [EBP - 4]
    MULSS XMM0, XMM1
    MOV [EBP - 0Ch], EAX
    LEA EAX, [EBP - 20h]
    MOV [EBP - 8], EAX
    MOVSS dword ptr [EBP - 10h], XMM0
    FLD dword ptr [EBP - 10h]
    FSINCOS
    MOV EDX, [EBP - 0Ch]
    MOV EAX, [EBP - 8]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    MOV EAX, [EBP + 0Ch]
    MOVSS XMM1, dword ptr [EBP - 14h]
    MOVSS XMM0, dword ptr [EBP + 8]
    MOVSS XMM3, dword ptr [EBP - 18h]
    MOVSS XMM7, dword ptr [EBP - 1Ch]
    MOVSS XMM2, dword ptr [EBP - 20h]
    MULSS XMM0, XMM1
    MOVAPS XMM4, XMM2
    MOV dword ptr [EAX + 0Ch], 0
    MOVSS XMM6, dword ptr [EBP - 4]
    MOVSS XMM5, dword ptr [EBP - 4]
    MOVSS dword ptr [EAX], XMM0
    MOVSS XMM0, dword ptr [EBP + 8]
    MULSS XMM0, XMM3
    MOV dword ptr [EAX + 1Ch], 0
    MULSS XMM4, XMM1
    MOV dword ptr [EAX + 2Ch], 0
    MOVSS dword ptr [EAX + 10h], XMM0
    MOVAPS XMM0, XMM7
    XORPS XMM0, xmmword ptr [SIGNMASK_16F]
    MOVSS dword ptr [EAX + 20h], XMM0
    MOVAPS XMM0, XMM4
    MULSS XMM0, XMM7
    MULSS XMM6, XMM3
    MULSS XMM5, XMM1
    MOVAPS XMM1, XMM2
    MULSS XMM2, dword ptr [EBP + 8]
    SUBSS XMM0, XMM6
    MULSS XMM1, XMM3
    MULSS XMM6, XMM7
    MOVSS dword ptr [EAX + 4], XMM0
    MOVAPS XMM0, XMM1
    MULSS XMM0, XMM7
    SUBSS XMM6, XMM4
    MOVSS dword ptr [EAX + 24h], XMM2
    ADDSS XMM0, XMM5
    MULSS XMM5, XMM7
    MOVSS dword ptr [EAX + 18h], XMM6
    ADDSS XMM5, XMM1
    MOVSS dword ptr [EAX + 14h], XMM0
    MOVSS XMM0, dword ptr [EBP - 4]
    MULSS XMM0, dword ptr [EBP + 8]
    MOVSS dword ptr [EAX + 8], XMM5
    MOVSS dword ptr [EAX + 28h], XMM0
    MOV ESP, EBP
    POP EBP
    RET
_MonAsm_AngleMatrix_9862575 ENDP

; void __cdecl AngleVectors(const QAngle* angles, Vector* f, Vector* r, Vector* u) : server.dll[0x48de80]
_MonAsm_AngleVectors_9862575 PROC PUBLIC
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 20h
    MOV ECX, [EBP + 8]
    LEA EAX, [EBP - 10h]
    MOVSS XMM1, dword ptr DS:[DEG2RAD_MUL_F]
    MOV [EBP - 8], EAX
    LEA EAX, [EBP - 14h]
    MOV [EBP - 0Ch], EAX
    MOVSS XMM0, dword ptr [ECX + 4]
    MULSS XMM0, XMM1
    MOVSS dword ptr [EBP - 4], XMM0
    FLD dword ptr [EBP - 4]
    FSINCOS
    MOV EDX, [EBP - 8]
    MOV EAX, [EBP - 0Ch]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    MOVSS XMM0, dword ptr [ECX]
    LEA EAX, [EBP - 18h]
    MULSS XMM0, XMM1
    MOV [EBP - 8], EAX
    LEA EAX, [EBP + 8]
    MOV [EBP - 4], EAX
    MOVSS dword ptr [EBP - 0Ch], XMM0
    FLD dword ptr [EBP - 0Ch]
    FSINCOS
    MOV EDX, [EBP - 8]
    MOV EAX, [EBP - 4]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    MOVSS XMM0, dword ptr [ECX + 8]
    LEA EAX, [EBP - 1Ch]
    MULSS XMM0, XMM1
    MOV [EBP - 8], EAX
    LEA EAX, [EBP - 20h]
    MOV [EBP - 4], EAX
    MOVSS dword ptr [EBP - 0Ch], XMM0
    FLD dword ptr [EBP - 0Ch]
    FSINCOS
    MOV EDX, [EBP - 8]
    MOV EAX, [EBP - 4]
    FSTP dword ptr [EDX]
    FSTP dword ptr [EAX]
    MOV EAX, [EBP + 0Ch]
    MOVSS XMM6, dword ptr [EBP - 10h]
    MOVSS XMM7, dword ptr [EBP - 14h]
    MOVSS XMM5, dword ptr [EBP - 18h]
    MOVSS XMM1, dword ptr [EBP + 8]
    TEST EAX, EAX
    JE short no_f
    MOVAPS XMM0, XMM5
    MULSS XMM0, XMM6
    MOVSS dword ptr [EAX], XMM0
    MOVAPS XMM0, XMM5
    MULSS XMM0, XMM7
    MOVSS dword ptr [EAX + 4], XMM0
    MOVAPS XMM0, XMM1
    XORPS XMM0, xmmword ptr [SIGNMASK_16F]
    MOVSS dword ptr [EAX + 8], XMM0
no_f:
    MOV EAX, [EBP + 10h]
    MOVSS XMM3, dword ptr [EBP - 1Ch]
    MOVSS XMM4, dword ptr [EBP - 20h]
    TEST EAX, EAX
    JE short no_r
    MOVAPS XMM2, XMM4
    MULSS XMM2, XMM1
    MOVAPS XMM1, XMM3
    MULSS XMM1, XMM7
    MOVAPS XMM0, XMM2
    MULSS XMM2, XMM7
    MULSS XMM0, XMM6
    SUBSS XMM1, XMM0
    MOVAPS XMM0, XMM3
    MULSS XMM0, dword ptr DS:[NEG1_F]
    MULSS XMM0, XMM6
    MOVSS dword ptr [EAX], XMM1
    MOVSS XMM1, dword ptr [EBP + 8]
    SUBSS XMM0, XMM2
    MOVSS dword ptr [EAX + 4], XMM0
    MOVAPS XMM0, XMM4
    MULSS XMM0, dword ptr DS:[NEG1_F]
    MULSS XMM0, XMM5
    MOVSS dword ptr [EAX + 8], XMM0
no_r:
    MOV EAX, [EBP + 14h]
    TEST EAX, EAX
    JE short no_u
    MOVAPS XMM2, XMM3
    MOVAPS XMM0, XMM4
    MULSS XMM2, XMM1
    MULSS XMM0, XMM7
    MOVAPS XMM1, XMM2
    MULSS XMM4, XMM6
    MULSS XMM1, XMM6
    MULSS XMM2, XMM7
    ADDSS XMM1, XMM0
    MULSS XMM3, XMM5
    SUBSS XMM2, XMM4
    MOVSS dword ptr [EAX + 8], XMM3
    MOVSS dword ptr [EAX], XMM1
    MOVSS dword ptr [EAX + 4], XMM2
no_u:
    MOV ESP, EBP
    POP EBP
    RET
_MonAsm_AngleVectors_9862575 ENDP

; Vector* __thiscall VMatrix::operator*(VMatrix *this,Vector *__return_storage_ptr__,Vector *vVec) : server.dll[0x45639d/0x456286]
;
; The first address is the entity teleport case and the second is the player case. The former is a bit easier to follow so that is
; what I used here. We do need to be very careful since the function is inlined in both cases and any differences could result in
; the wrong behavior. Associativity is not guaranteed e.g. ((x+y)+z) != (x+(y+z)) in general. I analyzed both cases and can confirm
; they are identical (except the player case has the additional "ptNewOrigin += ptOtherOrigin - ptOtherCenter;" stuffed in the middle.
@MonAsm_MatrixMulVector_9862575@16 PROC PUBLIC
    MOV EDX, [ESP + 8] ; in vec
    MOV EAX, [ESP + 4] ; out vec
    ; replaced:
    ; EAX with ECX
    ; [EBP - 5Ch] with EAX
    ; [EBP - 7Ch] with EDX
    MOVSS XMM4, dword ptr [EDX + Vector.x]
    MOVSS XMM5, dword ptr [EDX + Vector.y]
    MOVAPS XMM0, XMM4
    MOVSS XMM6, dword ptr [EDX + Vector.z]
    MULSS XMM0, dword ptr [ECX]
    MOVSS XMM3, dword ptr [ECX + 4]
    MOVSS XMM2, dword ptr [ECX + 14h]
    MULSS XMM3, XMM5
    MOVSS XMM1, dword ptr [ECX + 24h]
    MULSS XMM2, XMM5
    ADDSS XMM3, XMM0
    MULSS XMM1, XMM5
    MOVSS XMM0, dword ptr [ECX + 8]
    MULSS XMM0, XMM6
    ADDSS XMM3, XMM0
    MOVSS XMM0, dword ptr [ECX + 10h]
    MULSS XMM0, XMM4
    ADDSS XMM2, XMM0
    MOVSS XMM0, dword ptr [ECX + 18h]
    ADDSS XMM3, dword ptr [ECX + 0Ch]
    MULSS XMM0, XMM6
    ADDSS XMM2, XMM0
    MOVSS dword ptr [EAX + Vector.x], XMM3
    MOVSS XMM0, dword ptr [ECX + 20h]
    MULSS XMM0, XMM4
    ADDSS XMM2, dword ptr [ECX + 1Ch]
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [ECX + 28h]
    MULSS XMM0, XMM6
    MOVSS dword ptr [EAX + Vector.y], XMM2
    ADDSS XMM1, XMM0
    ADDSS XMM1, dword ptr [ECX + 2Ch]
    MOVSS dword ptr [EAX + Vector.z], XMM1

    ; xm0 = m[1][3]
    ; xm0 *= xm6
    ; v.y = xm2
    ; xm1 += xm0
    ; xm1 += m[2][3]
    ; v.z = xm1

    RET 8
@MonAsm_MatrixMulVector_9862575@16 ENDP

; void __cdecl MonAsm_PosAndNormToPlane_5135(const mon::Vector& pos, const mon::Vector& dir, mon::VPlane& out) : server.dll[0x44e30a]
_MonAsm_PosAndNormToPlane_9862575 PROC PUBLIC
    MOV ECX, [ESP + 12] ; ecx : plane
    MOV EDX, [ESP + 8]  ; edx : normal
    MOV EAX, [ESP + 4]  ; eax : pos
    ; copy normal
    FLD  dword ptr [EDX + Vector.x]
    FSTP dword ptr [ECX + VPlane.n.x]
    FLD  dword ptr [EDX + Vector.y]
    FSTP dword ptr [ECX + VPlane.n.y]
    FLD  dword ptr [EDX + Vector.z]
    FSTP dword ptr [ECX + VPlane.n.z]
    ; dot product
    MOVSS XMM0, dword ptr [EDX + VPlane.n.x]
    MOVSS XMM1, dword ptr [EDX + VPlane.n.y]
    MULSS XMM0, dword ptr [EAX + Vector.x]
    MULSS XMM1, dword ptr [EAX + Vector.y]
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [EDX + VPlane.n.z]
    MULSS XMM0, dword ptr [EAX + Vector.z]
    ADDSS XMM1, XMM0
    MOVSS dword ptr [ECX + VPlane.d], XMM1
    RET
_MonAsm_PosAndNormToPlane_9862575 ENDP

; bool __cdecl MonAsm_PointBehindPlane_5135(const mon::VPlane& plane, const mon::Vector& pt) : server.dll[0x455b2c]
_MonAsm_PointBehindPlane_9862575 PROC PUBLIC
    MOV ECX, [ESP + 4] ; ecx : plane
    MOV EDX, [ESP + 8] ; edx : point
    MOVSS XMM0, dword ptr [ECX + VPlane.n.x]
    MULSS XMM0, dword ptr [EDX + Vector.x]
    MOVSS XMM1, dword ptr [ECX + VPlane.n.y]
    MULSS XMM1, dword ptr [EDX + Vector.y]
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [ECX + VPlane.n.z]
    MULSS XMM0, dword ptr [EDX + Vector.z]
    ADDSS XMM1, XMM0
    MOVSS XMM0, dword ptr [ECX + VPlane.d]
    COMISS XMM0, XMM1
    SETA AL
    RET
_MonAsm_PointBehindPlane_9862575 ENDP

END
