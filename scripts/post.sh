#!/bin/sh
#
# File:
#	post.sh
#
# Author: Inspur OS Team
# 
# Created:
#	Mar 3, 2010
#
# Description:
#	Run before rpm hash been installed.
#

### comman defs ###
MKDIR='/bin/mkdir -p'
CP='/bin/cp -f'
CPP=/usr/bin/cpp
LDCONFIG=/sbin/ldconfig

### variable defs ###
INSTDIR=/usr/local/fms
SCRIPTSDIR=${INSTDIR}/scripts

ROOTLIB=/usr/lib/fm
INCDIR=/usr/include/fm
LOGDIR=/var/log/fm
ESCDIR=${ROOTLIB}/eversholt
FMDDIR=${ROOTLIB}/fmd
PLUGDIR=${ROOTLIB}/fmd/plugins
LDCONFDIR=/etc/ld.so.conf.d

### fminject ###
${CP} ${SCRIPTSDIR}/*.inj ${FMDDIR}

### ESC ###
cd ${ESCDIR}
${CPP} mpio.esc > mpio.esc.cpp 2>/dev/null
${CPP} cpumem.esc > cpumem.esc.cpp 2>/dev/null
${CPP} disk.esc > disk.esc.cpp 2>/dev/null
${CPP} nic.esc > nic.esc.cpp 2>/dev/null
${CPP} pcie.esc > pcie.esc.cpp 2>/dev/null
${CPP} service.esc > service.esc.cpp 2>/dev/null
${CPP} topo.esc > topo.esc.cpp 2>/dev/null
cd -

### lock ###
touch /var/lock/subsys/fmd

### syslog ###
sed -i '/fmd/d' /etc/syslog.conf
echo "" >> /etc/syslog.conf
echo "# Save fmd messages to fmd.log" >> /etc/syslog.conf
echo "local1.*						/var/log/fm/fmd.log" >> /etc/syslog.conf
/sbin/service syslog restart

### service fmd ###
rm -f /etc/rc.d/init.d/fmd
cp -f ${SCRIPTSDIR}/fmd /etc/rc.d/init.d 1>/dev/null 2>&1
chkconfig --add fmd

### message queue ###
${MKDIR} /dev/mqueue
mount -t mqueue none /dev/mqueue 1>/dev/null 2>&1

### lib ###
${CP} ${SCRIPTSDIR}/fmd.conf ${LDCONFDIR}
${LDCONFIG}

### close salinfo service ###
/sbin/service salinfo_decode stop
/sbin/chkconfig --del salinfo_decode

