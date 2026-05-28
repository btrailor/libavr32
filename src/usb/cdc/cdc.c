#include "cdc.h"

// std
#include <stdint.h>

// asf
#include "uhc.h"
#include "print_funcs.h"
#include "uhi_cdc.h"

// libavr32
//#include "events.h"

static uint8_t connected = 0;

void cdc_change(uhc_device_t* dev, uint8_t plug) {
    print_dbg("\r\ncdc plug");
    if (plug) {
        connected = 1;
    }
    else {
        connected = 0;
    }
}

void cdc_setup(void) {
    connected = 1;
}

void cdc_disconnect(void) {
    connected = 0;
}

void cdc_read(void) {
    // TODO: implement CDC read
}

u8 cdc_rx_busy(void) {
    return 0;
}

volatile u8 cdc_rx_bytes(void) {
    return 0;
}

u8* cdc_rx_buf(void) {
    return NULL;
}

u8 cdc_tx_busy(void) {
    return 0;
}

u8 cdc_write(u8* data, u8 bytes) {
    // TODO: implement CDC write
    return 0;
}

u8 cdc_connected(void) {
    return connected;
}
