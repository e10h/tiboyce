#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "usbdrvce.h"
uint8_t tba_test_wave[16],tba_test_status;
extern uint8_t tba_loaded;extern volatile uint8_t *tba_regs_ptr;
void tba_start(void);void tba_frame(void);void tba_stop(void);
static unsigned int tick;static uint32_t packets;
usb_error_t usb_Init(usb_error_t (*event)(usb_event_t,void*,void*),void *a,const usb_standard_descriptors_t *d,unsigned int f){(void)a;(void)d;(void)f;event(USB_HOST_CONFIGURE_EVENT,0,0);return 0;}
void usb_Cleanup(void){}void usb_HandleEvents(void){}
usb_device_t usb_FindDevice(void*a,void*b,int f){(void)a;(void)b;(void)f;return (void*)1;}
usb_endpoint_t usb_GetDeviceEndpoint(void*a,int e){(void)a;assert(e==0x81);return (void*)1;}
void usb_SetEndpointFlags(void*a,int f){(void)a;(void)f;}
unsigned int usb_GetFrameNumber(void){return tick;}
usb_error_t usb_ScheduleBulkTransfer(usb_endpoint_t e,void *p,size_t n,usb_callback_t cb,void*a){uint8_t *b=p;assert(n==272&&!memcmp(b,"DVU1",4));assert(b[8]==0x80&&b[9]==0x3e&&b[14]==4);packets++;return cb(e,0,n,a);}
int main(void){uint8_t regs[22]={0};const unsigned int delays[]={16,17,25,40,45};tba_loaded=1;tba_regs_ptr=regs;
for(unsigned int test=0;test<5;test++){tick=16320;packets=0;tba_start();tba_frame();unsigned int elapsed=0;
    for(unsigned int frame=0;frame<200;frame++){unsigned int ms=delays[(frame+test)%5];elapsed+=ms;tick=(tick+ms*8)&0x3fff;tba_frame();}
    assert(packets*128<=elapsed*16);assert(elapsed*16-packets*128<128);tba_stop();
}
puts("Variable 16–45 ms game frames and USB counter wrap: 16,000 PCM samples per elapsed second passed.");}
