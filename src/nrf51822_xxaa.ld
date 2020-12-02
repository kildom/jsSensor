/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
    PAGE0 (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00400 /* Code neccessary for corretly working bootloader and self program of page 1. */
    PAGE1 (rx)  : ORIGIN = 0x00000400, LENGTH = 0x003C0 /* Self program of page 0 and code that is optional. */
    CONF  (r)   : ORIGIN = 0x000007C0, LENGTH = 0x00040 /* Bootloader configuration. */
    RAM   (rw)  : ORIGIN = 0x20000000, LENGTH = 0x03FF8 /* RAM app on the beginning and the rest of data. Last 8 bytes are not included and reserved for write protection pattern. */
    EMPTY (rwx) : ORIGIN = 0x30000000, LENGTH = 0x00000 /* .data section is not required, so it goes to EMPTY memory to make sure that .data is not used. */
}

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

ENTRY(resetHandler)

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        KEEP(*(.selfProgPage1*))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > PAGE0

    .page1.text :
    {
        KEEP(*(.selfProgPage0*))
        *(.page1.text*)
        *(.page1.rodata*)
        . = ALIGN(4);
    } > PAGE1

    .conf :
    {
        KEEP(*(.conf*))
    } > CONF

    .app :
    {
        KEEP(*(.app_entry*))
        KEEP(*(.app_area*))
    } > RAM

    .bss :
    {
        __begin_rambss = .;
        *(.bss*)
        *(COMMON)
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
    } > EMPTY
}
