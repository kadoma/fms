#
#
#    This Makefile is for compiling cpumem event source library. 
#
#                    

CC      = gcc
CFLAGS  = -g -Wall -Wno-unused-function
LDFLAGS = -lpthread -ldl -levt -lfmd -shared -fpic -L /usr/lib/fms
COMPILE	= $(CC) $(CFLAGS) -c

INCLUDES	= -I ../../../include/ -I ../../include
EVTLIB_DIR	= ../../evtlib
FMS_LIBFMD_DIR = ../../../lib/libfmd
FMS_SO_DIR = /usr/lib/fms/plugins

PROGRAM = libcpumem_src.so

RM      = /bin/rm -f
CP		= /usr/bin/scp

LIBOBJS = cpumem_evtsrc.o	\
		bitfield.o		\
		dmi.o			\
		haswell.o		\
		intel.o			\
		p4.o			\
		$(EVTLIB_DIR)/cpumem.o \
		$(FMS_LIBFMD_DIR)/logging.o


all: $(PROGRAM)
	cp ./libcpumem_src.so /usr/lib/fms/plugins

$(PROGRAM): $(LIBOBJS)
	echo Creating $@...
	$(RM) $@
	$(CC) $(LDFLAGS) -o $@ $(LIBOBJS) 
	$(CP) $(PROGRAM) $(FMS_SO_DIR)
	
%.o:%.c
	$(COMPILE) $(INCLUDES) -fPIC -o $@ $^

clean:
	$(RM) $(LIBOBJS) $(PROGRAM)