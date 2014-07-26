# Makefile
#
# Created on: Jan 19, 2010
#     Author: Inspur OS Team
#
#

include ../Makefile.defs

LDFLAGS += -shared -fpic -g
LIBNAME = libfmd_eversholt.so

SRCS = $(wildcard *.c)

all:
	$(CC) -o $(LIBNAME) -I../../include $(LDFLAGS) $(SRCS)
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
