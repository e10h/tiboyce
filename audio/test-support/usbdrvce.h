#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
typedef void *usb_endpoint_t;typedef void *usb_device_t;
typedef int usb_error_t;typedef int usb_event_t;typedef int usb_transfer_status_t;
enum {USB_SUCCESS,USB_HOST_CONFIGURE_EVENT,USB_DEVICE_DISCONNECTED_EVENT,USB_DEVICE_DISABLED_EVENT,USB_DEFAULT_SETUP_EVENT,USB_IGNORE};
#define USB_SKIP_HUBS 0
#define USB_AUTO_TERMINATE 0
#define USB_TRANSFER_COMPLETED 0
#define USB_DEFAULT_INIT_FLAGS 0
struct usb_device_descriptor {uint8_t a,b;uint16_t c;uint8_t d,e,f,g;uint16_t h,i,j;uint8_t k,l,m,n;};
typedef struct usb_device_descriptor usb_device_descriptor_t;
typedef struct {uint8_t a;} usb_configuration_descriptor_t;
typedef struct {uint8_t bLength,bDescriptorType;wchar_t bString[64];} usb_string_descriptor_t;
typedef struct {const usb_device_descriptor_t *d;const usb_configuration_descriptor_t **c;const usb_string_descriptor_t *l;uint8_t n;const usb_string_descriptor_t **s;} usb_standard_descriptors_t;
typedef usb_error_t (*usb_callback_t)(usb_endpoint_t,usb_transfer_status_t,size_t,void*);
usb_error_t usb_Init(usb_error_t (*event)(usb_event_t,void*,void*),void*,const usb_standard_descriptors_t*,unsigned int);
void usb_Cleanup(void);void usb_HandleEvents(void);
usb_device_t usb_FindDevice(void*,void*,int);usb_endpoint_t usb_GetDeviceEndpoint(void*,int);
void usb_SetEndpointFlags(void*,int);unsigned int usb_GetFrameNumber(void);
usb_error_t usb_ScheduleBulkTransfer(usb_endpoint_t,void*,size_t,usb_callback_t,void*);

typedef struct {uint8_t bmRequestType,bRequest;uint16_t wValue,wIndex,wLength;} usb_control_setup_t;
usb_error_t usb_ScheduleControlTransfer(usb_endpoint_t,const usb_control_setup_t*,void*,usb_callback_t,void*);
