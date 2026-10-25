org 100h
section .text

start:
    mov     AH,  GET_DOS_VERSION
    int     21h
    cmp     AL, 0x2                 ; check if dos version is >= 2.0
    jnc     passed_dos_check
    xor     AX, AX                  ; return if it isn't
    push    ES
    push    AX
    retf

passed_dos_check:
    mov     di, 0x182b              ; not sure what this is doing tbh
    mov     si, [0x2]
    sub     si, di
    cmp     si, 0x1000
    jc      skip_set
    mov     si, 0x1000
skip_set:
    cli
    ; TODO: keep going

%include "macros.mac"