#if defined(NRF51422_XXAC)
#include "portmacro_m0.h"
#elif defined(NRF52832_XXAA)
#include "portmacro_m4.h"
#else
#error "Not implemented or missing NRF5xxxxx macro"
#endif
