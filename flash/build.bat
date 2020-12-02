@echo off
::
:: Copyright (c) 2020 Nordic Semiconductor
::
:: SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
::

:: Uncomment and fill following lines or provide following variables before
:: running this script.
:::: Directory containing C compiler "arm-none-eabi-gcc".
set GCC_ARM_BIN_DIR=C:\work\bin\gcc\bin
:::: Directory containing nrfx
::set NRFX_DIR=
:::: Directory containing CMSIS
::set CMSIS_DIR=
:::: Directory containing nrfjprog
::set NRFJPROG_DIR=


set TARGET=recovery-bootloader


goto main


:build

    call :find_CC     || goto error
    call :find_NRFX   || goto error
    call :find_CMSIS  || goto error

    set SOURCE_FILES=.\src\main.c .\src\startup.c .\src\conf.c .\src\utils.c
    set INCLUDE=-I%NRFX%\mdk -I%CMSIS%
    set LIBS=-L.\src
    set CFLAGS=-Os -g3 -fdata-sections ^
        -I./src ^
        -ffunction-sections -Wl,--gc-sections ^
        -Wno-unused-function -Wno-main ^
		-Wall -fno-strict-aliasing -fshort-enums ^
		-mthumb -mabi=aapcs ^
		--specs=nosys.specs -nostdlib ^
		-mcpu=cortex-m0 ^
		-mfloat-abi=soft ^
		-Tlinker.ld ^
        -Wl,--defsym=_LS_RAM_APP_SIZE=0x3DC0 ^
        -Wl,--defsym=_LS_RAM_SIZE=0x4000 ^
		-DNRF51 ^
		-DNRF51822_XXAA ^
        -DWITH_SOFT_DEVICE ^
        -DDUMMY_LTO

    echo. > %TARGET%.c
    call :add_source .\src\main.c 
    call :add_source .\src\startup.c 
    call :add_source .\src\conf.c 
    call :add_source .\src\utils.c
    call :add_source .\src\crypto.c
    call :add_source .\src\radio.c
    call :add_source .\src\timer.c
    call :add_source .\src\rand.c
    call :add_source .\src\conn.c
    call :add_source .\src\req.c

    echo Compiling %TARGET%...
    %CC% %CFLAGS% %INCLUDE% %LIBS% %TARGET%.c -Wl,-Map=%TARGET%.map -o %TARGET%.elf || goto error
    %OBJDUMP% -d %TARGET%.elf > %TARGET%.lst                                            || goto error
    %OBJCOPY% -O ihex -j .text %TARGET%.elf %TARGET%.hex                                         || goto error
    echo Success
    %PAUSE%

goto :EOF

:add_source
    set FULL_PATH="%~dpnx1"
    echo. >> %TARGET%.c
    echo #line 1 %FULL_PATH:\=/% >> %TARGET%.c
    type %1 >> %TARGET%.c
    echo. >> %TARGET%.c
    goto :EOF

:clean
    echo Cleaning %TARGET%...
    del /f /q %TARGET%.elf > nul 2> nul
    del /f /q %TARGET%.lst > nul 2> nul
    del /f /q %TARGET%.hex > nul 2> nul
    del /f /q %TARGET%.map > nul 2> nul
    echo Done
    %PAUSE%
goto :EOF


:flash
    call :find_NRFJPROG                                                                   || goto error
    echo Flashing %TARGET%...
    %NRFJPROG% --program %TARGET%.hex -f NRF53 --coprocessor CP_APPLICATION --sectorerase || goto error
    %NRFJPROG% --reset                                                                    || goto error
    echo Success
    %PAUSE%
goto :EOF


:find_CC_inner
    "%~1%2" --version > nul 2> nul
    if ERRORLEVEL 1 goto :EOF
    "%~1%2" --version
    echo Found compiler at: %~1%2
    set CC="%~1%2"
    set OBJCOPY="%~1%objcopy"
    set OBJDUMP="%~1%objdump"
    set SIZE="%~1%size"
goto :EOF

:find_CC
    call :find_CC_inner %GCC_ARM_BIN_DIR%\arm-none-eabi- gcc                              && goto :EOF
    call :find_CC_inner arm-none-eabi- gcc                                                && goto :EOF
    call :find_CC_inner %ZEPHYR_SDK_INSTALL_DIR%\arm-zephyr-eabi\bin\arm-zephyr-eabi- gcc && goto :EOF
    for /F "tokens=* delims=" %%i in ('dir /on /b "%ProgramFiles%\GNU Tools Arm Embedded\*"') do set WITH_VER=%%i
    for /F "tokens=* delims=" %%i in ('dir /on /b "%ProgramFiles%\GNU Tools Arm Embedded\1*"') do set WITH_VER=%%i
    for /F "tokens=* delims=" %%i in ('dir /on /b "%ProgramFiles%\GNU Tools Arm Embedded\2*"') do set WITH_VER=%%i
    call :find_CC_inner "%ProgramFiles%\GNU Tools Arm Embedded\%WITH_VER%\bin\arm-none-eabi-" gcc && goto :EOF
    echo ERROR: Cannot find compiler (arm-none-eabi-gcc)!
    echo ERROR: Add its bin directory to your PATH or GCC_ARM_BIN_DIR variable.
    %PAUSE%
exit /b 1


:find_CMSIS_inner
    if not exist "%1\core_cm33.h" exit /b 1
    echo Found CMSIS at: %1
    set CMSIS=%1
exit /b 0

:find_CMSIS
    call :find_CMSIS_inner %CMSIS_DIR%                                && goto :EOF
    call :find_CMSIS_inner %CMSIS_DIR%\Include                        && goto :EOF
    call :find_CMSIS_inner %CMSIS_DIR%\Core\Include                   && goto :EOF
    call :find_CMSIS_inner %CMSIS_DIR%\CMSIS\Core\Include             && goto :EOF
    call :find_CMSIS_inner .\CMSIS                                    && goto :EOF
    call :find_CMSIS_inner .\CMSIS\Include                            && goto :EOF
    call :find_CMSIS_inner .\CMSIS\Core\Include                       && goto :EOF
    call :find_CMSIS_inner .\CMSIS\CMSIS\Core\Include                 && goto :EOF
    for /D %%f in (_begin_of_list_ CMSIS_*) do set WITH_VER=%%f
    call :find_CMSIS_inner %WITH_VER%\CMSIS\Core\Include              && goto :EOF
    for /D %%f in (_begin_of_list_ CMSIS\CMSIS_*) do set WITH_VER=%%f
    call :find_CMSIS_inner %WITH_VER%\CMSIS\Core\Include              && goto :EOF
    call :find_CMSIS_inner %ZEPHYR_BASE%\ext\hal\CMSIS\Core\Include   && goto :EOF
    echo ERROR: Cannot find CMSIS directory!
    echo ERROR: Place it in .\CMSIS or provide path in CMSIS_DIR variable.
    %PAUSE%
exit /b 1


:find_NRFX_inner:
    if not exist "%1" exit /b 1
    echo Found nrfx at: %1
    set NRFX=%1
exit /b 0

:find_NRFX
    call :find_NRFX_inner %NRFX_DIR% _dummy__parameter_ && goto :EOF
    call :find_NRFX_inner .\nrfx                        && goto :EOF
    echo ERROR: Cannot find nrfx directory!
    echo ERROR: Place it in .\nrfx or provide path in NRFX_DIR variable.
    %PAUSE%
exit /b 1


:find_NRFJPROG_inner
    %1 --version > nul 2> nul
    if ERRORLEVEL 1 goto :EOF
    echo Found nrfjprog command at: %1
    %1 --version
    set NRFJPROG=%1
goto :EOF

:find_NRFJPROG
    call :find_NRFJPROG_inner %NRFJPROG_DIR%\nrfjprog          && goto :EOF
    call :find_NRFJPROG_inner %NRFJPROG_DIR%\nrfjprog\nrfjprog && goto :EOF
    call :find_NRFJPROG_inner nrfjprog                         && goto :EOF
    echo ERROR: Cannot find compiler (arm-none-eabi-gcc)!
    echo ERROR: Add its bin directory to your PATH or GCC_ARM_BIN_DIR variable.
    %PAUSE%
exit /b 1


:main
    cd /d %~dp0
    set PAUSE=echo Pass -n parameter to skip script pausing. ^&^& pause
    if "%1"=="-n" goto no_interactive
:main_ret
    if "%1"=="flash" goto flash
    if "%1"=="clean" goto clean
goto build

:no_interactive
    set PAUSE=rem
    shift
goto main_ret


:error
    set code=%ERRORLEVEL%
    echo Failed with error code %code%
    %PAUSE%
exit /b %code%
