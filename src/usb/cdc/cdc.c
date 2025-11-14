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

//---- defines

//---- extern vars
u8 cdcConnect = 0;

//----- static vars
static u8 rxBuf[CDC_RX_BUF_SIZE];
static u32 rxBytes = 0;
static u8 rxBusy = 0;
static u8 txBusy = 0;
static event_t e;

//------- static functions

static void cdc_rx_done(usb_add_t add,
                        usb_ep_t ep,
                        uhd_trans_status_t stat,
                        iram_size_t nb) {
  rxBytes = nb;

  if (stat != UHD_TRANS_NOERROR) {
    print_dbg("\r\n cdc rx transfer callback error. status: 0x");
    print_dbg_hex((u32)stat);
    print_dbg(" ; bytes transferred: ");
    print_dbg_ulong(nb);
  }

  if (rxBytes) {
    // check for monome events
    (*monome_read_serial)();
  }

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
  print_dbg("\r\n cdc_read() called");
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
  if(plug) {
    e.type = kEventCdcConnect;
  } else {
    cdcConnect = 0;
    e.type = kEventCdcDisconnect;
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
  // set connection flag
  cdcConnect = 1;

  // get string data...
  uhi_cdc_get_strings(&manstr, &prodstr, &serstr);
  
  print_dbg("\r\n CDC strings: man=");
  print_dbg(manstr);
  print_dbg(" prod=");
  print_dbg(prodstr);
  print_dbg(" ser=");
  print_dbg(serstr);
  
  //// query if this is a monome device
  result = check_monome_device_desc(manstr, prodstr, serstr);
  print_dbg("\r\n CDC device check result: ");
  print_dbg_hex(result);
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

// device connected flag
extern u8 cdc_connected(void) {
  return cdcConnect;
}