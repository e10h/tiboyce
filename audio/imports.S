.assume adl=1
.section .text.loader,"ax",@progbits
.global _tba_resolve
_tba_resolve:
    ld hl, .Lname-1
    call 0x020320
    ld a,0x15
    ld (0xd005f8),a
    call 0x02050c
    ret c
    call 0x021f98
    ret z
    ld hl,19
    add hl,de
    ld de,.Limports
    jp (hl)
.Lname: .asciz "LibLoad"
.Limports:
    .db 0xc0
    .asciz "USBDRVCE"
    .db 0
.global _usb_Init
_usb_Init: jp 0
.global _usb_Cleanup
_usb_Cleanup: jp 3
.global _usb_HandleEvents
_usb_HandleEvents: jp 9
.global _usb_FindDevice
_usb_FindDevice: jp 36
.global _usb_GetDeviceEndpoint
_usb_GetDeviceEndpoint: jp 84
.global _usb_SetEndpointFlags
_usb_SetEndpointFlags: jp 108
.global _usb_GetFrameNumber
_usb_GetFrameNumber: jp 117
.global _usb_ScheduleControlTransfer
_usb_ScheduleControlTransfer: jp 126
.global _usb_ScheduleTransfer
_usb_ScheduleTransfer: jp 129
    nop
    ld a,1
    ld (_tba_loaded),a
    ret
