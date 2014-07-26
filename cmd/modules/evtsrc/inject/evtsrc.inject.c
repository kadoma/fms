/*
 * evtsrc.inject.c
 *
 *  Created on: Jan 16, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	evtsrc for fminject CMD
 */

#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <fmd.h>
#include <nvpair.h>
#include <fmd_api.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>

#define MODULE_NAME "evtsrc.inject"

#define INJECT_ECPUMEM	"ereport.cpu.intel"
#define INJECT_EMEM	"ereport.cpu.intel.mem_dev"
#define INJECT_EDISK	"ereport.io.disk"
#define INJECT_EMPIO	"ereport.io.mpio"
#define INJECT_ENIC	"ereport.io.network"
#define INJECT_EPCIE	"ereport.io.pcie"
#define INJECT_ETOPO	"ereport.topo"

#define INJECT_CPU	 2
#define INJECT_MEM	0x100000000
#define INJECT_DISK	"sda"
#define INJECT_NIC	"eth0"
#define INJECT_PCIE	"eth0"

struct list_head *
inject_probe(evtsrc_module_t *emp)
{
	fmd_debug;

	return NULL;
}

/**
 * cpu_create
 *
 * TODO nvlist operations
 */
fmd_event_t *
inject_create(evtsrc_module_t *emp, nvlist_t *pnv)
{
	char 			*buff = NULL;
	struct mq_attr 	attr;
	ssize_t 		size;
	fmd_t 			*pfmd = ((fmd_module_t *)emp)->mod_fmd;
	uint64_t 		rscid;
	mqd_t 			mqd = mq_open("/fmd", O_RDWR | O_CREAT | O_NONBLOCK, 0644, NULL);

	if(mqd == -1) {
		syslog(LOG_ERR, "mq_open");
		return NULL;
	}
//printf("##DEBUG##: fmd_inject_module: before mq_receive()\n");
	mq_getattr(mqd, &attr);
	buff = (char *)calloc(attr.mq_msgsize, sizeof(char));
	size = mq_receive(mqd, buff, attr.mq_msgsize, NULL);
//printf("##DEBUG##: fmd_inject_module: after mq_receive() size=%d\n", size);
	if(size == -1) {
		if(errno == EAGAIN) {
			mq_close(mqd);
			free(buff);
			return NULL;
		} else {
			syslog(LOG_ERR, "mq_receive %s\n", strerror(errno));
			mq_close(mqd);
			free(buff);
			return NULL;
		}
	}

	syslog(LOG_DEBUG, "%s\n", buff);
	mq_close(mqd);

	/* I think we should allocate an reasonable rscid */
	if ( strstr(buff, INJECT_ECPUMEM) ) {		/* cpumem ereport */
		if ( strstr(buff, INJECT_EMEM) ) 
			rscid = topo_get_memid(pfmd, INJECT_MEM);
		else
			rscid = topo_get_cpuid(pfmd, INJECT_CPU);
		if ( rscid == 0 )
			return NULL;
	}
	else if ( strstr(buff,  INJECT_EDISK) )		/* disk ereport */
		rscid = topo_get_storageid(pfmd, INJECT_DISK);
	else if ( strstr(buff, INJECT_ENIC) )		/* network ereport*/
		rscid = topo_get_pcid(pfmd, INJECT_NIC);
	else if ( strstr(buff, INJECT_EPCIE) )		/* FIXME may have problem */
		rscid = topo_get_pcid(pfmd, INJECT_PCIE);
	/* TODO but we still lack sth...
	else if ( strstr(buff, INJECT_EMPIO) )
		;
	else if ( strstr(buff, INJECT_ETOPO) )
		;
	*/
	else
		rscid = topo_get_cpuid(pfmd, INJECT_CPU);

	return (fmd_event_t *)fmd_create_ereport(pfmd, buff, rscid, pnv);
}


static evtsrc_modops_t inject_mops = {
	.evt_probe = inject_probe,
	.evt_create = inject_create,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)evtsrc_init(&inject_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}
