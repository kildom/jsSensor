#!/bin/bash
#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Uncomment and fill following lines or provide following variables before
# running this script.
#GCC_ARM_BIN_DIR= #Directory containing C compiler "arm-none-eabi-gcc".
#NRFX_DIR= #Directory containing nrfx
#CMSIS_DIR= #Directory containing CMSIS
#NRFJPROG_DIR= #Directory containing nrfjprog

TARGET=blinky
#PLATFORM=NRF52832_XXAA
PLATFORM=NRF51822_XXAA

build() {
	find_CC
	find_NRFX
	find_CMSIS

	SOURCE_FILES="./src/main.c
		./src/startup.c"
	INCLUDE="-I$NRFX/mdk -I$CMSIS"
	LIBS="-L./src"
	CFLAGS="-Os -g3 -fdata-sections -ffunction-sections -Wl,--gc-sections
		-Wall -fno-strict-aliasing -fshort-enums
		-mthumb -mabi=aapcs
		--specs=nosys.specs -nostdlib
		"

	CFLAGS_NRF52832_XXAA="
		-mcpu=cortex-m4 -march=armv7e-m
		-mfloat-abi=hard -mfpu=fpv4-sp-d16
		-Tnrf52832_xxaa.ld
		-DNRF52
		-DNRF52832_XXAA
		"
	SOURCE_FILES_NRF52832_XXAA="
		./src/system_nrf52.c"

	CFLAGS_NRF51822_XXAA="
		-mcpu=cortex-m0
		-mfloat-abi=soft
		-Tnrf51822_xxaa.ld
		-DNRF51
		-DNRF51822_XXAA
		"
	SOURCE_FILES_NRF51822_XXAA="
		./src/system_nrf51.c"

	CFLAGS_PLATFORM=CFLAGS_${PLATFORM}
	SOURCE_FILES_PLATFORM=SOURCE_FILES_${PLATFORM}

	echo "Compiling ${TARGET}..."
	echo $CC $CFLAGS ${!CFLAGS_PLATFORM} $INCLUDE $LIBS $SOURCE_FILES ${!SOURCE_FILES_PLATFORM} -Wl,-Map=${TARGET}.map -o ${TARGET}.elf
	     $CC $CFLAGS ${!CFLAGS_PLATFORM} $INCLUDE $LIBS $SOURCE_FILES ${!SOURCE_FILES_PLATFORM} -Wl,-Map=${TARGET}.map -o ${TARGET}.elf
	echo $OBJDUMP -d ${TARGET}.elf > ${TARGET}.lst
	     $OBJDUMP -d ${TARGET}.elf > ${TARGET}.lst
	echo $OBJCOPY -R .noinit -O ihex ${TARGET}.elf ${TARGET}.hex
	     $OBJCOPY -R .noinit -O ihex ${TARGET}.elf ${TARGET}.hex
	echo $OBJCOPY -R .noinit -O binary ${TARGET}.elf ${TARGET}.bin
	     $OBJCOPY -R .noinit -O binary ${TARGET}.elf ${TARGET}.bin
	echo "Success"
}

clean() {
	rm -f ${TARGET}.elf
	rm -f ${TARGET}.lst
	rm -f ${TARGET}.hex
	rm -f ${TARGET}.map
}

flash() {
	find_NRFJPROG
	echo "Flashing ${TARGET}..."
	$NRFJPROG --program ${TARGET}.hex -f NRF53 --coprocessor CP_APPLICATION --sectorerase
	$NRFJPROG --reset
	echo "Success"
}

find_CC() {
	find_inner() {
		set +e
		$1$2 --version 2> /dev/null > /dev/null
		result=$?
		set -e
		if [ $result == 0 ]; then
			echo "Found compiler at: $1$2"
			echo "Version: $($1$2 --version | head -n 1)"
			CC=$1$2
			OBJCOPY=${1}objcopy
			OBJDUMP=${1}objdump
			return 1
		else
			return 0
		fi
	}
	trap 'return 0' ERR
	find_inner $GCC_ARM_BIN_DIR/arm-none-eabi- gcc
	find_inner $GCC_ARM_BIN_DIR/bin/arm-none-eabi- gcc
	find_inner arm-none-eabi- gcc
	find_inner $ZEPHYR_SDK_INSTALL_DIR/arm-zephyr-eabi/bin/arm-zephyr-eabi- gcc
	echo "ERROR: Cannot find compiler (arm-none-eabi-gcc)!"
	echo "ERROR: Add its bin directory to your PATH or GCC_ARM_BIN_DIR variable."
	exit 1
}

find_CMSIS() {
	find_inner() {
		if [ -e $1/core_cm33.h ]; then
			echo "Found CMSIS at: $1"
			CMSIS=$1
			return 1
		elif [ -e ${1/CMSIS/cmsis}/core_cm33.h ]; then
			echo "Found CMSIS at: ${1/CMSIS/cmsis}"
			CMSIS=${1/CMSIS/cmsis}
			return 1
		else
			return 0
		fi
	}
	trap 'return 0' ERR
	find_inner $CMSIS_DIR
	find_inner $CMSIS_DIR/Include
	find_inner $CMSIS_DIR/Core/Include
	find_inner $CMSIS_DIR/CMSIS/Core/Include
	find_inner ./CMSIS
	find_inner ./CMSIS/Include
	find_inner ./CMSIS/Core/Include
	find_inner ./CMSIS/CMSIS/Core/Include
	WITH_VER=(./CMSIS/*)
	find_inner $WITH_VER/CMSIS/Core/Include
	WITH_VER=(./CMSIS_*)
	find_inner $WITH_VER/CMSIS/Core/Include
	find_inner $ZEPHYR_BASE/ext/hal/CMSIS/Core/Include
	echo "ERROR: Cannot find CMSIS directory!"
	echo "ERROR: Place it in $(realpath .)/CMSIS or provide path in CMSIS_DIR variable."
	exit 1
}

find_NRFX() {
	find_inner() {
		if [ -e $1 ]; then
			echo "Found nrfx at: $1"
			NRFX=$1
			return 1
		else
			return 0
		fi
	}
	trap 'return 0' ERR
	find_inner $NRFX_DIR _dummy__parameter_
	find_inner ./nrfx
	echo "ERROR: Cannot find nrfx directory!"
	echo "ERROR: Place it in $(realpath .)/nrfx or provide path in NRFX_DIR variable."
	exit 1
}

find_NRFJPROG() {
	find_inner() {
		set +e
		$1 --version 2> /dev/null > /dev/null
		result=$?
		set -e
		if [ $result == 0 ]; then
			echo "Found nrfjprog command at: $1"
			$1 --version
			NRFJPROG=$1
			return 1
		else
			return 0
		fi
	}
	trap 'return 0' ERR
	find_inner $NRFJPROG_DIR/nrfjprog
	find_inner $NRFJPROG_DIR/nrfjprog/nrfjprog
	find_inner nrfjprog
	find_inner /opt/nrfjprog/nrfjprog
	echo "ERROR: Cannot find nrfjprog!"
	echo "ERROR: Add it to your PATH or set NRFJPROG_DIR variable."
	exit 1
}

set -e

if [ "$1" == "flash" ]; then
	flash "$@"
elif [ "$1" == "clean" ]; then
	clean "$@"
else
	build "$@"
fi
