
TARGET = sensor

INCLUDE = \
	-I. \
	-I../nrfx/mdk \
	-I../CMSIS/CMSIS/Core/Include \
	-I../FreeRTOS/FreeRTOS/Source/include \
	-I../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM0

CFLAGS   = \
	-O0 -g3 -fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-strict-aliasing -fshort-enums \
	-D__HEAP_SIZE=8192 \
	-D__STACK_SIZE=8192

LIBS = \
	-L../nrfx/mdk \
	-lc \
	-lnosys \
	-lm

OBJ = \
	main.o \
	worker.o \
	timer.o \
	fast_timer.o \
	onewire.o \
	dht22.o \
	../FreeRTOS/FreeRTOS/Source/list.o \
	../FreeRTOS/FreeRTOS/Source/queue.o \
	../FreeRTOS/FreeRTOS/Source/tasks.o \
	../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM0/port.o
	
	#../FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.o

#--- NRF52832_XXAA
#CPUFLAGS = -mthumb -mabi=aapcs -T../mdk/nrf52_xxaa.ld -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 --specs=nano.specs -DNRF52832_XXAA -DFLOAT_ABI_HARD
#OBJ     := ../mdk/gcc_startup_nrf52.o ../mdk/system_nrf52.o $(OBJ)

#--- NRF51422_XXAC
CPUFLAGS = -mthumb -mabi=aapcs -T../mdk/nrf51_xxac.ld -mcpu=cortex-m0 -mfloat-abi=soft --specs=nano.specs -DNRF51422_XXAC -DFLOAT_ABI_SOFT
OBJ     := ../nrfx/mdk/gcc_startup_nrf51.o ../nrfx/mdk/system_nrf51.o $(OBJ)

GCC_VERSION := $(shell arm-none-eabi-gcc --version 2>/dev/null)
NRFJPROG_VERSION := $(shell nrfjprog --version 2>/dev/null)

ifdef GCC_VERSION
	GCC_PATH=
else
    GCC_PATH=../gcc/bin/
endif

ifdef NRFJPROG_VERSION
	NRFJPROG_PATH=
else
    NRFJPROG_PATH=../nrfjprog/
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
	$(NRFJPROG_PATH)nrfjprog --program $(TARGET).hex --chiperase
	$(NRFJPROG_PATH)nrfjprog --reset
