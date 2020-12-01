
/*
Input symbols (for gcc use "-Wl,--defsym=NAME=VALUE"):
_LS_RAM_SIZE = total RAM size
*/

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH      (rx)  : ORIGIN = 0x00000000, LENGTH = 0x001000
  RAM        (rw)  : ORIGIN = 0x20000000, LENGTH = _LS_RAM_SIZE
  EMPTY      (rwx) : ORIGIN = 0x30000000, LENGTH = 0x000000
}

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

SECTIONS
{
    _LS_RAM_USAGE = 648;
    _LS_STACK_SIZE = 512;

    _LS_BL_MBR_COPY = 0x800;
    _LS_BL_CONF = 0xFB0;
    _LS_BL_MBR_CONF = 0xFF0;

    .text :
    {
        FILL(0xFF)
        KEEP(*(.isr_vector))
        . = _LS_BL_MBR_COPY;
        KEEP(*(.mbr_reset_copy))
        *(.text*)
        *(.rodata*)
        . = _LS_BL_CONF;
        KEEP(*(.conf*))
        . = _LS_BL_MBR_CONF;
    } > FLASH

    .ramapp :
    {
        __begin_ramapp = .;
        KEEP(*(.ramapp))
        . = ORIGIN(RAM) + LENGTH(RAM) - _LS_RAM_USAGE - _LS_STACK_SIZE;
        __end_ramapp = .;
    } > RAM

    .bss :
    {
        __begin_rambss = .;
        *(.bss*)
        *(COMMON*)
        . = ALIGN(4);
        __end_rambss = .;
    } > RAM

    __size_ramapp = __end_ramapp - __begin_ramapp;
    __size_rambss = __end_rambss - __begin_rambss;

    .noinit :
    {
        *(.noinit*)
        . = ALIGN(4);
        __unused_begin = .;
        . = ORIGIN(RAM) + LENGTH(RAM) - _LS_STACK_SIZE;
        __unused_end = .;
        __stack_begin = .;
        . = ORIGIN(RAM) + LENGTH(RAM);
        __stack_end = .;
    } > RAM

    .data :
    {
        *(.data*)
    } > EMPTY
}
