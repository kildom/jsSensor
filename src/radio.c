
#include "common.h"

#define CONN_COUNTER_DIFF_MAX 1024

NOINIT_DATA static uint8_t packetBuffer[2][256];
static int bufferIndex = 0;
NOINIT_DATA static uint8_t* buffer;
NOINIT_DATA static size_t packetLength;
NOINIT_DATA static uint32_t recvTime;


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
	NRF_RADIO->PACKETPTR = (uint32_t)&packetBuffer[bufferIndex];
	bufferIndex ^= 1;
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
	NRF_RADIO->FREQUENCY = /* TODO: from config */ 24;
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

EXTERN uint8_t* getPacket()
{
    return buffer;
}

EXTERN uint32_t getPacketSize()
{
	return packetLength;
}

EXTERN void setPacketSize(uint32_t size)
{
	packetLength = size;
}

EXTERN uint32_t getRecvTime()
{
    return recvTime;
}

EXTERN bool recv(uint32_t timeout)
{
	do
	{
		// wait for incoming buffer
		if (!waitForEvent(&NRF_RADIO->EVENTS_END, timeout))
		{
			return false;
		}
		recvTime = getTime();
		// switch to second buffer as soon as possible
		NRF_RADIO->PACKETPTR = (uint32_t)&packetBuffer[bufferIndex];
		NRF_RADIO->TASKS_START = 1;
		bufferIndex ^= 1;
		// parse current buffer
		buffer = packetBuffer[bufferIndex];
		packetLength = buffer[0] + 1;
	} while (packetLength < 16 || packetLength > MAX_PACKET_SIZE);
	return true;
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
	// prepare and encrypt buffer
	buffer[0] = packetLength + 12;
	copyMem(&buffer[packetLength], buffer, 12);
	aes_dcfb(&buffer[plainSize], packetLength + 12 - plainSize, AES_DCFB_ENCRYPT, iv);
	// wait for trasmitter ready
	assertEvent(&NRF_RADIO->EVENTS_READY);
	// send buffer
	NRF_RADIO->PACKETPTR = (uint32_t)buffer;
	NRF_RADIO->TASKS_START = 1;
	assertEvent(&NRF_RADIO->EVENTS_DISABLED);
	// go back to receiving
	startReceiver();
}
