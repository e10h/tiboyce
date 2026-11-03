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
    ld hl,0
    ld l,a
    ld h,3
    mlt hl
    ld de,_tba_levels+96*\n
    add hl,de
    ld de,(hl)
    add iy,de
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
    jr z,.Lnoise_done
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
    ld hl,0
    ld l,a
    ld h,3
    mlt hl
    ld de,_tba_levels+288
    add hl,de
    ld de,(hl)
    add iy,de
    push iy
    pop hl
    ld de,(_tba_dc)
    or a,a
    sbc hl,de
    ld (ix),l
    ld (ix+1),h
    inc ix
    inc ix
    ; Signed arithmetic right shift by nine, preserving the upper ADL byte.
    ld a,h
    add a,a
    sbc a,a
    ld l,h
    ld h,a
    sra h
    rr l
    add hl,de
    ld (_tba_dc),hl
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
