#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "nrf.h"

#define MS2TICKS(x) ((x) * 1000)
#define CAUGHT_MIN_DELAY MS2TICKS(1)
#define CAUGHT_MAX_DELAY MS2TICKS(17)

#define BLOCK_SIZE 128

#define STACK_SIZE 512
#define APP_SIZE (RAM_SIZE - 768)

typedef ;

#define APP_BLOCKS (APP_SIZE / BLOCK_SIZE)

__attribute__((section(".app_region")))
uint8_t appRegion[APP_BLOCKS * BLOCK_SIZE];

void appEntry(void* connInfo, const void* key) __attribute__((alias("appRegion")));

uint8_t appValidMask[(APP_BLOCKS + 7) / 8];

#pragma region Initialization

NOINIT_DATA
uint32_t stack[STACK_SIZE / 4];

void Reset_Handler()
{
	int main();
	extern uint32_t __begin_code_ram_src;
	extern uint32_t __begin_code_ram_dst;
	extern uint32_t __size_to_copy;
	extern uint32_t __begin_rambss;
	extern uint32_t __size_rambss;

	memCopyAligned_F(&__begin_code_ram_dst, &__begin_code_ram_src, (size_t)&__size_to_copy);
	memZeroAligned_F(&__begin_rambss, (size_t)&__size_rambss);

	//SystemInit();

	main();

	{
		uint32_t connInfo[4];
		memcpy(&connInfo, randData.connId, 16);
		appEntry(connInfo, conf.key);
	}
}

__attribute__((used))
__attribute__((section(".isr_vector")))
const void* __isr_vector[ISR_VECTOR_ITEMS] = {
	&stack[sizeof(stack) / sizeof(stack[0]) - 1],
	Reset_Handler,
	// Magic values to detect valid bootloader binary before uploading.
	// They will be replaced by the application ISR vectors.
	(void*)0x41dec8ba, (void*)0x0230a515, (void*)0x0f1955be, (void*)0x1aee5b4d,
	(void*)0x2061ac4c, (void*)0x18d4f330, (void*)0x31beca84, (void*)0x11fa6d52,
};


#pragma endregion

#pragma region Configuration

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

#pragma endregion

#pragma region Radio

static void radioInit()
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

static int radioRecv(bool useTimeout, uint32_t timeout)
{

}

#pragma endregion

#pragma region AES

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

#pragma endregion

/*

Startup procedure (before C startup):
	if reset reason == power on then goto boot
	if app invalid (APP_AREA[0..1] == 0xFFFFFFFFFFFF) then goto boot
	start app

start app on nRF52:
	// BOOT_ISR_VECT covers only first two vectors
	VTOR = &APP_ISR_VECT[0]
	change SP = APP_ISR_VECT[0]
	change PC = APP_ISR_VECT[1]

start app on nRF51 (alternative with faster ISR entry):
	if (BOOT_ISR_VECT[2..N] != APP_ISR_VECT[2..N])
		memcpy: buffer = BOOT_ISR_VECT[0..1] & APP_ISR_VECT[2..N] & rest_of_bootloader_flash
		jump to executing from RAM
		erase bootloader pages
		memcpy: bootloader = buffer
		jump back to FLASH
	change SP = APP_ISR_VECT[0]
	change PC = APP_ISR_VECT[1]

start app on nRF51 (safer alternative):
	// BOOT_ISR_VECT[2..N] points to one function that reads APP_ISR_VECT and jumps to actual implementation.
	change SP = APP_ISR_VECT[0]
	change PC = APP_ISR_VECT[1]
	
*/

#pragma region Random

static struct {
	uint8_t key[16];
	uint8_t plain[16];
	uint8_t iv[16];
	uint8_t connId[12];
	uint32_t cnt;
} randData;
static uint8_t* const randPtr = randData.iv;
static size_t randDataIndex = 0;

static void randPrepare()
{
	int i;
	uint8_t* ptr = (uint8_t*)randData.key;
	NRF_RNG->TASKS_START = 1;
	for (i = 0; i < sizeof(randData) - 16; i++) {
		while (!NRF_RNG->EVENTS_VALRDY) {
			__WFE();
		}
		NRF_RNG->EVENTS_VALRDY = 0;
		*ptr++ = NRF_RNG->VALUE;
	}
	NRF_RNG->TASKS_STOP = 1;
	doAES(randData.plain);
	doAES(randData.key);
}

static uint32_t randCaughtDelay()
{
	randDataIndex = (randDataIndex + 1) & 15;
	uint32_t rand = randPtr[randDataIndex];  // Fixed point 0.8
	uint32_t range = (CAUGHT_MAX_DELAY - CAUGHT_MIN_DELAY) << 8; // Fixed point 15.8
	uint32_t rand2 = rand * range; // Fixed point 15.16
	return CAUGHT_MIN_DELAY + (rand2 >> 16); // Fixed point 15.0
}

#pragma endregion

#pragma region Timer

static void timerInit()
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

#pragma endregion

enum {
	PACKET_TYPE_CATCH = 0,
	PACKET_TYPE_CAUGHT = 1,
	PACKET_TYPE_REQUEST = 2,
	PACKET_TYPE_RESPONSE = 3,
	PACKET_TYPE_TIMEOUT = 0x100,
};

uint8_t *radioPacket;
size_t radioPacketLen;
bool caughtSend = false;

const uint8_t packetCatch[11] = {
	PACKET_TYPE_CATCH,
	0x11, 0x79, 0x15, 0xac, 0xfe, 0x7b, 0x4e, 0x1d, 0x2a, 0x77,
};


#define ENCRYPT_ADDED 12
#define AES_DCTF_ENCRYPT 0
#define AES_DCTF_DECRYPT 48


void aes_dctf(uint8_t* data, size_t size, uint32_t mode, const uint8_t* iv){}

uint8_t caughtPacket[48];


static void encryptPacket(uint8_t* packet, uint8_t* iv, size_t offset, size_t len)
{
	uint8_t* from = packet;
	uint8_t* to = &packet[offset + len];
	uint8_t* end = from + 12;

	while (from < end) {
		*to++ = *from++;
	}
	aes_dctf(&packet[offset], len + 12, AES_DCTF_ENCRYPT, iv);
}


static void prepareCaughtPacket()
{
	uint32_t id[2];
	id[0] = NRF_FICR->DEVICEADDR[0];
	id[1] = NRF_FICR->DEVICEADDR[1] & 0xFFFF;

	/* Offset Size   Content
	 * ------ ----   -------
	 * 0      1      PACKET_TYPE_CAUGHT
	 * 1      1      Reserved, always 0
	 * 2      6      device id
	 * 8      12     iv
	 * 20     12     conn_id
	 * 32     4      cnt
	 * 36     12     verification data: copy of beginning
	 * 48     -      [size]
	 */

	caughtPacket[0] = PACKET_TYPE_CAUGHT;
	caughtPacket[1] = 0;
	memcpy(&caughtPacket[2], id, 6);
	memcpy(&caughtPacket[8], &randData.iv[4], 12 + 12 + 4);
	encryptPacket(caughtPacket, &caughtPacket[4], 20, 12 + 4);
}


bool appValid = false; // TODO: detect at startup

#define IDLE_TIMEOUT MS2TICKS(200)
#define CAUGHT_DELAY_TIMEOUT_MIN MS2TICKS(1)
#define CAUGHT_DELAY_TIMEOUT_MAX MS2TICKS(18)
#define CAUGHT_TIMEOUT MS2TICKS(1000)
#define RUNNING_TIMEOUT MS2TICKS(9000)

uint32_t randCaughtDelay()
{
	return CAUGHT_DELAY_TIMEOUT_MIN + randGet32() % (CAUGHT_DELAY_TIMEOUT_MAX - CAUGHT_DELAY_TIMEOUT_MIN + 1);
}

void stateMachine()
{
	enum { IDLE = 0, CAUGHT_DELAY = 1, CAUGHT = 2, RUNNING = 3 } state = IDLE;
	uint32_t timeout = IDLE_TIMEOUT;
	do {
		PacketType type = radioRecv(appValid || state == CAUGHT_DELAY, timeout);
		if (type == PACKET_TYPE_TIMEOUT) {
			if (state == CAUGHT_DELAY) {
				sendCaughtPacket();
				timeout = CAUGHT_TIMEOUT;
				state = CAUGHT;
			} else {
				startFlashApp();
			}
		} else if (type == PACKET_TYPE_CATCH) {
			if (state & 1) { // CAUGHT_DELAY or RUNNING
				continue;
			} else {
				if (state == IDLE) {
					randPrepare();
					prepareCaughtPacket();
				}
				timeout = radioTime() + randCaughtDelay();
				state = CAUGHT_DELAY;
			}
		} else if (type == PACKET_TYPE_REQUEST) {
			if (executeRequest()) {
				return;
			}
			timeout = radioTime() + RUNNING_TIMEOUT;
			state = RUNNING;
		}
	} while (true);
}

void main()
{
    radioInit();
    timerInit();
	stateMachine();
}
