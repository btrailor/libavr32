/* monome_transport.c
   aleph-avr32

   Transport abstraction layer for monome communication.
   Provides unified interface for FTDI and CDC transports.
*/

#include "monome_transport.h"
#include "ftdi.h"
#include "usb/cdc/cdc.h"
#include "print_funcs.h"

//------ static variables

// current transport type
eMonomeTransport monome_transport_type = eTransportNone;

// function pointer tables for each transport
static const transport_write_t transport_write_funcs[eTransportNumTransports] = {
  NULL,           // eTransportNone
  &ftdi_write,    // eTransportFTDI
  &cdc_write      // eTransportCDC
};

static const transport_tx_busy_t transport_tx_busy_funcs[eTransportNumTransports] = {
  NULL,           // eTransportNone
  &ftdi_tx_busy,  // eTransportFTDI
  &cdc_tx_busy    // eTransportCDC
};

static const transport_read_t transport_read_funcs[eTransportNumTransports] = {
  NULL,           // eTransportNone
  &ftdi_read,     // eTransportFTDI
  &cdc_read       // eTransportCDC
};

static const transport_setup_t transport_setup_funcs[eTransportNumTransports] = {
  NULL,           // eTransportNone
  &ftdi_setup,    // eTransportFTDI
  &cdc_setup      // eTransportCDC
};

static const transport_rx_busy_t transport_rx_busy_funcs[eTransportNumTransports] = {
  NULL,             // eTransportNone
  &ftdi_rx_busy,    // eTransportFTDI
  &cdc_rx_busy      // eTransportCDC
};

static const transport_rx_bytes_t transport_rx_bytes_funcs[eTransportNumTransports] = {
  NULL,             // eTransportNone
  &ftdi_rx_bytes,   // eTransportFTDI
  &cdc_rx_bytes     // eTransportCDC
};

static const transport_rx_buf_t transport_rx_buf_funcs[eTransportNumTransports] = {
  NULL,             // eTransportNone
  &ftdi_rx_buf,     // eTransportFTDI
  &cdc_rx_buf       // eTransportCDC
};

static const transport_disconnect_t transport_disconnect_funcs[eTransportNumTransports] = {
  NULL,             // eTransportNone
  NULL,             // eTransportFTDI (no explicit disconnect needed)
  &cdc_disconnect   // eTransportCDC
};

//------ extern function definitions

// initialize transport system
void monome_transport_init(void) {
  monome_transport_type = eTransportNone;
  print_dbg("\r\n monome transport system initialized");
}

// set active transport
void monome_transport_set(eMonomeTransport transport) {
  if (transport >= eTransportNumTransports) {
    print_dbg("\r\n WARNING: invalid transport type");
    return;
  }
  
  monome_transport_type = transport;
  print_dbg("\r\n monome transport set to: ");
  print_dbg_ulong(transport);
}

// get current transport
eMonomeTransport monome_transport_get(void) {
  return monome_transport_type;
}

// unified transport interface functions
u8 monome_transport_write(u8* data, u8 bytes) {
  if (monome_transport_type == eTransportNone) {
    return 0;
  }
  
  transport_write_t write_func = transport_write_funcs[monome_transport_type];
  if (write_func == NULL) {
    return 0;
  }
  
  return write_func(data, bytes);
}

u8 monome_transport_tx_busy(void) {
  if (monome_transport_type == eTransportNone) {
    return 0;
  }
  
  transport_tx_busy_t busy_func = transport_tx_busy_funcs[monome_transport_type];
  if (busy_func == NULL) {
    return 0;
  }
  
  return busy_func();
}

u8 monome_transport_read(void) {
  if (monome_transport_type == eTransportNone) {
    return 0;
  }
  
  transport_read_t read_func = transport_read_funcs[monome_transport_type];
  if (read_func == NULL) {
    return 0;
  }
  
  return read_func();
}

u8 monome_transport_rx_busy(void) {
  if (monome_transport_type == eTransportNone) {
    return 0;
  }
  
  transport_rx_busy_t rx_busy_func = transport_rx_busy_funcs[monome_transport_type];
  if (rx_busy_func == NULL) {
    return 0;
  }
  
  return rx_busy_func();
}

u8 monome_transport_rx_bytes(void) {
  if (monome_transport_type == eTransportNone) {
    return 0;
  }
  
  transport_rx_bytes_t rx_bytes_func = transport_rx_bytes_funcs[monome_transport_type];
  if (rx_bytes_func == NULL) {
    return 0;
  }
  
  return rx_bytes_func();
}

u8* monome_transport_rx_buf(void) {
  if (monome_transport_type == eTransportNone) {
    return NULL;
  }
  
  transport_rx_buf_t rx_buf_func = transport_rx_buf_funcs[monome_transport_type];
  if (rx_buf_func == NULL) {
    return NULL;
  }
  
  return rx_buf_func();
}

void monome_transport_setup(void) {
  if (monome_transport_type == eTransportNone) {
    return;
  }
  
  transport_setup_t setup_func = transport_setup_funcs[monome_transport_type];
  if (setup_func != NULL) {
    setup_func();
  }
}

void monome_transport_disconnect(void) {
  if (monome_transport_type == eTransportNone) {
    return;
  }
  
  transport_disconnect_t disconnect_func = transport_disconnect_funcs[monome_transport_type];
  if (disconnect_func != NULL) {
    disconnect_func();
  }
  
  monome_transport_type = eTransportNone;
}

// transport-specific setup functions
void monome_transport_setup_ftdi(void) {
  print_dbg("\r\n setting up FTDI monome transport");
  monome_transport_set(eTransportFTDI);
  monome_transport_setup();
}

void monome_transport_setup_cdc(void) {
  print_dbg("\r\n setting up CDC monome transport");
  monome_transport_set(eTransportCDC);
  monome_transport_setup();
}