#makefile
#
# Created on: Jan 19, 2010
#     Author: Inspur OS Team
#
#

include ../Makefile.defs

SUBDIRS = \
	libmod \
	libevtsrc \
	libagent

all clean install uninstall: $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

FRC:
