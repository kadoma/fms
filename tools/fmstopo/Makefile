### Makefile for topo tool 

SHELL=/bin/sh
CC = gcc
RM = rm -f
CP = cp 1>/dev/null 2>&1 -f

TARGET=fmstopo

SRCS = $(wildcard *.c)

CFLAGS= -Wall -g -O0 \
	-Wno-unused-function \
    	-Wno-unused-variable\
	-I../../include \
	-L /usr/lib/fms/

LDFLAGS = -lpthread -ldl 


all: 
	$(CC) -o$(TARGET) $(LDFLAGS) $(CFLAGS) $(SRCS)

clean:
	$(RM) $(TARGET)

#install:
#	$(CP) $(PROG) $(FMDDIR)
#	$(CP) $(PROG) $(CMDDIR)

#uninstall:
#	$(RM) $(FMDDIR)/$(PROG)
#	$(RM) $(CMDDIR)/$(PROG)
