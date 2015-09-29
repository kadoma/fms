#############################################################
# Copyright (C) inspur Inc. <http://www.inspur.com>
#  FileName:    Makefile
#  Author:      Inspur OS Team 
#                wang.leibj@inspur.com
#  Date:        2015-08-28
#  Description: the topo make file
# 
#############################################################

CC = /usr/bin/gcc
CP = /bin/scp 1>/dev/null 2>&1 -f
LN = /bin/ln -s -f

SRCINC =../../include
CFLAGS = -Wall -I$(SRCINC) \
		-Wno-unused-function \
    	-Wno-unused-variable
LDFLAGS = -shared -fpic -g 

LIBNAME = libfmd_topo.so
LIBVER = 2.0.0
ROOTLIB = /usr/lib/fms
SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)

all:
	$(CC) -o $(LIBNAME) $(LDFLAGS) $(CFLAGS) $(SRCS)

clean:
	$(RM) $(OBJS) $(LIBNAME)

install:
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME).$(LIBVER)
	$(LN) $(ROOTLIB)/$(LIBNAME).$(LIBVER) $(ROOTLIB)/$(LIBNAME)
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME)

uninstall:
	$(RM) $(ROOTLIB)/$(LIBNAME)
	$(RM) $(ROOTLIB)/$(LIBNAME).$(LIBVER)
