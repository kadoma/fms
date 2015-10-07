### Makefile for libamd.so

SHELL=/bin/sh
CC=gcc
RM=rm -f
CP=cp 1>/dev/null 2>&1 -f

LIBDIR=/usr/lib/fms

TARGET=libadm.so

CFLAGS= -Wall -g -O0 \
	-Wno-unused-function \
    	-Wno-unused-variable \
	-I../../include
	
LDFLAGS= -ldl -lpthread -shared -fpic

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)
	$(CP) $(TARGET) $(LIBDIR)/$(TARGET)

clean:
	$(RM) $(OBJS) $(TARGET)

install:
	$(CP) $(TARGET) $(LIBDIR)/$(TARGET)

uninstall:
	$(RM) $(LIBDIR)/$(TARGET)

