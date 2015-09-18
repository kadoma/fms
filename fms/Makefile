SHELL=/bin/sh
CC=gcc

BASEPATH=.
OUTPUT=$(BASEPATH)

TARGET=fmd

LDFLAGS= -lpthread -ldl -lfmd -levt -lcase -lesc

CFLAGS= -g -O0 -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
	-I ../include\
	-I ../evt_modules/include\
	-L /usr/lib/fms

all:$(TARGET)

$(TARGET): ./fmd_common.c ./fmd_queue.c ./main.c
	$(CC) -o $(OUTPUT)/$@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(TARGET) *.o
