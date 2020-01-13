
TARGET = sensor

INCLUDE = \
	-I. \
	-I../nrfx/mdk \
	-I../CMSIS/CMSIS/Core/Include \
	-I../FreeRTOS/FreeRTOS/Source/include \

# C and C++ flags
CPPFLAGS   = \
	-O0 -g3 -fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-strict-aliasing -fshort-enums \
	-D__HEAP_SIZE=8192 \
	-D__STACK_SIZE=8192

# C flags
CFLAGS   = 

# C++ flags
CXXFLAGS   = 

# Assembler flags
ASMFLAGS =

LIBS = \
	-L../nrfx/mdk \
	-lc \
	-lnosys \
	-lm

OBJ = test1.o \
	../app/worker.o \
	../app/timer.o \
	../app/fast_timer.o \
	../FreeRTOS/FreeRTOS/Source/list.o \
	../FreeRTOS/FreeRTOS/Source/queue.o \
	../FreeRTOS/FreeRTOS/Source/tasks.o \

#OBJ = \
	main.o \
	worker.o \
	timer.o \
	../FreeRTOS/FreeRTOS/Source/list.o \
	../FreeRTOS/FreeRTOS/Source/queue.o \
	../FreeRTOS/FreeRTOS/Source/tasks.o \

	# \
	fast_timer.o \
	onewire.o \
	dht22.o \
	
	#../FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.o

#--- NRF52832_XXAA
#CPUFLAGS = -mthumb -mabi=aapcs -T../mdk/nrf52_xxaa.ld -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 --specs=nano.specs -DNRF52832_XXAA -DFLOAT_ABI_HARD
#OBJ     := ../nrfx/mdk/gcc_startup_nrf52.o ../nrfx/mdk/system_nrf52.o $(OBJ)
#OBJ     += ../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.o
#INCLUDE += -I../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F

#--- nRF52840_XXAA
# CPUFLAGS = -mthumb -mabi=aapcs -Tnrf52840_xxaa.ld -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 --specs=nano.specs -DNRF52840_XXAA -DFLOAT_ABI_HARD
# OBJ     := ../nrfx/mdk/gcc_startup_nrf52840.o ../nrfx/mdk/system_nrf52840.o $(OBJ)
# OBJ     += ../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.o
# INCLUDE += -I../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F


#--- NRF51422_XXAC
CPUFLAGS = -mthumb -mabi=aapcs -T../mdk/nrf51_xxac.ld -mcpu=cortex-m0 -mfloat-abi=soft --specs=nano.specs -DNRF51422_XXAC -DFLOAT_ABI_SOFT
OBJ     := ../nrfx/mdk/gcc_startup_nrf51.o ../nrfx/mdk/system_nrf51.o $(OBJ)
OBJ     += ../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM0/port.o
INCLUDE += -I../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM0

GCC_VERSION := $(shell arm-none-eabi-gcc --version 2>/dev/null)
NRFJPROG_VERSION := $(shell nrfjprog --version 2>/dev/null)

ifdef GCC_VERSION
	GCC_PATH=
else
    GCC_PATH=../binutils/gcc/bin/
endif

ifdef NRFJPROG_VERSION
	NRFJPROG_PATH=
else
    NRFJPROG_PATH=../binutils/nrfjprog/
endif

all: $(TARGET).elf

clean:
	rm -f $(OBJ) $(TARGET).elf $(TARGET).lst $(TARGET).map $(TARGET).hex

.cpp.o: Makefile
	$(GCC_PATH)arm-none-eabi-c++ -c $(CPUFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDE) -o $@ $<

.c.o: Makefile
	$(GCC_PATH)arm-none-eabi-gcc -c $(CPUFLAGS) $(CPPFLAGS) $(CFLAGS) $(INCLUDE) -o $@ $<

.S.o: Makefile
	$(GCC_PATH)arm-none-eabi-gcc -c $(CPUFLAGS) $(CPPFLAGS) $(ASMFLAGS) $(INCLUDE) -o $@ $<

$(TARGET).elf: $(OBJ) Makefile
	$(GCC_PATH)arm-none-eabi-c++ $(CPUFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LIBS) -Wl,-Map=$(TARGET).map -o $(TARGET).elf $(OBJ)
	$(GCC_PATH)arm-none-eabi-objdump -d $(TARGET).elf > $(TARGET).lst
	$(GCC_PATH)arm-none-eabi-objcopy -O ihex $(TARGET).elf $(TARGET).hex

flash: $(TARGET).elf
	$(NRFJPROG_PATH)nrfjprog --program $(TARGET).hex --chiperase
	$(NRFJPROG_PATH)nrfjprog --reset
