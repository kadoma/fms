###Makefile for plugins is ok

SHELL=/bin/sh
CC=gcc

BASEPATH=.
OUTPUT=$(BASEPATH)

TARGET=disk_src.so

LDFLAGS= -lpthread -ldl -levt -lfmd -ll -shared -fpic -g

CFLAGS= -g -O0 -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
	-I ../../include\
	-I ../../../include\
	-L /usr/lib/fms

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)
	cp disk_src.so /usr/lib/fms/plugins

install:
	cp disk_src.so /usr/lib/fms/plugins

clean:
	rm -rf *.o *.so


