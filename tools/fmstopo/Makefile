# Makefile.prog
#
# Created on: Jan 19, 2015
#     Author: wanglei
#
#
CC = /usr/bin/gcc
RM = /bin/rm -f
CP = /bin/cp 1>/dev/null 2>&1 -f

SRCINC = ../../include
SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)
CFLAGS = -c -Wall -g -I$(SRCINC) -DFMD_DEBUG \
		-Wno-unused-function \
    	-Wno-unused-variable
LDFLAGS = -L /usr/lib -lpthread -ldl
PROG = fmstopo

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< 

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
