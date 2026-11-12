#include "g923_protocol.h"

const uint8_t g923_init_packet[G923_INIT_PACKET_SIZE] = {
    0x0F, 0x00, 0x01, 0x01, 0x42
};

int g923_is_initialization_packet(const uint8_t *data, size_t length)
{
    size_t i;

    if (data == NULL || length != G923_INIT_PACKET_SIZE)
        return 0;

    for (i = 0; i < G923_INIT_PACKET_SIZE; ++i) {
        if (data[i] != g923_init_packet[i])
            return 0;
    }
    return 1;
}
