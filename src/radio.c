
#include "common.h"

#define CONN_COUNTER_DIFF_MAX 1024

static const uint8_t magicPacket[] = {
	0x7e, 0x3d, 0x71, 0x0b, 0x96, 0x5f, 0x11, 0xe5, 0x92, 0x56, 0x28, 0x20, 
};
static uint8_t packetBuffer[2][256];
static int currentPacketBuffer = 0;
static uint8_t* packet;
static PacketType packetType;
static size_t packetLength;
static uint32_t packetTime;

typedef struct {
	uint32_t counter;
	uint8_t connId[12];
} ConnState;
static ConnState connState;
static uint32_t connCounter;
static bool connValid = false;


static bool waitForEvent(volatile uint32_t *reg, uint32_t time)
{
	while (!*reg)
	{
		if (timedOut(time))
		{
			return false;
		}
	}
	*reg = 0;
	return true;
}

static void assertEvent(volatile uint32_t *reg)
{
	BL_ASSERT(waitForEvent(reg, getTime() + MS2TICKS(100)));
}

static void startReceiver()
{
	NRF_RADIO->SHORTS = 0;
	NRF_RADIO->PACKETPTR = (uint32_t)&packetBuffer[currentPacketBuffer];
	currentPacketBuffer ^= 1;
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_RXEN = 1;
	assertEvent(&NRF_RADIO->EVENTS_READY);
	NRF_RADIO->TASKS_START = 1;
}

static void stopReceiver()
{
	NRF_RADIO->TASKS_DISABLE = 1;
	assertEvent(&NRF_RADIO->EVENTS_DISABLED);
}

EXTERN void initRadio()
{
	// TODO: check configuration (and POWER register)
	NRF_RADIO->SHORTS = 0;
	NRF_RADIO->INTENSET = 0; // TODO:
	NRF_RADIO->FREQUENCY = RADIO_FREQUENCY_MHZ - 2400;
	NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos4dBm;
	NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit;
	NRF_RADIO->PCNF0 = (8 << RADIO_PCNF0_LFLEN_Pos)
		| (0 << RADIO_PCNF0_S0LEN_Pos)
		| (0 << RADIO_PCNF0_S1LEN_Pos);
	NRF_RADIO->PCNF1 = (192 << RADIO_PCNF1_MAXLEN_Pos) // TODO: set maxlen to actual buffer size
		| (0 << RADIO_PCNF1_STATLEN_Pos)
		| (4 << RADIO_PCNF1_BALEN_Pos)
		| (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos)
		| (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos);
	
	// Default state for RADIO is receiver enabled and started
	startReceiver();
}

EXTERN void shutdownRadio()
{

}

static bool validateCatchPacket()
{
	if (packetLength < 2 + sizeof(magicPacket))
	{
		return false;
	}
	return compareMem(&packet[2], magicPacket, sizeof(magicPacket));
}


static bool decodeRequestPacket()
{
	uint32_t counter;
	uint32_t oldCouter;
	int32_t diff;

	// validate state and packet length
	if (!connValid || packetLength < 17 || packetLength > sizeof(packetBuffer[0]))
	{
		return false;
	}

	// calculate current packet counter based on previous value and lower half of current value
	counter = packet[2];
	counter |= (uint32_t)packet[3] << 8;
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

	// decrypt packet
	connState.counter = counter;
	aes_dcfb(&packet[4], packetLength - 4, AES_DCFB_DECRYPT, (uint8_t*)&connState);

	// do integrity check
	if (!compareMem(packet, &packet[packetLength - 12], 12))
	{
		return false;
	}

	// use new packet counter value if packet is valid
	connCounter = counter;
	return true;
}

EXTERN PacketType recv(uint32_t timeout)
{
	do
	{
		// wait for incoming packet
		if (!waitForEvent(&NRF_RADIO->EVENTS_END, timeout))
		{
			return PACKET_TYPE_TIMEOUT;
		}
		packetTime = getTime();
		// switch to second buffer as soon as possible
		NRF_RADIO->PACKETPTR = (uint32_t)&packetBuffer[currentPacketBuffer];
		NRF_RADIO->TASKS_START = 1;
		currentPacketBuffer ^= 1;
		// parse current packet
		packet = packetBuffer[currentPacketBuffer];
		packetLength = packet[0] + 1;
		packetType = packet[1];
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

EXTERN uint32_t recvTime()
{
    return packetTime;
}

EXTERN uint32_t getPacketSize()
{
	return packetLength;
}

EXTERN void setPacketSize(uint32_t size)
{
	packetLength = size;
}

EXTERN uint8_t* getPacket()
{
    return packet;
}

EXTERN void send(size_t plainSize, uint8_t* iv)
{
	// stop receiving as fast as possible
	stopReceiver();
	// enable transmitter
	NRF_RADIO->SHORTS = RADIO_SHORTS_END_DISABLE_Msk;
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_TXEN = 1;
	// prepare and encrypt packet
	packet[0] = packetLength + 12;
	copyMem(&packet[packetLength], packet, 12);
	aes_dcfb(&packet[plainSize], packetLength + 12 - plainSize, AES_DCFB_ENCRYPT, iv);
	// wait for trasmitter ready
	assertEvent(&NRF_RADIO->EVENTS_READY);
	// send packet
	NRF_RADIO->PACKETPTR = (uint32_t)packet;
	NRF_RADIO->TASKS_START = 1;
	assertEvent(&NRF_RADIO->EVENTS_DISABLED);
	// go back to receiving
	startReceiver();
}

EXTERN void sendResponse()
{
	connState.counter = connCounter;
	packet[1] = PACKET_TYPE_RESPONSE;
	send(2, (uint8_t*)&connState);
}

EXTERN void sendCaught(size_t size)
{
	packet[1] = PACKET_TYPE_CAUGHT;
	send(22, &packet[6]);
}

EXTERN uint8_t* getConnState()
{
	connState.counter = connCounter;
	return (uint8_t*)&connState;
}
