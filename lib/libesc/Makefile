### create so lib file is ok


SHELL=/bin/sh
CC=gcc

BASEPATH=.
OUTPUT=$(BASEPATH)

TARGET=libesc.so

LDFLAGS= -ldl -lpthread -ll -shared -fpic

CFLAGS= -g -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
	-I ../../include\
	-I ../include\


SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)
	cp libesc.so /usr/lib/fms/

install:
	cp libesc.so /usr/lib/fms/

clean:
	rm -rf *.o *.so

