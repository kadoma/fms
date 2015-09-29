# makefile.defs
#
# Created on: Jan 19, 2010
#     Author: nilei
#


CC = /usr/bin/gcc

CFLAGS = -c -g -DFMD_DEBUG -Wall -I../../include \
		-Wno-unused-function \
    	-Wno-unused-variable
LDFLAGS = -L$(ROOTLIB) -ldl -lpthread\
	-L../../lib/libfmd_adm -L../../lib/libfmd_msg -lrt -lfmd_adm -lfmd_msg

RM = /bin/rm -f
CP = /bin/scp 1>/dev/null 2>&1 -f
LN = /bin/ln -s -f

LDCONFILE = fmd-i386.conf
LDCONFDIR = /etc/ld.so.conf.d/
LDCONFIG = /sbin/ldconfig

INCDIR = $(DESTDIR)/usr/include/fm
FMDDIR =/usr/lib/fms/fmd

LIBVER = 2.0.0

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)
PROG = fmsadm

%.o: %.c
	$(CC) $(CFLAGS) $<

all: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(PROG)

clean:
	$(RM) $(OBJS) $(PROG)

install:
	$(CP) $(PROG) $(FMDDIR)
	$(CP) $(PROG) $(CMDDIR)

uninstall:
	$(RM) $(FMDDIR)/$(PROG)
	$(RM) $(CMDDIR)/$(PROG)
