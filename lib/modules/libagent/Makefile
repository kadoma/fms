# Makefile
#
# Created on: Jan 19, 2010
#     Author: Inspur OS Team
#
#

include ../../Makefile.defs

LDFLAGS += -L$(ROOTLIB) -L../libmod -shared -lfmd_module -fpic
LIBNAME = libfmd_agent.so
CFLAGS += -I../../../include

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
	$(CP) $(LIBNAME) $(ROOTLIB)/$(LIBNAME)
#	$(LN) $(ROOTLIB)/$(LIBNAME).$(LIBVER) $(ROOTLIB)/$(LIBNAME)

uninstall:
	$(RM) $(ROOTLIB)/$(LIBNAME)
	$(RM) $(ROOTLIB)/$(LIBNAME).$(LIBVER)
