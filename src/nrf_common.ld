OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

ENTRY(Reset_Handler)

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .flashtext :
    {
        *(.flashtext*)
        *(.flashrodata*)
        . = ALIGN(4);
    } > FLASH

    __begin_code_ram_src = .;
    .text : AT(__begin_code_ram_src)
    {
        __begin_code_ram_dst = .;
        *(.text*)
        . = ALIGN(4);
        __end_code_ram_dst = .;
    } > CODE_RAM

    __size_code_ram = __end_code_ram_dst - __begin_code_ram_dst;

    ._empty_dummy_shift_ :
    {
        . = __end_code_ram_dst - ORIGIN(CODE_RAM) + ORIGIN(RAM);
    } > RAM

    __begin_data_src = __begin_code_ram_src + __size_code_ram;
    .data : AT(__begin_data_src)
    {
        __begin_data_dst = .;
        *(.rodata*)
        *(.data*)
        . = ALIGN(4);
        __end_data_dst = .;
    } > RAM

    __size_to_copy = __size_code_ram + __end_data_dst - __begin_data_dst;

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

    .conf :
    {
        KEEP(*(.conf*))
    } > CONF_FLASH

    __begin_boot_flash = ORIGIN(FLASH);
    __size_boot_flash = LENGTH(FLASH) + LENGTH(CONF_FLASH);
    __begin_app_flash = ORIGIN(APP_FLASH);
    __size_app_flash = LENGTH(APP_FLASH);
    __begin_stack = ORIGIN(RAM) + LENGTH(RAM) - 4;
}
