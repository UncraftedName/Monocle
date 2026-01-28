.686
.MODEL FLAT

; I'm not exactly an assembler pro, so I don't really understand the full reasons behind this, but
; to link the mangled C++ names directly I have to:
; - not specify a language (no 'OPTION LANGUAGE' and no '.model flat, C')
; - not use aliases (that causes MSVC to jump to zero-filled mememory instead of my function)
;
; If I decide to switch this out to thunks like:
; float Dot(const Vector& o) const { return MonAsmVectorDot(this, &o); }
; then I either need to use C as the language in this file or add an underscore to the symbol.

Vector STRUCT
    x DWORD ?
    y DWORD ?
    z DWORD ?
Vector ENDS

VPlane STRUCT
    n Vector <>
    d DWORD ?
VPlane ENDS

EXTERN DEG2RAD_MUL_F:REAL4
EXTERN NEG1_F:REAL4
