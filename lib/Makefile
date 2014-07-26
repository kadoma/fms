# Makefile
#
# Created on: Jan 19, 2010
#     Author: Inspur OS Team
#
# Delete the following two lines on IA64:
# finish:
#	...
#	$(CHCON) $(ROOTLIB)/*.so
#	$(CHCON) $(PLUGDIR)/*.so
#

include ./Makefile.defs

SUBDIRS = \
	libnvpair \
	libesc \
	libfmd \
	libfmd_adm \
	libfmd_msg \
	libtopo \
	modules

all install uninstall clean: $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@;	pwd; $(MAKE) $(TARGET)

FRC:
