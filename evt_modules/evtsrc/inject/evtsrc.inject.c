/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    disk_evtsr.c
 * Author:      Inspur OS Team 
                guomeisi@inspur.com
 * Date:        20xx-xx-xx
 * Description: evtsrc for fminject CMD
 *
 ************************************************************/

#include <stdint.h>
#include <mqueue.h>
#include <errno.h>

#include "wrap.h"
#include "fmd.h"
#include "fmd_nvpair.h"
#include "fmd_api.h"
#include "evt_src.h"
#include "fmd_module.h"

#define MODULE_NAME "evtsrc.inject"

#define INJECT_ECPUMEM	"ereport.cpu.intel"
#define INJECT_EMEM	"ereport.cpu.intel.mem_dev"
#define INJECT_EDISK	"ereport.io.disk"
#define INJECT_ETOPO	"ereport.topo"

#define INJECT_CPU	 2
#define INJECT_MEM	0x100000000
#define INJECT_DISK	"/dev/sdd"


/**
 * cpu_create
 *
 * TODO nvlist operations
 */
static struct list_head *
inject_probe(evtsrc_module_t *emp)
{
	char 			*buff = NULL;
	struct mq_attr 	attr;
	ssize_t 		size;
	fmd_t 			*pfmd = ((fmd_module_t *)emp)->p_fmd;
	char 			*rscid = NULL;
	struct list_head *head = nvlist_head_alloc();
	
	mqd_t 			mqd = mq_open("/fmd", O_RDWR | O_CREAT | O_NONBLOCK, 0644, NULL);

	if(mqd == -1){
		syslog(LOG_ERR, "mq_open");
		return NULL;
	}
	
	mq_getattr(mqd, &attr);
	buff = (char *)calloc(attr.mq_msgsize, sizeof(char));
	size = mq_receive(mqd, buff, attr.mq_msgsize, NULL);
	if(size == -1){
		if(errno == EAGAIN){
			mq_close(mqd);
			free(buff);
			wr_log("", WR_LOG_ERROR, "so module mq receive message error.");
			return NULL;
		} else {
			wr_log("", WR_LOG_ERROR, "so module mq_receive %s\n", strerror(errno));
			mq_close(mqd);
			free(buff);
			return NULL;
		}
	}

	wr_log("", WR_LOG_DEBUG, "inject receive is [%s].......", buff);
	mq_close(mqd);

	rscid = INJECT_DISK;  /* disk ereport */

	nvlist_t 	*nvl = NULL;
	nvl = nvlist_alloc();
    sprintf(nvl->name, rscid);
    strcpy(nvl->value, buff);

    nvlist_add_nvlist(head, nvl);

	return head;
	
}


static evtsrc_modops_t inject_mops = {
	.evt_probe = inject_probe,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)evtsrc_init(&inject_mops, path, pfmd);
}

void
fmd_module_fini(fmd_module_t *mp)
{
	return;
}
