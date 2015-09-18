### create so lib file is ok

SHELL=/bin/sh
CC=gcc

TARGET_PATH=/usr/lib/fms/

TARGET=libfmd.so

LDFLAGS= -ldl -lpthread -shared -fpic -g

CFLAGS= -g -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
	-I ../../include\
	-I ../include\

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)
	$ mkdir -p $(TARGET_PATH) && cp $(TARGET) $(TARGET_PATH)

install:
	$ mkdir -p $(TARGET_PATH) && cp $(TARGET) $(TARGET_PATH)

clean:
	rm -rf *.o *.so
