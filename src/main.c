#include "common.h"


#define IDLE_TIMEOUT             MS2TICKS(200)
#define CAUGHT_DELAY_TIMEOUT_MIN MS2TICKS(1)
#define CAUGHT_DELAY_TIMEOUT_MAX MS2TICKS(18)
#define CAUGHT_TIMEOUT           MS2TICKS(1000)
#define RUNNING_TIMEOUT          MS2TICKS(9000)

static uint32_t randCaughtDelay()
{
	return CAUGHT_DELAY_TIMEOUT_MIN + randGet32() % (CAUGHT_DELAY_TIMEOUT_MAX - CAUGHT_DELAY_TIMEOUT_MIN + 1);
}

static void stateMachine()
{
	PacketType type;
	enum { IDLE = 0, CAUGHT_DELAY = 1, CAUGHT = 2, RUNNING = 3 } state = IDLE;
	uint32_t timeout = IDLE_TIMEOUT;
	do {
		if (!recv(timeout)) {
			if (state == CAUGHT_DELAY) {
				//sendCaughtPacket();
				timeout = CAUGHT_TIMEOUT;
				state = CAUGHT;
			} else {
				shutdown();
			}
		} else {
			type = parsePacket();
			if (type == PACKET_TYPE_CATCH) {
				if (state & 1) { // CAUGHT_DELAY or RUNNING
					continue;
				} else {
					timeout = getRecvTime() + randCaughtDelay();
					state = CAUGHT_DELAY;
				}
			} else if (type == PACKET_TYPE_REQUEST) {
				timeout = getRecvTime() + RUNNING_TIMEOUT;
				state = RUNNING;
			} else if (type == PACKET_TYPE_REQUEST_START) {
				return;
			}
		}
	} while (true);
}


EXTERN void main()
{
    initTimer();
    initRadio();
	stateMachine();
	shutdownRadio();
	shutdownTimer();
}

__attribute__((noreturn))
EXTERN void shutdown()
{
	shutdownRadio();
	shutdownTimer();
	startMbr();
}
