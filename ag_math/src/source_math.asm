.686
.model flat, C

.const

M_PI_F REAL4 3.1415927
_DEG2RAD_MUL REAL4 0.01745329 ; float(PI/180)

.code

; server.dll[0x451670]
AngleMatrix PROC PUBLIC
    SUB ESP, 20h
    LEA ECX, [ESP + 8]
    MOV [ESP + 14h], ECX
    MOV ECX, [ESP + 24h]
    FLD dword ptr [ECX + 4]
    LEA EAX, [ESP + 4]
    FMUL dword ptr DS:[_DEG2RAD_MUL]
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
    FMUL dword ptr DS:[_DEG2RAD_MUL]
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
    FMUL dword ptr DS:[_DEG2RAD_MUL]
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

END
