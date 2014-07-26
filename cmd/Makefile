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
	fmd \
	fminject \
	fmtopo \
	fmadm \
	fmdump \
	fmtmsg \
	fmckpt \
	modules

all clean install uninstall: $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@;	pwd; $(MAKE) $(TARGET)

FRC:
