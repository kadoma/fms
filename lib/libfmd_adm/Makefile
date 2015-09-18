# Makefile
#
# Created on: Oct 13, 2010
#     Author: liubin
#
#

CC = /usr/bin/gcc
RM = /bin/rm -f
CP = /bin/cp 1>/dev/null 2>&1 -f

ROOTLIB = $(DESTDIR)/usr/lib/fms

SRCINC = ../../include 
CFLAGS = -c -Wall -g -DFMD_DEBUG -I$(SRCINC)  \
		-Wno-unused-function \
    	-Wno-unused-variable
LDFLAGS = -L$(ROOTLIB) -ldl -lpthread -shared -fpic -g 

#lib version
LIBVER = 2.0.0
LIBNAME = libfmd_adm.so

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(LIBNAME) $(CFLAGS) $(LDFLAGS) $(SRCS)

clean:
	$(RM) $(OBJS) $(LIBNAME)

install:
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME).$(LIBVER)
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME)

uninstall:
	$(RM) $(ROOTLIB)/$(LIBNAME)
	$(RM) $(ROOTLIB)/$(LIBNAME).$(LIBVER)

