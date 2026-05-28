/* cdc.h
   aleph-avr32

   CDC (Communication Device Class) driver for modern monome grids.
   Provides interface compatible with FTDI driver patterns.
*/

#ifndef _ALEPH_CDC_H_
#define _ALEPH_CDC_H_

#include "types.h"
#include "uhc.h"

//------ defines
#define CDC_RX_BUF_SIZE 64
#define CDC_TX_BUF_SIZE 64

//------ extern function declarations

// initialization
extern void cdc_init(void);

// device connection/disconnection
extern void cdc_setup(void);
extern void cdc_disconnect(void);

// data transmission
extern u8 cdc_write(u8* data, u8 bytes);
extern u8 cdc_tx_busy(void);

// data reception
extern void cdc_read(void);
extern u8* cdc_rx_buf(void);
extern volatile u8 cdc_rx_bytes(void);
extern volatile u8 cdc_rx_busy(void);

// device status
extern u8 cdc_connected(void);

// USB callbacks (called by USB host system)
extern void cdc_change(uhc_device_t* dev, u8 plug);
extern void cdc_rx_notify(void);

#endif // _ALEPH_CDC_H_