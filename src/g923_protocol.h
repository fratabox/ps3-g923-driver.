#ifndef G923_PROTOCOL_H
#define G923_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define G923_USB_VENDOR_ID  0x046D
#define G923_USB_PRODUCT_ID 0xC26D
#define G923_INIT_ENDPOINT_OUT 0x01
#define G923_INIT_PACKET_SIZE 5

extern const uint8_t g923_init_packet[G923_INIT_PACKET_SIZE];

/* Protocol evidence from the PC test; PS3 USB transport is not implemented. */
int g923_is_initialization_packet(const uint8_t *data, size_t length);

#endif
