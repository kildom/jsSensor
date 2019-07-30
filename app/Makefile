
# 'C:/Program Files (x86)/GNU Tools ARM Embedded/7 2018-q2-update/bin/arm-none-eabi-gcc' -O3 -g3 -mthumb -mabi=aapcs -L../../../../../../modules/nrfx/mdk -TMbedTLS_threading_test_FreeRTOS_gcc_nrf52.ld -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections --specs=nano.specs @_build/nrf52840_xxaa.in -Wl,-Map=_build/nrf52840_xxaa.map -o _build/nrf52840_xxaa.out


TARGET   = sensor
INCLUDE  = -Imdk -ICMSIS/CMSIS/Core/Include/
CFLAGS   = -O0 -g3 -fdata-sections -ffunction-sections -Wl,--gc-sections -fno-strict-aliasing -fshort-enums -D__HEAP_SIZE=8192 -D__STACK_SIZE=8192
LIBS     = -Lmdk -lc -lnosys -lm

OBJ      = main.o

CPUFLAGS = -mthumb -mabi=aapcs -Tmdk/nrf52_xxaa.ld -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 --specs=nano.specs -DNRF52832_XXAA -DFLOAT_ABI_HARD
OBJ     := mdk/gcc_startup_nrf52.o mdk/system_nrf52.o $(OBJ)


all: $(TARGET).elf

.c.o: Makefile
	arm-none-eabi-gcc -c $(CPUFLAGS) $(CFLAGS) $(INCLUDE) -o $@ $<

.S.o: Makefile
	arm-none-eabi-gcc -c $(CPUFLAGS) $(CFLAGS) $(INCLUDE) -o $@ $<

$(TARGET).elf: $(OBJ) Makefile
	arm-none-eabi-gcc $(CPUFLAGS) $(CFLAGS) $(LIBS) -Wl,-Map=$(TARGET).map -o $(TARGET).elf $(OBJ)
	arm-none-eabi-objdump -d $(TARGET).elf > $(TARGET).lst
	arm-none-eabi-objcopy -O ihex $(TARGET).elf $(TARGET).hex

flash: $(TARGET).elf
	nrfjprog --program $(TARGET).hex --chiperase
	nrfjprog --reset