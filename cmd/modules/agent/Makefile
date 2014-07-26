# Makefile
#
# Created on: Jan 10, 2011
#     Author: Inspur OS Team
#
#

include ../../Makefile.defs

LDFLAGS += -L../../../lib/libfmd -lfmd

SUBDIRS = \
	cpumem \
	inject \
	network \
	disk \
	mpio \
	pcie \
	sysagent \
	proc \
	raid \
	topo
#	apache \

all install clean uninstall: $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

FRC:
