###Makefile for plugins is ok

SHELL=/bin/sh
CC=gcc

BASEPATH=.
OUTPUT=$(BASEPATH)

TARGET=adm_src.so

LDFLAGS= -lpthread -ldl -shared -fpic -lrt -g

CFLAGS= -g -O0 -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
	-I ../../../include\
	-I ../../include\
	-L /usr/lib/fms\

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)
	scp adm_src.so /usr/lib/fms/plugins

install:
	scp adm_src.so /usr/lib/fms/plugins

clean:
	rm -rf *.o *.so


