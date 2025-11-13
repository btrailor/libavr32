/*
  uhi_cdc.c
  aleph-avr32

  usb host interface wrapper for CDC driver (monome grids)
 */

// asf
#include <string.h>
#include "delay.h"
#include "print_funcs.h"
#include "usb_protocol.h"
#include "usb_protocol_cdc.h"
#include "uhd.h"
#include "uhc.h"
// aleph
#include "conf_usb_host.h"
#include "cdc.h"
#include "uhi_cdc.h"

#ifdef USB_HOST_HUB_SUPPORT
# error USB HUB support is not implemented
#endif

//------ DEFINES
#define UHI_CDC_TIMEOUT 20
#define CDC_STRING_DESC_REQ_TYPE ( (USB_REQ_DIR_IN) | (USB_REQ_TYPE_STANDARD) | (USB_REQ_RECIP_DEVICE) )
#define CDC_STRING_DESC_LANGID USB_LANGID_EN_US
// offset into the string descriptor to get an actual (unicode) string
#define CDC_STRING_DESC_OFFSET 2

// control request types
#define CDC_DEVICE_OUT_REQTYPE 0b00100001  // Interface, Class, Device to host
#define CDC_DEVICE_IN_REQTYPE  0b10100001  // Interface, Class, Host to device

// CDC class-specific requests
#define CDC_REQ_SET_LINE_CODING         0x20
#define CDC_REQ_GET_LINE_CODING         0x21
#define CDC_REQ_SET_CONTROL_LINE_STATE  0x22

//----- data types
// device data structure
typedef struct {
  uhc_device_t *dev;
  usb_ep_t ep_in;
  usb_ep_t ep_out;
  usb_ep_t ep_comm; // communication endpoint (interrupt)
} uhi_cdc_dev_t;

//----- static variables

// device data
static uhi_cdc_dev_t uhi_cdc_dev = {
  .dev = NULL,
};

// manufacturer string
char manufacturer_string[CDC_STRING_MAX_LEN];
// product string
char product_string[CDC_STRING_MAX_LEN];
// serial number string
char serial_string[CDC_STRING_MAX_LEN];

// control read-busy flag
static volatile u8 ctlReadBusy = 0;

//------- static funcs

// send control request
static u8 send_ctl_request(u8 reqtype, u8 reqnum,
			   u8* data, u16 size,
			     u16 index, u16 val,
			     uhd_callback_setup_end_t callbackEnd);
// control request end
static void ctl_req_end(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);

//----- external (UHC) functions
uhc_enum_status_t uhi_cdc_install(uhc_device_t* dev) {
  bool b_iface_supported;
  bool b_iface_comm_supported = false;
  bool b_iface_data_supported = false;
  uint16_t conf_desc_lgt;
  usb_iface_desc_t *ptr_iface;
  uint16_t vid, pid;

  print_dbg("\r\n CDC: uhi_cdc_install() called");

  if (uhi_cdc_dev.dev != NULL) {
    return UHC_ENUM_SOFTWARE_LIMIT; // Device already allocated
  }

  vid = le16_to_cpu(dev->dev_desc.idVendor);
  pid = le16_to_cpu(dev->dev_desc.idProduct);

  // Check if this is a monome device by VID
  // Accept both original monome VID (0x16c0) and STM32 VID (0x0483) for 2021+ grids
  if(vid != 0x16c0 && vid != 0x0483) {
    print_dbg("\r\n CDC: unsupported VID, rejecting");
    return UHC_ENUM_UNSUPPORTED;
  }
  
  print_dbg("\r\n CDC: VID accepted (");
  print_dbg_hex(vid);
  print_dbg("), proceeding with enumeration");

  conf_desc_lgt = le16_to_cpu(dev->conf_desc->wTotalLength);
  ptr_iface = (usb_iface_desc_t*)dev->conf_desc;

  uhi_cdc_dev.ep_in = 0;
  uhi_cdc_dev.ep_out = 0;
  uhi_cdc_dev.ep_comm = 0;

  while(conf_desc_lgt) {
    switch (ptr_iface->bDescriptorType) {

    case USB_DT_INTERFACE:
      if ((ptr_iface->bInterfaceClass == CDC_CLASS_COMM) &&
          (ptr_iface->bInterfaceSubClass == CDC_SUBCLASS_ACM)) {
        print_dbg("\r\n CDC: found communication interface");
        b_iface_comm_supported = true;
        b_iface_supported = true;
      } else if (ptr_iface->bInterfaceClass == CDC_CLASS_DATA) {
        print_dbg("\r\n CDC: found data interface");
        b_iface_data_supported = true;
        b_iface_supported = true;
      } else {
        b_iface_supported = false;
      }
      break;

    case USB_DT_ENDPOINT:
      if (!b_iface_comm_supported && !b_iface_data_supported) {
	break;
      }
      if (!uhd_ep_alloc(dev->address, (usb_ep_desc_t*)ptr_iface)) {
	print_dbg("\r\n endpoint allocation failed");
	return UHC_ENUM_HARDWARE_LIMIT;
      }

      switch(((usb_ep_desc_t*)ptr_iface)->bmAttributes & USB_EP_TYPE_MASK) {
      case USB_EP_TYPE_BULK:
	print_dbg("\r\n allocating bulk endpoint");
	if (((usb_ep_desc_t*)ptr_iface)->bEndpointAddress & USB_EP_DIR_IN) {
	  uhi_cdc_dev.ep_in = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
	} else {
	  uhi_cdc_dev.ep_out = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
	}
	break;
      case USB_EP_TYPE_INTERRUPT:
	print_dbg("\r\n allocating interrupt endpoint");
	uhi_cdc_dev.ep_comm = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
	break;
      default:
	print_dbg("\r\n unhandled endpoint in cdc device enumeration");
	break;
      }
      break;
    default:
      // print_dbg("\r\n ignoring descriptor in cdc device enumeration");
      break;
    }
    Assert(conf_desc_lgt>=ptr_iface->bLength);
    conf_desc_lgt -= ptr_iface->bLength;
    ptr_iface = (usb_iface_desc_t*)((uint8_t*)ptr_iface + ptr_iface->bLength);
  }

  print_dbg("\r\n CDC: checking device compatibility:");
  print_dbg("\r\n   comm_supported="); print_dbg_hex(b_iface_comm_supported);
  print_dbg(" data_supported="); print_dbg_hex(b_iface_data_supported);
  print_dbg(" ep_in="); print_dbg_hex(uhi_cdc_dev.ep_in);
  print_dbg(" ep_out="); print_dbg_hex(uhi_cdc_dev.ep_out);

  if (b_iface_comm_supported && b_iface_data_supported && 
      uhi_cdc_dev.ep_in && uhi_cdc_dev.ep_out) {
    uhi_cdc_dev.dev = dev;
    print_dbg("\r\n CDC: device install SUCCESS");
    return UHC_ENUM_SUCCESS;
  }
  print_dbg("\r\n CDC: device install FAILED - missing required interfaces or endpoints");
  return UHC_ENUM_UNSUPPORTED; // No interface supported
}

void uhi_cdc_enable(uhc_device_t* dev) {
  if (uhi_cdc_dev.dev != dev) {
    return;  // No interface to enable
  }

  // Set line coding (optional for many CDC devices)
  // Most monome devices work without explicit line coding setup
  
  print_dbg("\r\n CDC device enabled");
  delay_ms(100);
  
  cdc_change(dev, true);
}

void uhi_cdc_uninstall(uhc_device_t* dev) {
  if (uhi_cdc_dev.dev != dev) {
    return; // Device not installed here
  }
  if (uhi_cdc_dev.ep_in) {
    uhd_ep_free(dev->address, uhi_cdc_dev.ep_in);
  }
  if (uhi_cdc_dev.ep_out) {
    uhd_ep_free(dev->address, uhi_cdc_dev.ep_out);
  }
  if (uhi_cdc_dev.ep_comm) {
    uhd_ep_free(dev->address, uhi_cdc_dev.ep_comm);
  }
  uhi_cdc_dev.dev = NULL;
  print_dbg("\r\n CDC device uninstalled");
  cdc_change(dev, false);
}

// transfers
bool uhi_cdc_read(uint8_t * buf, iram_size_t buf_size,
		uhd_callback_trans_t callback) {
  return uhd_ep_run(uhi_cdc_dev.dev->address,
		    uhi_cdc_dev.ep_in,
		    false,
		    buf,
		    buf_size,
		    UHI_CDC_TIMEOUT,
		    callback);
}

bool uhi_cdc_write(uint8_t * buf, iram_size_t buf_size,
		 uhd_callback_trans_t callback) {
  return uhd_ep_run(uhi_cdc_dev.dev->address,
		    uhi_cdc_dev.ep_out,
		    false,
		    buf,
		    buf_size,
		    UHI_CDC_TIMEOUT,
		    callback);
}

// get string descriptions - simplified version
void uhi_cdc_get_strings(char** pManufacturer, char** pProduct, char** pSerial) {
  // For simplicity, we'll just provide static strings
  // In a full implementation, we'd read the actual USB string descriptors
  strcpy(manufacturer_string, "monome");
  strcpy(product_string, "grid");
  strcpy(serial_string, "cdc001");
  
  *pManufacturer = manufacturer_string;
  *pProduct = product_string;
  *pSerial = serial_string;
}

//------ static function implementations
static u8 send_ctl_request(u8 reqtype, u8 reqnum,
			   u8* data, u16 size,
			   u16 index, u16 val,
			   uhd_callback_setup_end_t callbackEnd) {
  usb_setup_req_t req;

  req.bmRequestType = reqtype;
  req.bRequest = reqnum;
  req.wValue = cpu_to_le16(val);
  req.wIndex = cpu_to_le16(index);
  req.wLength = cpu_to_le16(size);

  ctlReadBusy = 1;
  return uhd_setup_request(uhi_cdc_dev.dev->address,
			   &req,
			   data,
			   size,
			   NULL, // Use internal callback for simplicity
			   callbackEnd);
}

static void ctl_req_end(usb_add_t add,
			uhd_trans_status_t status,
			uint16_t payload_trans) {
  ctlReadBusy = 0;
  UNUSED(add);
  UNUSED(status);
  UNUSED(payload_trans);
}