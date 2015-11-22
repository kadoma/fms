###Makefile for libtopo.so

SHELL=/bin/sh
CC=gcc
CP=cp 1>/dev/null 2>&1 -f

TARGET=libtopo.so

CFLAGS = -g -Wall -O0 \
	-Wno-unused-function \
	-Wno-unused-variable \
	-I../../include \

LDFLAGS = -shared -fpic -ldl -lpthread
LIBDIR = /usr/lib/fms
SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(LDFLAGS) $(CFLAGS) $(SRCS)
	$(CP) $(TARGET) $(LIBDIR)/$(TARGET)

clean:
	$(RM) $(OBJS) $(TARGET)

install:
	$(CP) $(TARGET) $(LIBDIR)/$(TARGET)

uninstall:
	$(RM) $(LIBDIR)/$(TARGET)

