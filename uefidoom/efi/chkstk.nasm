SECTION .text

; A stub needed for alloca() support in MS compiler
global ASM_PFX(__chkstk)
ASM_PFX(__chkstk):
    ret
