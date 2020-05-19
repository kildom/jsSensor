/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH      (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00FC0
  CONF_FLASH (r)   : ORIGIN = 0x00000FC0, LENGTH = 0x00040
  APP_FLASH  (r)   : ORIGIN = 0x00001000, LENGTH = 0x7F000
  CODE_RAM   (rwx) : ORIGIN = 0x00800000, LENGTH = 0x10000
  RAM        (rwx) : ORIGIN = 0x20000000, LENGTH = 0x10000
}


INCLUDE "nrf_common.ld"
