#
#	Makefile for KFM modules.
#  

KVER = /lib/modules/$(shell uname -r)/build
CURDIR = $(shell pwd)

kfm-objs := kfm_core.o \
			kfm_cache.o \
			kfm_mem.o \
			kfm_ctl.o

# kfm core
obj-m += kfm.o
#obj-$(CONFIG_KFM) += kfm.o

all:
	$(MAKE) -C $(KVER) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KVER) M=$(CURDIR) clean