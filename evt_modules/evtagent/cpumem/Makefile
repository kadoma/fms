#
#	This Makefile is for compiling cpumem event agent library. 
#	          

CC      = gcc
CFLAGS  = -g -Wall -Wno-unused-function -Wno-unused-function-variable
LDFLAGS = -lpthread -ldl -levt -lfmd -lcpumem_src -shared -fpic -lnl -L /usr/lib/fms -L /usr/lib/fms/plugins
COMPILE	= $(CC) $(CFLAGS) -c

KFM_LIB_DIR = ../../evtlib/kfm/kfmadm
INCLUDES	= -I ../../../include/ -I ../../include -I $(KFM_LIB_DIR)
EVTLIB_DIR	= ../../evtlib
FMS_LIBFMD_DIR = ../../../lib/libfmd
FMS_SO_DIR = /usr/lib/fms/plugins

PROGRAM = cpumem_agent.so

RM      = /bin/rm -f
CP		= /usr/bin/scp

LIBOBJS = cpumem_agent.o	\
		  evt_process.o		\
		  memdev.o			\
		  sysfs.o			\
		  tsc.o				\
		  cache.o			\
		  $(EVTLIB_DIR)/cpumem.o \
		  $(FMS_LIBFMD_DIR)/logging.o \
		  $(KFM_LIB_DIR)/kfm_nl.o \
		  $(KFM_LIB_DIR)/kfm_nl_policy.o \
		  $(KFM_LIB_DIR)/libkfm_cpumem.o 

all: $(PROGRAM)
	cp cpumem_agent.so /usr/lib/fms/plugins/

$(PROGRAM): $(LIBOBJS)
	echo Creating $@...
	$(RM) $@
	$(CC) $(LDFLAGS) -o $@ $(LIBOBJS)
	$(CP) $(PROGRAM) $(FMS_SO_DIR)
	
%.o:%.c
	$(COMPILE) $(INCLUDES) -fPIC -o $@ $^

clean:
	$(RM) $(LIBOBJS) $(PROGRAM)