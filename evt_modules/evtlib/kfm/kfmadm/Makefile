#
#   Makefile for kfmadm userspace utility (kfmadm)
#

CC      = gcc
CFLAGS  = -g -Wall -DKFM_TEST
COMPILE	= $(CC) $(CFLAGS) -c

PROGRAM = kfmadm

RM      = /bin/rm -f

LIBOBJS = kfmadm.o			\
		  kfm_nl.o			\
		  kfm_nl_policy.o	\
		  libkfm_cpumem.o	

all: $(PROGRAM)

$(PROGRAM): $(LIBOBJS)
	$(CC) -o $@ $(LIBOBJS) -lnl

%.o:%.c
	$(COMPILE) -o $@ $^

clean:
	$(RM) $(LIBOBJS) $(PROGRAM)