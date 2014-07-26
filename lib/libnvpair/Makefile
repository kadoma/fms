# Makefile
#
# Created on: Dec 22, 2010
#     Author: Inspur OS Team
#
#

include ../Makefile.defs

LDFLAGS += -shared -fpic
LIBNAME = libnvpair.so
CFLAGS += -I../../include

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $<

all: $(OBJS)
	$(CC) -o $(LIBNAME) $(LDFLAGS) $(OBJS)
#	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME).$(LIBVER)
#	$(LN) $(ROOTLIB)/$(LIBNAME).$(LIBVER) $(ROOTLIB)/$(LIBNAME)

clean:
	$(RM) $(OBJS) $(LIBNAME)

install:
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME).$(LIBVER)
#	$(LN) $(ROOTLIB)/$(LIBNAME).$(LIBVER) $(ROOTLIB)/$(LIBNAME)
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME)

uninstall:
	$(RM) $(ROOTLIB)/$(LIBNAME)
	$(RM) $(ROOTLIB)/$(LIBNAME).$(LIBVER)
