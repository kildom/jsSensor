#ifndef _CONF_H_
#define _CONF_H_

#ifdef NRF51822_XXAA
#   ifdef WITH_SOFT_DEVICE
#       define _CNF_SEL_(a, X, ...) X
#   else
#       define _CNF_SEL_(X, ...) X
#   endif
#elif defined(NRF52832_XXAA)
#   ifdef WITH_SOFT_DEVICE
#       define _CNF_SEL_(a, b, c, X, ...) X
#   else
#       define _CNF_SEL_(a, b, X, ...) X
#   endif
#else
#   error Not implemented for this device
#endif

/*                                 51822   51822   52832   52832
 *                                         +SD             +SD     */
#define ISR_VECTOR_ITEMS _CNF_SEL_(48,     0,      2,      0       )
#define APP_START_ADDR   _CNF_SEL_(0x1000, 0x1000, 0x1000, 0x1000  )

#endif