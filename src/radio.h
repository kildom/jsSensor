#ifndef _RADIO_H_
#define _RADIO_H_

EXTERN void initRadio();
EXTERN void shutdownRadio();
EXTERN uint8_t* getPacket();
EXTERN uint32_t getPacketSize();
EXTERN void setPacketSize(uint32_t size);
EXTERN uint32_t getRecvTime();
EXTERN bool recv(uint32_t timeout);
EXTERN void send(size_t plainSize, uint8_t* iv);

#endif
