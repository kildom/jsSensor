/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH      (rx)  : ORIGIN = 0x00000000, LENGTH = 0x01000
  RAM        (rw)  : ORIGIN = 0x20000000, LENGTH = 0x04000
  EMPTY      (rwx) : ORIGIN = 0x30000000, LENGTH = 0x00000
}

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

ENTRY(Reset_Handler)

SECTIONS
{
    _LS_BL_CONF = 0xFC0;

    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        . = _LS_BL_CONF;
        KEEP(*(.conf*))
    } > FLASH

    .bss :
    {
        __begin_rambss = .;
        *(.bss*)
        *(COMMON*)
        . = ALIGN(4);
        __end_rambss = .;
    } > RAM

    __size_rambss = __end_rambss - __begin_rambss;

    .noinit :
    {
        *(.noinit*)
    } > RAM

    .data :
    {
        *(.data*)
        *(.mbr_reset_copy)
    } > EMPTY
}
