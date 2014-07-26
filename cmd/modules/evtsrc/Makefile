# Makefile
#
# Created on: Jan 19, 2010
#     Author: Inspur OS Team
#
#

include ../../Makefile.defs

LDFLAGS += -lfmd

SUBDIRS = \
	cpumem \
	inject \
	disk \
	proc \
	ipmi \
	mpio \
	sysmon \
	raid \
	adm \
	apache

all install clean uninstall: $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

FRC:
