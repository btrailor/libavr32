/*
  cdc.c
  aleph-avr32

  CDC (Communications Device Class) USB host functions for modern monome grids.

  Modeled directly on ftdi.c — uses the same callback-driven pattern with
  UHI CDC bulk transfers instead of FTDI-specific endpoints.

  The monome serial protocol runs over CDC the same way it runs over FTDI:
  - host sends query bytes to identify the device
  - device responds with serial data
  - LED state is pushed from host to device

  CDC has no status bytes (unlike FTDI), so rxBytes == nb directly.
*/

// std
#include <stdint.h>
#include <string.h>

// asf
#include "uhc.h"
#include "print_funcs.h"
#include "uhi_cdc.h"
#include "delay.h"

#include "cdc.h"
#include "conf_usb_host.h"
#include "events.h"
#include "monome.h"
#include "usb_protocol_cdc.h"

//---- defines

//---- extern vars
u8 cdcConnect = 0;

//---- static vars
static u8 rxBuf[CDC_RX_BUF_SIZE];
static u32 rxBytes = 0;
static u8 rxBusy = 0;
static u8 txBusy = 0;
static event_t e;

//------- static callbacks

static void cdc_rx_done(usb_add_t add,
                        usb_ep_t ep,
                        uhd_trans_status_t stat,
                        iram_size_t nb) {
  // CDC has no FTDI status bytes — received count is actual data
  rxBytes = nb;

  if (rxBytes) {
    // dispatch to monome serial handler (same pattern as FTDI)
    if (monome_read_serial != NULL) {
      (*monome_read_serial)();
    }
  }

  rxBusy = false;
}

static void cdc_tx_done(usb_add_t add,
                        usb_ep_t ep,
                        uhd_trans_status_t stat,
                        iram_size_t nb) {
  txBusy = false;

  if (stat != UHD_TRANS_NOERROR) {
    print_dbg("\r\n cdc tx transfer error");
  }
}

//-------- extern functions

void cdc_write(u8* data, u32 bytes) {
  if (txBusy == false) {
    txBusy = true;
    // UHI CDC write returns bytes remaining; 0 means all written
    iram_size_t remaining = uhi_cdc_write_buf(0, data, bytes);
    if (remaining == bytes) {
      // nothing was written
      print_dbg("\r\n cdc tx write error: 0 bytes written");
      txBusy = false;
    } else {
      print_dbg("\r\n cdc_write: ");
      print_dbg_ulong(bytes);
      print_dbg(" bytes, remaining=");
      print_dbg_ulong(remaining);
      // data queued, will be sent by UHI
      // For now, clear immediately since UHI write_buf is synchronous
      // TODO: make this properly async with callback
      txBusy = false;
    }
  }
}

u8 cdc_tx_busy(void) {
  return txBusy;
}

void cdc_read(void) {
  if (rxBusy == false) {
    rxBytes = 0;
    rxBusy = true;
    // request CDC bulk IN transfer into rxBuf
    // uhi_cdc_read_buf() may block waiting for data,
    // so only read what is already available.
    iram_size_t nb = uhi_cdc_get_nb_received(0);
    print_dbg("\r\n cdc_read: nb_received=");
    print_dbg_ulong(nb);
    if (nb) {
      iram_size_t remaining = uhi_cdc_read_buf(0, rxBuf, CDC_RX_BUF_SIZE);
      rxBytes = CDC_RX_BUF_SIZE - remaining;
      print_dbg(" bytes read=");
      print_dbg_ulong(rxBytes);
    }
    rxBusy = false;
  }
}

u8* cdc_rx_buf(void) {
  return rxBuf;
}

volatile u8 cdc_rx_bytes(void) {
  return rxBytes;
}

volatile u8 cdc_rx_busy(void) {
  return rxBusy;
}

// device plugged/unplugged callback
void cdc_change(uhc_device_t* dev, u8 plug) {
  print_dbg("\r\ncdc plug");
  if (plug) {
    cdcConnect = 1;
    e.type = kEventSerialConnect;
  }
  else {
    cdcConnect = 0;
    e.type = kEventSerialDisconnect;
  }
  event_post(&e);
}

// setup new device connection (called from main loop)
void cdc_setup(void) {
  print_dbg("\r\n cdc_setup: opening CDC port");

  // open CDC port 0 with default 115200 8N1 config
  usb_cdc_line_coding_t conf = {
    .dwDTERate   = 115200,
    .bCharFormat = CDC_STOP_BITS_1,
    .bParityType = CDC_PAR_NONE,
    .bDataBits   = 8
  };

  if (!uhi_cdc_open(0, &conf)) {
    print_dbg("\r\n cdc setup: failed to open port");
    return;
  }

  cdcConnect = 1;
  print_dbg("\r\n cdc_setup: port open, calling monome_setup_mext");

  // set up monome function pointers for CDC transport
  // but don't block waiting for grid size response
  monome_setup_mext();
}

void cdc_disconnect(void) {
  uhi_cdc_close(0);
  cdcConnect = 0;
}

u8 cdc_connected(void) {
  return cdcConnect;
}
