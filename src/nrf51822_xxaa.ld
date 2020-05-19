/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH      (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00BC0
  CONF_FLASH (r)   : ORIGIN = 0x00000BC0, LENGTH = 0x00040
  APP_FLASH  (r)   : ORIGIN = 0x00000C00, LENGTH = 0x3F400
  CODE_RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x04000
  RAM        (rwx) : ORIGIN = 0x20000000, LENGTH = 0x04000
}


INCLUDE "nrf_common.ld"
