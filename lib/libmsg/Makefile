### Makefile for libmsg.so 

SHELL=/bin/sh
CC=gcc
RM=rm -f
CP=cp 1>/dev/null 2>&1 -f

LIBDIR=/usr/lib/fms

TARGET=libmsg.so

CFLAGS = -g -Wall -O0 \
	-Wno-unused-function \
    	-Wno-unused-variable \
	-I../../include

LDFLAGS = -ldl -lpthread -shared -fpic 


SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(LDFLAGS) $(CFLAGS) $(SRCS)
	$(CP) $(TARGET) $(LIBDIR)/$(TARGET)

clean:
	$(RM) $(TARGET)

install:
	$(CP) $(TARGET) $(LIBDIR)/$(TARGET)

uninstall:
	$(RM) $(LIBDIR)/$(TARGET)
