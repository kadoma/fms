#makefile
#
# Created on: Jan 19, 2010
#     Author: Inspur OS Team
#
#

include ../Makefile.defs

SUBDIRS = \
	agent \
	evtsrc

all install clean uninstall: $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

FRC:
