### Makefile for hotblug tool

SHELL=/bin/sh
CC=gcc
RM=rm -f

TARGET= fmshotplug

CFLAGS= -g -Wall -O0 \
	-I../../include \
	-Wno-unused-function \
    	-Wno-unused-variable \
	-L/usr/lib/fms \
	-L/usr/lib

LDFLAGS= -ldl -lpthread -lrt -ladm -ldict -lfmd

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRCS)

clean:
	$(RM) $(TARGET)

#install:
#	$(CP) $(TARGET) $(FMDDIR)
#	$(CP) $(TARGET) $(CMDDIR)

#uninstall:
#	$(RM) $(FMDDIR)/$(TARGET)
#	$(RM) $(CMDDIR)/$(TARGET)
