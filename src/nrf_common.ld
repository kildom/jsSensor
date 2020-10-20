OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .conf :
    {
        KEEP(*(.conf*))
    } > CONF_FLASH

    .app :
    {
        KEEP(*(.app*))
    }

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
