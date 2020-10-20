/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH      (rx)  : ORIGIN = 0x00000000, LENGTH = 0x003C0
  CONF_FLASH (r)   : ORIGIN = 0x000003C0, LENGTH = 0x00040
  RAM        (rw)  : ORIGIN = 0x20000000, LENGTH = 0x04000
  EMPTY      (rwx) : ORIGIN = 0x30000000, LENGTH = 0x00000
}


INCLUDE "nrf_common.ld"
