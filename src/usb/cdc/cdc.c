/*
  cdc.c
  aleph-avr32

  CDC usb functions for modern monome grid support.
*/

// asf
#include "delay.h"
#include "print_funcs.h"
// aleph
#include "conf_usb_host.h"
#include "events.h"
#include "cdc.h"
#include "monome.h"
#include "uhi_cdc.h"
#include "usb_protocol_cdc.h"

//---- defines

//---- extern vars
u8 cdcConnect = 0;

//----- static vars
static u8 rxBuf[CDC_RX_BUF_SIZE];
static u32 rxBytes = 0;
static u8 rxBusy = 0;
static u8 txBusy = 0;
static event_t e;
static u8 cdcPlugged = 0;

//------- static functions

static void cdc_rx_done(usb_add_t add,
                        usb_ep_t ep,
                        uhd_trans_status_t stat,
                        iram_size_t nb) {
  rxBytes = nb;

  // FIXME: if the buffer is full, it's a false receive
  if (rxBytes > 0 && rxBytes < CDC_RX_BUF_SIZE) {
    // check for monome events
    (*monome_read_serial)();
  }

  rxBytes = 0;
  rxBusy = false;
}

static void cdc_tx_done(usb_add_t add,
                        usb_ep_t ep,
                        uhd_trans_status_t stat,
                        iram_size_t nb) {
  txBusy = false;

  if (stat != UHD_TRANS_NOERROR) {
    print_dbg("\r\n cdc tx transfer callback error. status: 0x");
    print_dbg_hex((u32)stat);
  }
}

//-------- extern functions
void cdc_write(u8* data, u32 bytes) {
  if (txBusy == false) {
    txBusy = true;
    if(!uhi_cdc_write(data, bytes, &cdc_tx_done)) {
      print_dbg("\r\n cdc tx transfer error");
      txBusy = false;
    }
  }
}

void cdc_read(void) {
  // print_dbg("\r\n cdc_read() called");
  if (rxBusy == false) {
    rxBytes = 0;
    rxBusy = true;
    if (!uhi_cdc_read((u8*)rxBuf, CDC_RX_BUF_SIZE, &cdc_rx_done)) {
      print_dbg("\r\n cdc rx transfer error");
      rxBusy = false;
    }
  }
}

// respond to connection or disconnection of cdc device.
// may be called from an interrupt
void cdc_change(uhc_device_t* dev, u8 plug) {
  print_dbg("\r\n cdc_change: plug=");
  print_dbg_hex(plug);
  // guard against duplicate events from interrupt flooding
  static u8 lastPlug = 0xff;
  if(plug == lastPlug) {
    return;  // already in this state, ignore duplicate
  }
  lastPlug = plug;
  
  if(plug) {
    cdcPlugged = 1;
    e.type = kEventSerialConnect;
  } else {
    cdcConnect = 0;
    e.type = kEventSerialDisconnect;
  }
  // posting an event so the main loop can respond
  event_post(&e);
}

// setup new device connection
void cdc_setup(void) {
  char * manstr;
  char * prodstr;
  char * serstr;
  u8 result;
  
  print_dbg("\r\n CDC setup routine");

  // open CDC port 0 with default 115200 8N1 config
  usb_cdc_line_coding_t conf = {
    .dwDTERate   = 115200,
    .bCharFormat = CDC_STOP_BITS_1,
    .bParityType = CDC_PAR_NONE,
    .bDataBits   = 8
  };

  if (!uhi_cdc_open(0, &conf)) {
    print_dbg("\r\n CDC setup: failed to open port");
    return;
  }

  // set connection flag
  cdcConnect = 1;

  // get string data...
  uhi_cdc_get_strings(&manstr, &prodstr, &serstr);
  
  // query if this is a monome device
  result = check_monome_device_desc(manstr, prodstr, serstr);

  if(result) {
    print_dbg("\r\n CDC setup: monome device detected");
    monome_setup_mext();
  }
}

// disconnect
void cdc_disconnect(void) {
  uhi_cdc_close(0);
  cdcConnect = 0;
}

// rx buffer
extern u8* cdc_rx_buf() {
  return rxBuf;
}

// number of bytes from last rx transfer
extern volatile u8 cdc_rx_bytes() {
  return rxBytes;
}

// busy flags
extern volatile u8 cdc_rx_busy() {
  return rxBusy;
}

extern volatile u8 cdc_tx_busy() {
  return txBusy;
}

// boot-time plug state accessor
extern u8 cdc_was_plugged(void) {
  return cdcPlugged;
}

// device connected flag
extern u8 cdc_connected(void) {
  return cdcConnect;
}