; C calls must preserve both emulator register banks and the event-time I register.
#macro AUDIO_CALL(target)
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    ld hl,i
    push hl
    exx
    ex af,af'
    push af
    push bc
    push de
    push hl
    ld (audio_saved_sp),sp
    ld sp,tba_call_stack+2048
    call target
    di
    ld sp,(audio_saved_sp)
    pop hl
    pop de
    pop bc
    pop af
    ex af,af'
    exx
    pop hl
    ld i,hl
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
#endmacro

AudioResolve:
    AUDIO_CALL(tba_resolve)
    ret
AudioStart:
    push hl
    ld hl,z80codebase+audio_port_values
    ld (tba_regs_ptr),hl
    pop hl
    AUDIO_CALL(tba_start)
    ret
AudioFrame:
    AUDIO_CALL(tba_frame)
    ret
AudioStop:
    AUDIO_CALL(tba_stop)
    ret

; Called from Z80-mode event dispatch using CALL.IL; RET.L restores that mode.
AudioPollZ80:
    AUDIO_CALL(tba_poll)
    ret.l
AudioPoll:
    AUDIO_CALL(tba_poll)
    ret

; Calls are synchronous; driver callbacks stay on the dedicated C stack.
audio_saved_sp:
    .dl 0
