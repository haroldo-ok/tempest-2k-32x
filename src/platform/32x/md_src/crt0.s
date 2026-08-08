|----------------------------------------------------------------------------
| WIREFIGHT 32X - minimal 68000 side
| Runs from the 32X cart view at 0x00880800.
| Job: park interrupts, feed both SH2 COMM regs with joypad state, halt.
|----------------------------------------------------------------------------

        .text

        .globl  _start
_start:
        move.w  #0x2700,%sr
        movea.l #0x00FFFFFC,%sp
        jsr     main
0:
        stop    #0x2700
        bra.b   0b

        .align  64
| 0x880840 - general exception handler
        rte
        nop

        .align  64
| 0x880880 - level 4 (HBlank)
        rte
        nop

        .align  64
| 0x8808C0 - level 6 (VBlank)
        rte
        nop

        .align  64
| 0x880900 - level 2 (EXT)
        rte
        nop
