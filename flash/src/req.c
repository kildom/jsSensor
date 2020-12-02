
#include "common.h"

typedef enum {
    GET_DEVICE_INFO = 0x00,
    GET_STATUS = 0x01,
    GET_HASH = 0x02,
    START_APP = 0x03,
    WRITE_BLOCK = 0x100,
} RequestType;

static void sendDeviceInfo()
{
    uint8_t* buffer = getPacket();
    uint8_t* ptr = buffer;
    
    ptr[1] = PACKET_TYPE_RESPONSE;
    ptr += 2;

    // TODO: More info

    setPacketSize(ptr - buffer);
}

bool executeRequest()
{
    uint8_t* buffer = getPacket();
    uint32_t size = getPacketSize();
    RequestType type;

    if (size == 5 + BLOCK_SIZE)
    {
        type = WRITE_BLOCK;
    }
    else
    {
        type = buffer[4];
    }

    switch (type)
    {
    case GET_DEVICE_INFO:
        sendDeviceInfo();
        break;
    case GET_STATUS:
        sendStatus();
        break;
    case GET_HASH:
        sendHash();
        break;
    case START_APP:
        return false;
        break;
    case WRITE_BLOCK:
        writeBlock();
        break;
    default:
        break;
    }

    return true;
}