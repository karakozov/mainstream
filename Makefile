#
#change this makefile for your target...
#

DIRS := ../../BARDY/gipcy/include

ifeq "$(findstring c6x-uclinux-gcc, $(CC))" "c6x-uclinux-gcc"
    OS := __LINUX__
    CSTOOL_PREFIX ?= c6x-uclinux-
    IPC := __IPC_LINUX__
    TARGET := -march=c674x
    NCURSES  = ../../tools/ncurses-5.9-c6x/build/lib/libncurses.a
    DIRS += ../../tools/ncurses-5.9-c6x/build/include
    DIRS += ../../tools/ncurses-5.9-c6x/build/include/ncurses
    LIBPATH  = ./libs_c6x
    PLATFORM := _c6x_
else
    NCURSES  = -lncurses
    LIBPATH  = ./libs_x86
    PLATFORM := _x86_
endif

PHONY = clean
TARGET_NAME = mainstream

all: $(TARGET_NAME)

ROOT_DIR = $(shell pwd)

INC := $(addprefix -I, $(DIRS))

CC = $(CSTOOL_PREFIX)g++
LD = $(CSTOOL_PREFIX)g++

CFLAGS += $(TARGET) -D$(PLATFORM) -D__LINUX__ -g -Wall $(INC)
#LFLAGS = -Wl,-rpath $(LIBPATH) -L"$(LIBPATH)"
LFLAGS += $(TARGET)

$(TARGET_NAME): $(patsubst %.cpp,%.o, $(wildcard *.cpp))
	$(LD) -o $(TARGET_NAME) $^ $(LIBPATH)/libgipcy.a $(NCURSES) $(LFLAGS)
ifeq "$(findstring c6x, $(CC))" "c6x"
	cp $(TARGET_NAME) $(INSTALL_PREFIX)/home/$(TARGETFS_USER)/bardy/bin
endif
#	rm -f *.o *~ core

%.o: %.cpp
	$(CC) $(CFLAGS) -c -MD $<
	
include $(wildcard *.d)


clean:
	rm -f *.o *~ core
	rm -f *.d *~ core
	rm -f $(TARGET_NAME)
	
distclean:
	rm -f *.o *~ core
	rm -f *.d *~ core
	rm -f $(TARGET_NAME)
