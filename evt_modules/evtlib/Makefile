### create so lib file is ok


SHELL=/bin/sh
CC=gcc

BASEPATH=.
OUTPUT=$(BASEPATH)

TARGET=libevt.so

LDFLAGS= -ldl -lpthread -lfmd -lcase -lesc -shared -fpic -g

CFLAGS= -g -O0 -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
	-I ../../include\
	-I ../include\
	-L /usr/lib/fms/

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)
	cp libevt.so /usr/lib/fms/

install:
	cp libevt.so /usr/lib/fms/

clean:
	rm -rf *.o *.so
