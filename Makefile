#
#change this makefile for your target...
#

ifeq "$(findstring QNX, $(shell uname -a))" "QNX"
	OS := DZY_QNX
else
	OS := DZY_LINUX
endif

PHONY = clean
TARGET_NAME = mainstream

all: $(TARGET_NAME)

ROOT_DIR = $(shell pwd)
LIBPATH  = ../../bin

DIRS := ../../gipcy/include
INC := $(addprefix -I, $(DIRS))

CC = $(CSTOOL_PREFIX)g++
LD = $(CSTOOL_PREFIX)g++

ARCH := c674x
TARGET := c66x

CFLAGS =  -march=$(ARCH) -D$(OS) -D__LINUX__ -g -Wall $(INC)
#LFLAGS = -Wl,-rpath $(LIBPATH) -L"$(LIBPATH)"
LFLAGS = -march=$(ARCH)

$(TARGET_NAME): $(patsubst %.cpp,%.o, $(wildcard *.cpp))
#	$(LD) -o $(TARGET_NAME) $^ $(LFLAGS)
	$(LD) -o $(TARGET_NAME) $^ $(LIBPATH)/libgipcy.a $(LFLAGS)
	cp $(TARGET_NAME) ../../bin
	cp $(TARGET_NAME) $(INSTALL_PREFIX)/home/$(TARGETFS_USER)/bardy/bin
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
