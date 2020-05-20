#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "nrf.h"

FLASH_FUNC
static void memCopyAligned_F(void* dst, const void* src, size_t size) {
	uint32_t* d = dst;
	const uint32_t* s = src;
	uint32_t* e = (uint32_t*)((uint8_t*)dst + size);
	while (d < e)
	{
		*d++ = *s++;
	}
}

static void memCopyAligned(void* dst, const void* src, size_t size) {
	uint32_t* d = dst;
	const uint32_t* s = src;
	uint32_t* e = (uint32_t*)((uint8_t*)dst + size);
	while (d < e)
	{
		*d++ = *s++;
	}
}

FLASH_FUNC
static void memZeroAligned_F(void* dst, size_t size) {
	uint32_t* d = dst;
	uint32_t* e = (uint32_t*)((uint8_t*)dst + size);
	while (d < e)
	{
		*d++ = 0;
	}
}

FLASH_FUNC
void Reset_Handler()
{
	int main();
	extern uint32_t __begin_code_ram_src;
	extern uint32_t __begin_code_ram_dst;
	extern uint32_t __size_to_copy;
	extern uint32_t __begin_rambss;
	extern uint32_t __size_rambss;

	memCopyAligned_F(&__begin_code_ram_dst, &__begin_code_ram_src, (size_t)&__size_to_copy);
	// TODO: No need for a new function, because memset is available at this point
	memZeroAligned_F(&__begin_rambss, (size_t)&__size_rambss);

	//SystemInit();

	main();
}

__attribute__((used))
__attribute__((section(".isr_vector")))
const void* __isr_vector[ISR_VECTOR_ITEMS] = {
	(void*)BEGIN_STACK,
	Reset_Handler,
	// Magic values to detect valid bootloader binary before uploading.
	// They will be replaced by the application ISR vectors.
	(void*)0x41dec8ba, (void*)0x0230a515, (void*)0x0f1955be, (void*)0x1aee5b4d,
	(void*)0x2061ac4c, (void*)0x18d4f330, (void*)0x31beca84, (void*)0x11fa6d52,
};


__attribute__((used))
__attribute__((section(".conf")))
const struct {
	uint8_t key[32];
	uint8_t nameLength;
	char name[64 - 32 - 1];
} conf = {
	.nameLength = 20,
	.name = "Uninitialized device",
};


FLASH_FUNC
static void radioInit_F()
{
	NRF_RADIO->SHORTS = 0; // TODO:
	NRF_RADIO->INTENSET = 0; // TODO:
	NRF_RADIO->FREQUENCY = 2400 - RADIO_FREQUENCY_MHZ;
	NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos4dBm;
	NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit;
	NRF_RADIO->PCNF0 = (8 << RADIO_PCNF0_LFLEN_Pos)
		| (0 << RADIO_PCNF0_S0LEN_Pos)
		| (0 << RADIO_PCNF0_S1LEN_Pos);
	NRF_RADIO->PCNF1 = (192 << RADIO_PCNF1_MAXLEN_Pos)
		| (0 << RADIO_PCNF1_STATLEN_Pos)
		| (4 << RADIO_PCNF1_BALEN_Pos)
		| (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos)
		| (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos);
}


static void doAES(void* data)
{
	NRF_ECB->ECBDATAPTR = (uint32_t)data;
	NRF_ECB->TASKS_STARTECB = 1;
	while (!NRF_ECB->EVENTS_ENDECB)
	{
		__WFE();
	}
	NRF_ECB->EVENTS_ENDECB = 0;
}


static struct {
	uint32_t key[4];
	uint32_t plain[4];
	uint32_t connId[4];
	uint32_t iv[4];
} randBuffer;
static uint8_t* randPtr = (uint8_t*)randBuffer.iv;
static size_t randBufferIndex = 0;

FLASH_FUNC
static void randInit_F()
{
	int i;
	uint8_t* ptr = (uint8_t*)randBuffer.key;
	NRF_RNG->TASKS_START = 1;
	for (i = 0; i < sizeof(randBuffer) - 16; i++) {
		while (!NRF_RNG->EVENTS_VALRDY) {
			__WFE();
		}
		NRF_RNG->EVENTS_VALRDY = 0;
		*ptr++ = NRF_RNG->VALUE;
	}
	NRF_RNG->TASKS_STOP = 1;
	LONG_JUMP(doAES)(randBuffer.plain);
	LONG_JUMP(doAES)(randBuffer.key);
}


FLASH_FUNC
static void timerInit_F()
{
	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	NRF_TIMER0->PRESCALER = 4;
	NRF_TIMER0->TASKS_START = 1;
}

static uint32_t timerGet()
{
	NRF_TIMER0->TASKS_CAPTURE[0] = 1;
	return NRF_TIMER0->CC[0];
}


FLASH_FUNC
static uint32_t randCaughtDelay_F()
{
	#define MS2TICKS(x) ((x) * 1000)
	#define CAUGHT_MIN_DELAY MS2TICKS(1)
	#define CAUGHT_MAX_DELAY MS2TICKS(17)
	uint32_t rand = randPtr[randBufferIndex];
	randBufferIndex = (randBufferIndex + 1) & 15;
	uint32_t range = (CAUGHT_MAX_DELAY - CAUGHT_MIN_DELAY) << 8;
	return CAUGHT_MIN_DELAY + ((rand * range) >> 16);
}


void mainLoop()
{
	while (true) {

	};
}

enum {
	PACKET_TYPE_CATCH = 0,
	PACKET_TYPE_CAUGHT = 1,
	PACKET_TYPE_RUN = 2,
	PACKET_TYPE_RUNNING = 3,
	PACKET_TYPE_REQUEST = 4,
	PACKET_TYPE_RESPONSE = 5,
	PACKET_TYPE_TIMEOUT = 0xFF,
};

uint8_t *radioPacket;
size_t radioPacketLen;
bool caughtSend = false;

FLASH_DATA
const uint8_t packetCatch[11] = {
	PACKET_TYPE_CATCH,
	0x11, 0x79, 0x15, 0xac, 0xfe, 0x7b, 0x4e, 0x1d, 0x2a, 0x77,
};


#define ENCRYPT_ADDED 12
#define AES_DCTF_ENCRYPT 0
#define AES_DCTF_DECRYPT 48

uint8_t packetRun[1 + 8 + ENCRYPT_ADDED];

bool radioRecv(uint32_t maxTime)
{
	return NRF_TIMER0->CC[0];
}

#define timerGet_L LONG_JUMP(timerGet)
#define memcmp_L LONG_JUMP(memcmp)

__attribute__((noinline))
FLASH_FUNC
static uint32_t radioRecvCatchOrBootloader_F(uint32_t timeout)
{
	uint32_t endTime = timerGet_L() + timeout;
	while (true) {
		if (!radioRecv(endTime))
		{
			return PACKET_TYPE_TIMEOUT;
		}
		else if (radioPacketLen >= sizeof(packetCatch)
			&& memcmp_L(radioPacket, packetCatch, sizeof(packetCatch)) == 0)
		{
			break;
		}
		else if (caughtSend
			&& radioPacketLen >= sizeof(packetRun)
			&& memcmp_L(radioPacket, packetRun, sizeof(packetRun)) == 0)
		{
			break;
		}
	}
	return radioPacket[0];
}


void aes_dctf(uint8_t* data, size_t size, uint32_t mode, const uint8_t* iv){}

uint8_t caughtPacket[1 + 8 + sizeof(conf.name) + 16 + ENCRYPT_ADDED];
size_t caughtPacketLen;

uint8_t runningPacket[1 + 16 + ENCRYPT_ADDED];

static void encryptPacket(uint8_t* iv, uint8_t* buffer, size_t len)
{
	memset(&buffer[len], 0, ENCRYPT_ADDED);
	aes_dctf(buffer, len + ENCRYPT_ADDED, AES_DCTF_ENCRYPT, iv);
}

__attribute__((noinlie))
FLASH_FUNC
static void prepareCaughtPacket_F()
{
	uint32_t id[2];
	id[0] = NRF_FICR->DEVICEADDR[0];
	id[1] = NRF_FICR->DEVICEADDR[1] & 0xFFFF;

	randInit_F();

	caughtPacket[0] = PACKET_TYPE_CAUGHT;
	memcpy(&caughtPacket[1], id, 8);
	size_t nameLen = conf.nameLength <= sizeof(conf.name) ? conf.nameLength : 0;
	memcpy(&caughtPacket[1 + 8], conf.name, nameLen);
	memcpy(&caughtPacket[1 + 8 + nameLen], randBuffer.iv, 16);
	encryptPacket(randBuffer.iv, &caughtPacket[1 + 8 + 16 + nameLen], 0);

	packetRun[0] = PACKET_TYPE_RUN;
	memcpy(&packetRun[1], id, 8);
	randBuffer.iv[0] ^= 1;
	encryptPacket(randBuffer.iv, &packetRun[1], 8);

	runningPacket[0] = PACKET_TYPE_RUNNING;
	memcpy(&runningPacket[1], randBuffer.connId, 16);
	randBuffer.iv[0] ^= 3;
	encryptPacket(randBuffer.iv, &runningPacket[1], 16);
}

void radioSendCaughtPacket()
{
	prepareCaughtPacket_F();
}


bool appValid = false;

FLASH_FUNC
int main()
{
    int i;
    enum { IDLE_DELAY, CATCH_DELAY } state = IDLE_DELAY;
    uint32_t longTimeout = 1000;
    uint32_t timeout = longTimeout;

    randInit_F();
    radioInit_F();
    timerInit_F();
    prepareCaughtPacket_F();

    do {
        uint32_t type = radioRecvCatchOrBootloader_F(timeout);
        if (type == PACKET_TYPE_RUN) {
            break;
        } else if (type == PACKET_TYPE_CATCH) {
            state = CATCH_DELAY;
            timeout = randCaughtDelay_F();
        } else if (state == CATCH_DELAY) {
            radioSendCaughtPacket(); // also prepares 'run bootloader' to compare later
            state = IDLE_DELAY;
            timeout = longTimeout;
        } else if (state == IDLE_DELAY && appValid) {
            //deviceSoftReset();
        }
    } while (true);
    

	LONG_JUMP(mainLoop)();
}
