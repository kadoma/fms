#!/bin/sh
#
# File:
#	preun.sh
#
# Author: Inspur OS Team
# 
# Created:
#	Mar 3, 2010
#
# Description:
#	Run before rpm is been removed.
#

### comman defs ###
MKDIR='/bin/mkdir -p'
CP='/bin/cp -f'
CPP=/usr/bin/cpp
LDCONFIG=/sbin/ldconfig

### variable defs ###
ROOTLIB=/usr/lib/fm
INCDIR=/usr/include/fm
LOGDIR=/var/log/fm
ESCDIR=${ROOTLIB}/eversholt
FMDDIR=${ROOTLIB}/fmd
PLUGDIR=${ROOTLIB}/fmd/plugins
LDCONFDIR=/etc/ld.so.conf.d

### lock ###
rm -f /var/lock/subsys/fmd

### syslog ###
sed -i '/fmd/d' /etc/syslog.conf
/sbin/service syslog restart

### service fmd ###
/sbin/service fmd stop
chkconfig --del fmd
rm -f /etc/rc.d/init.d/fmd

### message queue ###
umount /dev/mqueue 2>/dev/null
rm -rf /dev/mqueue 2>/dev/null

### lib ###
rm -f ${LDCONFDIR}/fmd.conf
${LDCONFIG}
