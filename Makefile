# Makefile
#
# Created on: Jan 19, 2010
#	Author: Inspur OS Team
#
# Delete the following two lines on IA64:
# finish:
#	...
#	$(CHCON) $(ROOTLIB)/*.so
#	$(CHCON) $(PLUGDIR)/*.so
#

include ./Makefile.defs

SUBDIRS = \
	lib \
	cmd

#all: prep $(SUBDIRS) preun
all: $(SUBDIRS) 

install: prepins $(SUBDIRS) post

clean: $(SUBDIRS)

uninstall: preun $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@;	pwd; $(MAKE) $(TARGET)

FRC:

prepins:
	${MKDIR} ${INCDIR}
	${MKDIR} ${INSTDIR}
	${MKDIR} ${INSCTDIR}
	${MKDIR} ${ERRLOGDIR}
	${MKDIR} ${FLTLOGDIR}
	${MKDIR} ${LSTLOGDIR}
	${MKDIR} ${ROOTLIB}
	${MKDIR} ${ESCDIR}
	${MKDIR} ${FMDDIR}
	${MKDIR} ${DICTDIR}
	${MKDIR} ${DICTDIRS}
	${MKDIR} ${CKPTDIR}
	${MKDIR} ${NOWDIR}
	${MKDIR} ${PLUGDIR}
	${CP} include/*.h ${INCDIR}
	${CP} scripts/* ${INSCTDIR}
	${CP} esc/*.esc ${ESCDIR}
	${CP} dict/*.po ${DICTDIR}
	${CP} dict/*.mo ${DICTDIRS}
	${CP} cmd/fmckpt/ckpt.conf ${CKPTDIR}
	${CP} cmd/modules/evtsrc/proc/evtsrc.proc.monitor.xml ${PLUGDIR}

preun:
	${RM} ${CKPTDIR}/ckpt.conf
	${RM} ${PLUGDIR}/evtsrc.proc.monitor.xml
	./scripts/preun.sh

post:
	./scripts/post.sh
