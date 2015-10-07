### create so lib file is ok


SHELL=/bin/sh
CC=gcc

BASEPATH=.
OUTPUT=$(BASEPATH)

TARGET=libesc.so

LDFLAGS= -ldl -lpthread -shared -fpic

CFLAGS= -g -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
	-I ../../include\
	-I ../include\


SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)
	scp libesc.so /usr/lib/fms/

install:
	scp libesc.so /usr/lib/fms/

clean:
	rm -rf *.o *.so

