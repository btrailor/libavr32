/* cdc.h

   cdc driver for monome aleph - USB CDC grid support
 */

#ifndef _ALEPH_CDC_H_
#define _ALEPH_CDC_H_

#include "types.h"
#include "uhc.h"

#define CDC_RX_BUF_SIZE 64

// CDC class requests and specific handling
#define CDC_GET_LINE_CODING             0x21
#define CDC_SET_LINE_CODING             0x20
#define CDC_SET_CONTROL_LINE_STATE      0x22

// pointer to rx data
// read from CDC device on usb.
// returns bytes read.
// data is in the externally-visible rx buffer.
// non-blocking
extern void cdc_read(void);

// write to CDC device
extern void cdc_write(u8* data, u32 bytes);

// boot-time plug state (set in cdc_change before event queue wipe)
extern u8 cdc_was_plugged(void);

// CDC device was plugged or unplugged
extern void cdc_change(uhc_device_t* dev, u8 plug);
// main-loop setup routine for new device connection
extern void cdc_setup(void);
extern void cdc_disconnect(void);

//-- getters

// rx buffer 
extern u8* cdc_rx_buf(void);
// number of bytes from last rx transfer
extern volatile u8 cdc_rx_bytes(void);
// busy flags
extern volatile u8 cdc_rx_busy(void);
extern volatile u8 cdc_tx_busy(void);
// device connected flag
extern u8 cdc_connected(void);

#endif