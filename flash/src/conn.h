#ifndef _CONN_H_
#define _CONN_H_

#include "common.h"

typedef enum {
    PACKET_TYPE_CATCH         = 0x00,
    PACKET_TYPE_CAUGHT        = 0x01,
    PACKET_TYPE_REQUEST       = 0x02,
    PACKET_TYPE_RESPONSE      = 0x03,
    PACKET_TYPE_REQUEST_START = 0x7F,
} PacketType;

EXTERN PacketType parsePacket();
EXTERN void sendCaught();
EXTERN void sendResponse();

#endif
