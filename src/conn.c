
#include "common.h"

static const uint8_t magicPacket[] = {
	0x7e, 0x3d, 0x71, 0x0b, 0x96, 0x5f, 0x11, 0xe5, 0x92, 0x56, 0x28, 0x20, 
};

static PacketType packetType;

typedef struct {
	uint32_t counter;
	uint8_t connId[12];
} ConnState;
static ConnState connState;
static uint32_t connCounter;
static bool connValid = false;

static bool validateCatchPacket()
{
	if (packetLength < 2 + sizeof(magicPacket))
	{
		return false;
	}
	return compareMem(&buffer[2], magicPacket, sizeof(magicPacket));
}


static bool decodeRequestPacket()
{
	uint32_t counter;
	uint32_t oldCouter;
	int32_t diff;

	// validate state and buffer length
	if (!connValid || packetLength < 17 || packetLength > sizeof(packetBuffer[0]))
	{
		return false;
	}

	// calculate current buffer counter based on previous value and lower half of current value
	counter = buffer[2];
	counter |= (uint32_t)buffer[3] << 8;
	oldCouter = connCounter & 0xFFFF;
	if (oldCouter < 0x4000 && counter > 0xC000)
	{
		// if counter lower half increased too much it is an error
		return false;
	}
	else if (oldCouter > 0xC000 && counter < 0x4000)
	{
		// if counter lower half decreased too much then it is half-word overflow
		counter |= (connCounter + 0x10000) & 0xFFFF0000;
	}
	else
	{
		// in other cases use higher half directly
		counter |= connCounter & 0xFFFF0000;
	}

	// make sure that counter increased correcly
	diff = counter - connCounter;
	if (diff <= 0 || diff > CONN_COUNTER_DIFF_MAX)
	{
		return false;
	}

	// decrypt buffer
	connState.counter = counter;
	aes_dcfb(&buffer[4], packetLength - 4, AES_DCFB_DECRYPT, (uint8_t*)&connState);

	// do integrity check
	if (!compareMem(buffer, &buffer[packetLength - 12], 12))
	{
		return false;
	}

	// use new buffer counter value if buffer is valid
	connCounter = counter;
	return true;
}

EXTERN PacketType parsePacket()
{
	do
	{
		uint8_t* buffer = getPacket();
		packetType = buffer[1];
		if (packetType == PACKET_TYPE_CATCH)
		{
			if (validateCatchPacket())
			{
				return packetType;
			}
		}
		else if (packetType == PACKET_TYPE_REQUEST)
		{
			if (decodeRequestPacket())
			{
				return packetType;
			}
		}
	} while(true);
}


EXTERN void sendResponse()
{
	connState.counter = connCounter;
	buffer[1] = PACKET_TYPE_RESPONSE;
	send(2, (uint8_t*)&connState);
}

EXTERN void sendCaught(size_t size)
{
	buffer[1] = PACKET_TYPE_CAUGHT;
	send(22, &buffer[6]);
}

EXTERN uint8_t* getConnState()
{
	connState.counter = connCounter;
	return (uint8_t*)&connState;
}
