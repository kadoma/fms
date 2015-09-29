# Makefile
#
# Created on: Oct 13, 2010
#     Author: liubin, wanghuan
#

CC = /usr/bin/gcc
RM = /bin/rm -f
CP = /bin/scp 1>/dev/null 2>&1 -f

ROOTLIB = /usr/lib/fms
SRCINC =../../include
LIBNAME = libfmd_msg.so

CFLAGS = -g -Wall -I$(SRCINC)  \
		-Wno-unused-function \
    	-Wno-unused-variable
LDFLAGS = -L$(ROOTLIB) -ldl -lpthread -shared -fpic 

#lib version
LIBVER = 2.0.0

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(LIBNAME) $(LDFLAGS) $(CFLAGS) $(SRCS)

clean:
	$(RM) $(OBJS) $(LIBNAME)

install:
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME).$(LIBVER)
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME)

uninstall:
	$(RM) $(ROOTLIB)/$(LIBNAME)
	$(RM) $(ROOTLIB)/$(LIBNAME).$(LIBVER)

