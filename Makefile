#
#change this makefile for your target...
#

ifeq "$(findstring c6x-uclinux-gcc, $(CC))" "c6x-uclinux-gcc"
    OS := __LINUX__
    CSTOOL_PREFIX ?= c6x-uclinux-
    IPC := __IPC_LINUX__
    TARGET := -march=c674x
endif

PHONY = clean
TARGET_NAME = mainstream

all: $(TARGET_NAME)

ROOT_DIR = $(shell pwd)
LIBPATH  = ../../BARDY/bin

DIRS := ../../BARDY/gipcy/include
INC := $(addprefix -I, $(DIRS))

CC = $(CSTOOL_PREFIX)g++
LD = $(CSTOOL_PREFIX)g++

CFLAGS += $(TARGET) -D__LINUX__ -g -Wall $(INC)
#LFLAGS = -Wl,-rpath $(LIBPATH) -L"$(LIBPATH)"
LFLAGS += $(TARGET)

$(TARGET_NAME): $(patsubst %.cpp,%.o, $(wildcard *.cpp))
#	$(LD) -o $(TARGET_NAME) $^ $(LFLAGS)
	$(LD) -o $(TARGET_NAME) $^ $(LIBPATH)/libgipcy.a $(LFLAGS)
#	cp $(TARGET_NAME) ../../bin
#	cp $(TARGET_NAME) $(INSTALL_PREFIX)/home/$(TARGETFS_USER)/bardy/bin
	rm -f *.o *~ core

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
