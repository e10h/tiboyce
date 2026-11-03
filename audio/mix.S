.assume adl=1
.section .text.mix,"ax",@progbits
.global _tba_fast_mix
.macro voice n, mask
    ld hl,(_tba_phase+3*\n)
    ld de,(_tba_step+3*\n)
    add hl,de
    ld (_tba_phase+3*\n),hl
    ld a,(_tba_phase+3*\n+2)
    and \mask
    add a,a
    add a,a
    ld hl,_tba_levels+256*\n
    ld l,a
    ld de,(hl)
    add iy,de
.endm
.macro noise_jump n
    ld a,(_tba_short)
    or a,a
    jr nz,.Lshort_jump\n
    ld c,h
    ld de,0
    ld e,l
    sla e
    rl d
    ld hl,.Lwide\n\()_lo
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    ld hl,0
    ld l,c
    add hl,hl
    ld de,.Lwide\n\()_hi
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    pop hl
    ld a,l
    xor e
    ld l,a
    ld a,h
    xor d
    ld h,a
    jr .Ljump_done\n
.Lshort_jump\n:
    ld a,l
    and 127
    ld de,0
    ld e,a
    sla e
    rl d
    ld hl,.Lshort\n
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
.Ljump_done\n:
    ld a,b
    sub \n
    ld b,a
    jp z,.Lnoise_done
    jp .Lnoise_blocks
.endm
_tba_fast_mix:
    push ix
    push iy
    ld ix,0
    add ix,sp
    ld hl,(ix+12)
    ld (.Lcount),hl
    ld ix,(ix+9)
.Lsample:
    ld iy,0
    voice 0,7
    voice 1,7
    voice 2,31
    ld hl,(_tba_phase+9)
    ld de,(_tba_step+9)
    add hl,de
    ld (_tba_phase+9),hl
    ld a,(_tba_phase+11)
    ld b,a
    xor a,a
    ld (_tba_phase+11),a
    ld hl,(_tba_noise)
    or a,b
    jp z,.Lnoise_done
.Lnoise_blocks:
    ld a,b
    cp 128
    jr c,.Ltry32
    noise_jump 128
.Ltry32:
    ld a,b
    cp 32
    jr c,.Ltry8
    noise_jump 32
.Ltry8:
    ld a,b
    cp 8
    jr c,.Lnoise_tail
    ld a,(_tba_short)
    or a,a
    jr nz,.Lnoise_short8
    ; Eight 15-bit clocks: (x >> 8) | (((x ^ (x >> 1)) & 255) << 7).
    ld e,l
    ld a,h
    rrca
    rr e
    ld a,l
    xor e
    ld l,h
    rra
    ld h,a
    jr nc,.Lnoise_block_next
    set 7,l
    jr .Lnoise_block_next
.Lnoise_short8:
    ; The 7-bit feedback sequence depends only on the current low seven bits.
    ld a,l
    and 127
    ld de,0
    ld e,a
    ld hl,.Lshort8
    add hl,de
    ld a,(hl)
    ld hl,0
    srl a
    ld h,a
    ld l,a
    jr nc,.Lnoise_block_next
    set 7,l
.Lnoise_block_next:
    ld a,b
    sub 8
    ld b,a
    jp nz,.Lnoise_blocks
    jp .Lnoise_done
.Lnoise_tail:
    ld a,b
    or a,a
    jp z,.Lnoise_done
.Lnoise:
    ld a,l
    rrca
    xor l
    and 1
    srl h
    rr l
    or a,a
    jr z,.Lfeedback_zero
    set 6,h
.Lfeedback_zero:
    ld e,a
    ld a,(_tba_short)
    or a,a
    jr z,.Lnoise_next
    res 6,l
    ld a,e
    or a,a
    jr z,.Lnoise_next
    set 6,l
.Lnoise_next:
    djnz .Lnoise
.Lnoise_done:
    ld (_tba_noise),hl
    ld a,l
    and 1
    add a,a
    add a,a
    ld hl,_tba_levels+768
    ld l,a
    ld de,(hl)
    add iy,de
    push iy
    pop hl
    ld (ix),l
    ld (ix+1),h
    inc ix
    inc ix
    ; Device-mode FIFO work needs servicing during synthesis, not just between
    ; game events. At 16 kHz this yields once per millisecond of generated PCM.
    ld a,(.Lpoll_count)
    dec a
    ld (.Lpoll_count),a
    jr nz,.Lno_poll
    ld a,(_tba_poll_interval)
    ld (.Lpoll_count),a
    push ix
    push iy
    call _tba_poll
    di
    pop iy
    pop ix
.Lno_poll:
    ld hl,(.Lcount)
    dec hl
    ld (.Lcount),hl
    ld a,h
    or a,l
    jp nz,.Lsample
    pop iy
    pop ix
    ret
.section .data.mix,"aw",@progbits
.Lcount: .d24 0
.Lpoll_count: .byte 16

.section .rodata.noise,"a",@progbits
.Lshort8:
 .byte 0,193,67,130,134,71,197,4,12,205,79,142,138,75,201,8
 .byte 24,217,91,154,158,95,221,28,20,213,87,150,146,83,209,16
 .byte 48,241,115,178,182,119,245,52,60,253,127,190,186,123,249,56
 .byte 40,233,107,170,174,111,237,44,36,229,103,166,162,99,225,32
 .byte 96,161,35,226,230,39,165,100,108,173,47,238,234,43,169,104
 .byte 120,185,59,250,254,63,189,124,116,181,55,246,242,51,177,112
 .byte 80,145,19,210,214,23,149,84,92,157,31,222,218,27,153,88
 .byte 72,137,11,202,206,15,141,76,68,133,7,198,194,3,129,64

.include "audio/noise-jumps.inc"
