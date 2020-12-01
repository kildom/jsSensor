#ifndef _RADIO_H_
#define _RADIO_H_

typedef enum {
    PACKET_TYPE_CATCH         = 0x00,
    PACKET_TYPE_CAUGHT        = 0x01,
    PACKET_TYPE_REQUEST       = 0x02,
    PACKET_TYPE_RESPONSE      = 0x03,
    PACKET_TYPE_REQUEST_START = 0x7F,
} PacketType;

EXTERN void initRadio();
EXTERN void shutdownRadio();
EXTERN PacketType recv(uint32_t timeout);
EXTERN uint32_t recvTime();
EXTERN uint32_t getPacketSize();
EXTERN void setPacketSize(uint32_t size);
EXTERN uint8_t* getPacket();
EXTERN void send(size_t plainSize, uint8_t* iv);
EXTERN uint8_t* getConnState();

#endif
