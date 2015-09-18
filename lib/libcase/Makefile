### create so lib file is ok


SHELL=/bin/sh
CC=gcc

BASEPATH=.
OUTPUT=$(BASEPATH)

TARGET=libcase.so

LDFLAGS= -ldl -lpthread -shared -fpic -g

CFLAGS= -g -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
	-I ../../include\
	-I ../include\

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)
	cp libcase.so /usr/lib/fms/

install:
	cp libcase.so /usr/lib/fms/

clean:
	rm -rf *.o *.so

