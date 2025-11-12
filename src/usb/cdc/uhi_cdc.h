/*
  uhi_cdc.h
  aleph-avr32

  usb host interface for CDC driver

 */

#ifndef _UHI_CDC_H_
#define _UHI_CDC_H_

#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "uhi.h"

//! Global define which contains standard UHI API for UHC
//! It must be added in USB_HOST_UHI define from conf_usb_host.h file.
#define UHI_CDC { \
	.install = uhi_cdc_install, \
	.enable = uhi_cdc_enable, \
	.uninstall = uhi_cdc_uninstall, \
	.sof_notify = NULL, \
}

#define CDC_STRING_MAX_LEN 64

// install
extern uhc_enum_status_t uhi_cdc_install(uhc_device_t* dev);
// uninstall
extern void uhi_cdc_uninstall(uhc_device_t* dev);
// enable
extern void uhi_cdc_enable(uhc_device_t* dev);
// input transfer
extern bool uhi_cdc_read(uint8_t * buf, iram_size_t buf_size,
		uhd_callback_trans_t callback);
// output transfer
extern bool uhi_cdc_write(uint8_t * buf, iram_size_t buf_size,
		uhd_callback_trans_t callback);

// get string descriptions
extern void uhi_cdc_get_strings(char** pManufacturer, char** pProduct, char** pSerial);

#endif // _UHI_CDC_H_