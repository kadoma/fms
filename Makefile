##############################
# file   Makefile
###############################

FMD_DIR = ./fms
EVT_PREFIX = ./evt_modules
KFM_DIR = $(EVT_PREFIX)/evtlib/kfm/kfm
KFMADM_DIR = $(EVT_PREFIX)/evtlib/kfm/kfmadm
EVT_SRC_DIR = $(EVT_PREFIX)/evtsrc
EVT_AGENT_DIR = $(EVT_PREFIX)/evtagent
INSTALL_KFM_DIR = /lib/modules/$(shell uname -r)/kernel/fm
INSTALL_KFMADM_DIR = /usr/sbin/

PROJECT = fms
PROGRAM = fmd

#SUBDIRS=`ls -d */ | grep -v 'bin' | grep -v 'lib' | grep -v 'include'`
SUBDIRS=lib/libcase lib/libesc lib/libfmd evt_modules/evtlib  \
				lib/libadm lib/libmsg lib/libtopo \
									fms \
				tools/fmstopo tools/fmsinject tools/fmsadm \
				evt_modules/evtsrc/trace evt_modules/evtagent/trace \
				evt_modules/evtsrc/inject evt_modules/evtagent/inject \
				evt_modules/evtsrc/disk evt_modules/evtagent/disk \
				evt_modules/evtsrc/adm \
				evt_modules/evtsrc/cpumem evt_modules/evtagent/cpumem \
				evt_modules/evtlib/kfm/kfm evt_modules/evtlib/kfm/kfmadm				


define make_subdir
 @for subdir in $(SUBDIRS) ; do \
 ( cd $$subdir && make $1) \
 done;
endef

all:
	$(call make_subdir , all)

install :
	$(call make_subdir , install)

clean:
	$(call make_subdir , clean)


###   for rpm   ###
PKG=$(PROJECT)
RPM_BUILD_ROOT=/root/rpmbuild/BUILD
BRROOTDIR=$(RPM_BUILD_ROOT)/$(PKG)
RPM=/usr/bin/rpmbuild
RPMFLAGS=-bb
FMS_SPEC_FILE=fms.spec
RPM_FMS=$(BRROOTDIR)/usr/sbin/

rpm: all
	rm -rf $(BRROOTDIR)/*
	mkdir -p $(BRROOTDIR)/usr/sbin
#	mkdir -p $(BRROOTDIR)/etc/$(PROJECT)
	mkdir -p $(BRROOTDIR)/var/log/$(PROJECT)
	mkdir -p $(BRROOTDIR)/$(INSTALL_KFM_DIR)
	mkdir -p $(BRROOTDIR)/$(INSTALL_KFMADM_DIR)
	mkdir -p $(BRROOTDIR)/lib/systemd/system
	mkdir -p $(BRROOTDIR)/var/run
	mkdir -p $(BRROOTDIR)/usr/lib/$(PROJECT)
	mkdir -p $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	mkdir -p $(BRROOTDIR)/usr/lib/$(PROJECT)/escdir
	
	install -cm 755 $(FMD_DIR)/$(PROGRAM)   $(RPM_FMS)
	install -cm 755 ./tools/fmsadm/fmsadm   $(RPM_FMS)
	install -cm 755 ./tools/fmstopo/fmstopo $(RPM_FMS)
	install -cm 755 $(KFMADM_DIR)/kfmadm    $(BRROOTDIR)/$(INSTALL_KFMADM_DIR)
	cp ./fmd.service                        $(BRROOTDIR)/lib/systemd/system/fmd.service
	cp ./lib/libcase/*.so                   $(BRROOTDIR)/usr/lib/$(PROJECT)
	cp ./lib/libesc/*.so                    $(BRROOTDIR)/usr/lib/$(PROJECT)
	cp ./lib/libfmd/*.so                    $(BRROOTDIR)/usr/lib/$(PROJECT)
	cp ./lib/libfmd_adm/*.so                $(BRROOTDIR)/usr/lib/$(PROJECT)
	cp ./lib/libfmd_msg/*.so                $(BRROOTDIR)/usr/lib/$(PROJECT)
	cp ./lib/libtopo/*.so                   $(BRROOTDIR)/usr/lib/$(PROJECT)
	cp ./evt_modules/evtlib/*.so            $(BRROOTDIR)/usr/lib/$(PROJECT)
	cp $(EVT_SRC_DIR)/disk/*.so             $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(EVT_AGENT_DIR)/disk/*.so           $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(EVT_SRC_DIR)/cpumem/*.so           $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(EVT_AGENT_DIR)/cpumem/*.so         $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(EVT_SRC_DIR)/trace/*.so            $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(EVT_AGENT_DIR)/trace/*.so          $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(EVT_SRC_DIR)/adm/*.so              $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(EVT_SRC_DIR)/inject/*.so           $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(EVT_AGENT_DIR)/inject/*.so         $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp ./fms_conf/escdir/*.esc              $(BRROOTDIR)/usr/lib/$(PROJECT)/escdir
	cp ./fms_conf/plugins/*.conf            $(BRROOTDIR)/usr/lib/$(PROJECT)/plugins
	cp $(KFM_DIR)/kfm.ko                    $(BRROOTDIR)/$(INSTALL_KFM_DIR)/fms.ko
	$(RPM) $(RPMFLAGS) $(FMS_SPEC_FILE)
