
TARGET = test

INCLUDE = \
	-I. \
	-I../app \
	-Igtest/googletest/include \
	-Igtest/googlemock/include \
	-Igtest/googletest \
	-Igtest/googlemock

# C and C++ flags
CPPFLAGS   = \
	-O0 -g3 -fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-strict-aliasing -fshort-enums

# C flags
CFLAGS   = 

# C++ flags
CXXFLAGS   = 

# Assembler flags
ASMFLAGS =

LIBS = \
	-lm
OBJ = main.o \
	test_ref.o \
	gtest/googletest/src/gtest-all.o \
	gtest/googlemock/src/gmock-all.o

all: $(TARGET).exe

clean:
	rm -f $(OBJ) $(TARGET).exe

.cc.o: Makefile
	c++ -c $(CPUFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDE) -o $@ $<

.cpp.o: Makefile
	c++ -c $(CPUFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDE) -o $@ $<

.c.o: Makefile
	gcc -c $(CPUFLAGS) $(CPPFLAGS) $(CFLAGS) $(INCLUDE) -o $@ $<

.S.o: Makefile
	gcc -c $(CPUFLAGS) $(CPPFLAGS) $(ASMFLAGS) $(INCLUDE) -o $@ $<

$(TARGET).exe: $(OBJ) Makefile
	c++ $(CPUFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LIBS) -o $(TARGET).exe $(OBJ)
	