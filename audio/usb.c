#include <usbdrvce.h>
#include <stdint.h>
#include <string.h>
#include "apu.h"
#ifdef TBA_HOST_TEST
extern uint8_t tba_test_wave[16],tba_test_status;
#define WAVE_RAM tba_test_wave
#define APU_STATUS tba_test_status
#else
#define WAVE_RAM ((volatile uint8_t*)0xd4ff30)
#define APU_STATUS (*(volatile uint8_t*)0xd4ff26)
#endif
uint8_t tba_loaded;
volatile uint8_t *tba_regs_ptr;
static TbaApu apu;
static usb_endpoint_t endpoint;
static uint8_t started;
static uint32_t sequence;
static uint16_t position;
static unsigned int last_usb_tick;
static uint8_t clock_sync;
typedef struct { uint8_t data[272],busy; } Slot;
static Slot slots[8];
static uint8_t assembling[272];
static int16_t frame_pcm[268];
static const uint8_t config[25]={9,2,25,0,1,1,0,128,50,9,4,0,0,1,255,68,1,0,7,5,129,2,64,0,0};
static const usb_device_descriptor_t device={18,1,0x0200,0,0,0,64,0x0451,0xe008,0x0201,0,1,0,1};
static const usb_configuration_descriptor_t *configs[]={(const usb_configuration_descriptor_t*)config};
static const usb_string_descriptor_t product={.bLength=30,.bDescriptorType=3,.bString=L"TI-Boy CE PCM2"};
static const usb_string_descriptor_t lang={.bLength=4,.bDescriptorType=3,.bString={0x0409}};
static const usb_string_descriptor_t *strings[]={&product};
static const usb_standard_descriptors_t descriptors={&device,configs,&lang,1,strings};
static usb_error_t complete(usb_endpoint_t ep,usb_transfer_status_t status,size_t n,void *arg){(void)ep;(void)status;(void)n;((Slot*)arg)->busy=0;return USB_SUCCESS;}
static usb_error_t event(usb_event_t e,void *data,void *arg){(void)data;(void)arg;
    if(e==USB_HOST_CONFIGURE_EVENT){usb_device_t host=usb_FindDevice(NULL,NULL,USB_SKIP_HUBS);endpoint=usb_GetDeviceEndpoint(host,0x81);if(endpoint)usb_SetEndpointFlags(endpoint,USB_AUTO_TERMINATE);position=0;clock_sync=1;}
    else if(e==USB_DEVICE_DISCONNECTED_EVENT||e==USB_DEVICE_DISABLED_EVENT)endpoint=NULL;
    return USB_SUCCESS;
}
void tba_stop(void){if(started){usb_Cleanup();started=0;}endpoint=NULL;}
void tba_start(void){
    tba_apu_reset(&apu);sequence=0;position=0;endpoint=NULL;
    memset(slots,0,sizeof(slots));memset(assembling,0,sizeof(assembling));
    if(tba_loaded&&!started)started=usb_Init(event,NULL,&descriptors,USB_DEFAULT_INIT_FLAGS)==USB_SUCCESS;
}
static void packet(void){unsigned int i;assembling[0]='D';assembling[1]='V';assembling[2]='U';assembling[3]='1';
    memcpy(assembling+4,&sequence,4);sequence++;
    assembling[8]=0x80;assembling[9]=0x3e;assembling[10]=assembling[11]=0;
    assembling[12]=128;assembling[13]=0;assembling[14]=4;assembling[15]=0;
    for(i=0;i<8;i++)if(!slots[i].busy){Slot *s=&slots[i];memcpy(s->data,assembling,272);s->busy=1;
        if(usb_ScheduleBulkTransfer(endpoint,s->data,272,complete,s)!=USB_SUCCESS)s->busy=0;
        break;
    }
}
void tba_frame(void){uint8_t regs[0x30]={0},wave[16];unsigned int i,n;
    if(!started)return;
    usb_HandleEvents();
    if(!endpoint||!tba_regs_ptr)return;
    for(i=0;i<22;i++)regs[i]=tba_regs_ptr[i];
    /* Consume trigger latches; TI-Boy keeps CPU-visible masked values separately. */
    for(i=4;i<20;i+=5)tba_regs_ptr[i]&=0x7f;
    for(i=0;i<16;i++)wave[i]=WAVE_RAM[i];
    tba_apu_snapshot(&apu,regs,wave,APU_STATUS);
    /* One full-speed USB frame is 8 counter units = 1 ms. Generate two
       16 kHz samples per unit even when emulation falls below 59.7 fps. */
    unsigned int now=usb_GetFrameNumber()&0x3ff8;
    if(clock_sync){last_usb_tick=now;clock_sync=0;return;}
    n=((now-last_usb_tick)&0x3fff)*2;last_usb_tick=now;
    /* Long pauses are discontinuities, not an unbounded audio backlog. */
    if(n>2048)n=1024;
    while(n){unsigned int count=n>268?268:n;
        tba_apu_render(&apu,frame_pcm,count);
        for(i=0;i<count;i++){int16_t sample=frame_pcm[i];assembling[16+position*2]=(uint8_t)sample;assembling[17+position*2]=(uint16_t)sample>>8;
            if(++position==128){packet();position=0;usb_HandleEvents();}
        }
        n-=count;
    }
}
