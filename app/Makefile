
# 'C:/Program Files (x86)/GNU Tools ARM Embedded/7 2018-q2-update/bin/arm-none-eabi-gcc' -O3 -g3 -mthumb -mabi=aapcs -L../../../../../../modules/nrfx/mdk -TMbedTLS_threading_test_FreeRTOS_gcc_nrf52.ld -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections --specs=nano.specs @_build/nrf52840_xxaa.in -Wl,-Map=_build/nrf52840_xxaa.map -o _build/nrf52840_xxaa.out


TARGET   = sensor
INCLUDE  = -I../mdk -I../CMSIS/CMSIS/Core/Include \
	-I../FreeRTOS/Source/include \
	-I. \
	-I../FreeRTOS/Source/portable/GCC/ARM_CM0
CFLAGS   = -O0 -g3 -fdata-sections -ffunction-sections -Wl,--gc-sections -fno-strict-aliasing -fshort-enums -D__HEAP_SIZE=8192 -D__STACK_SIZE=8192
LIBS     = -L../mdk -lc -lnosys -lm

OBJ      = main.o \
	../FreeRTOS/Source/list.o \
	../FreeRTOS/Source/queue.o \
	../FreeRTOS/Source/tasks.o \
	../FreeRTOS/Source/portable/GCC/ARM_CM0/port.o

CPUFLAGS = -mthumb -mabi=aapcs -T../mdk/nrf52_xxaa.ld -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 --specs=nano.specs -DNRF52832_XXAA -DFLOAT_ABI_HARD
OBJ     := ../mdk/gcc_startup_nrf52.o ../mdk/system_nrf52.o $(OBJ)

GCC_VERSION := $(shell arm-none-eabi-gcc --version 2>/dev/null)

ifdef GCC_VERSION
	GCC_PATH=
else
    GCC_PATH=../gcc/bin/
endif

all: $(TARGET).elf

clean:
	rm -f $(OBJ) $(TARGET).elf $(TARGET).lst $(TARGET).hex

.c.o: Makefile
	$(GCC_PATH)arm-none-eabi-gcc -c $(CPUFLAGS) $(CFLAGS) $(INCLUDE) -o $@ $<

.S.o: Makefile
	$(GCC_PATH)arm-none-eabi-gcc -c $(CPUFLAGS) $(CFLAGS) $(INCLUDE) -o $@ $<

$(TARGET).elf: $(OBJ) Makefile
	$(GCC_PATH)arm-none-eabi-gcc $(CPUFLAGS) $(CFLAGS) $(LIBS) -Wl,-Map=$(TARGET).map -o $(TARGET).elf $(OBJ)
	$(GCC_PATH)arm-none-eabi-objdump -d $(TARGET).elf > $(TARGET).lst
	$(GCC_PATH)arm-none-eabi-objcopy -O ihex $(TARGET).elf $(TARGET).hex

flash: $(TARGET).elf
	nrfjprog --program $(TARGET).hex --chiperase
	nrfjprog --reset
