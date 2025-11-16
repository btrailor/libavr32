/* monome_transport.h
   aleph-avr32

   Transport abstraction layer for monome communication.
   Provides unified interface for FTDI and CDC transports.
*/

#ifndef _ALEPH_MONOME_TRANSPORT_H_
#define _ALEPH_MONOME_TRANSPORT_H_

#include "types.h"

//------ defines
#define MONOME_TRANSPORT_TX_BUF_LEN 72

//------ typedefs

// transport type enumeration
typedef enum {
  eTransportNone,     // no transport selected
  eTransportFTDI,     // FTDI-based transport (older grids)
  eTransportCDC,      // CDC-based transport (modern grids)
  eTransportNumTransports // count
} eMonomeTransport;

// transport function pointers
typedef u8 (*transport_write_t)(u8* data, u8 bytes);
typedef u8 (*transport_tx_busy_t)(void);
typedef u8 (*transport_read_t)(void);
typedef u8 (*transport_rx_busy_t)(void);
typedef u8 (*transport_rx_bytes_t)(void);
typedef u8* (*transport_rx_buf_t)(void);
typedef void (*transport_setup_t)(void);
typedef void (*transport_disconnect_t)(void);

//------ extern variables

// current transport type
extern eMonomeTransport monome_transport_type;

//------ extern function declarations

// transport management
extern void monome_transport_init(void);
extern void monome_transport_set(eMonomeTransport transport);
extern eMonomeTransport monome_transport_get(void);

// unified transport interface
extern u8 monome_transport_write(u8* data, u8 bytes);
extern u8 monome_transport_tx_busy(void);
extern u8 monome_transport_read(void);
extern u8 monome_transport_rx_busy(void);
extern u8 monome_transport_rx_bytes(void);
extern u8* monome_transport_rx_buf(void);
extern void monome_transport_setup(void);
extern void monome_transport_disconnect(void);

// transport-specific setup functions
extern void monome_transport_setup_ftdi(void);
extern void monome_transport_setup_cdc(void);

#endif // _ALEPH_MONOME_TRANSPORT_H_