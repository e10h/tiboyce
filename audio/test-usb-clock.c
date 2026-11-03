#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "usbdrvce.h"
uint8_t tba_test_wave[16],tba_test_status;
extern uint8_t tba_loaded;extern volatile uint8_t *tba_regs_ptr;
void tba_start(void);void tba_frame(void);void tba_stop(void);void tba_poll(void);
static unsigned int tick;static uint32_t packets,completed;
static int async_mode;
static struct {usb_endpoint_t ep;size_t left;usb_callback_t cb;void *arg;} q[32];
static unsigned int qread,qwrite;
usb_error_t usb_Init(usb_error_t (*event)(usb_event_t,void*,void*),void *a,const usb_standard_descriptors_t *d,unsigned int f){(void)a;(void)d;(void)f;event(USB_HOST_CONFIGURE_EVENT,0,0);return 0;}
void usb_Cleanup(void){qread=qwrite=0;}
void usb_HandleEvents(void){if(async_mode&&qread!=qwrite){
    unsigned int i=qread%32;
    if(q[i].left>64)q[i].left-=64;
    else {q[i].cb(q[i].ep,0,272,q[i].arg);qread++;completed++;}
}}
usb_device_t usb_FindDevice(void*a,void*b,int f){(void)a;(void)b;(void)f;return (void*)1;}
usb_endpoint_t usb_GetDeviceEndpoint(void*a,int e){(void)a;assert(e==0x81);return (void*)1;}
void usb_SetEndpointFlags(void*a,int f){(void)a;(void)f;}
unsigned int usb_GetFrameNumber(void){return tick;}
usb_error_t usb_ScheduleBulkTransfer(usb_endpoint_t e,void *p,size_t n,usb_callback_t cb,void*a){uint8_t *b=p;assert(n==272&&!memcmp(b,"DVU1",4));assert(b[8]==0x80&&b[9]==0x3e&&b[14]==4);packets++;
if(async_mode){assert(qwrite-qread<32);unsigned int i=qwrite++%32;q[i].ep=e;q[i].left=n;q[i].cb=cb;q[i].arg=a;return 0;}
return cb(e,0,n,a);}
int main(void){uint8_t regs[22]={0};const unsigned int delays[]={16,17,25,40,45};tba_loaded=1;tba_regs_ptr=regs;
for(unsigned int test=0;test<5;test++){tick=16320;packets=0;tba_start();tba_frame();unsigned int elapsed=0;
    for(unsigned int frame=0;frame<200;frame++){unsigned int ms=delays[(frame+test)%5];elapsed+=ms;tick=(tick+ms*8)&0x3fff;tba_frame();}
    assert(packets*128<=elapsed*16);assert(elapsed*16-packets*128<128);tba_stop();
}
/* Model a 64-byte FIFO requiring five poll passes per audio packet.
   VBlank-only polling must reproduce starvation; event/wait polling must drain. */
async_mode=1;unsigned int old_completed=0;
for(unsigned int frequent=0;frequent<2;frequent++){
    tick=0;packets=completed=0;qread=qwrite=0;tba_start();tba_frame();
    for(unsigned int ms=1;ms<=4000;ms++){
        tick=(ms*8)&0x3fff;
        if(frequent)tba_poll();
        if(ms%40==0)tba_frame();
    }
    if(!frequent){old_completed=completed;assert(completed<250);}
    else {assert(completed>480);assert(completed>old_completed*2);}
    tba_stop();
}
puts("USB FIFO model: frame-only polls starve; 1 ms service sustains >120 chunks/s.");
puts("Variable 16–45 ms game frames and USB counter wrap: 16,000 PCM samples per elapsed second passed.");}
