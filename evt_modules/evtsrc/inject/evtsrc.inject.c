/************************************************************
* Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    evtsrc.inject.c
 * Author:      Inspur OS Team 
                guomeisi@inspur.com
 * Date:        2015-08-04
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
#include "logging.h"
#include "fm_disk.h"

#define MODULE_NAME "evtsrc.inject"

#define INJECT_ECPUMEM	"ereport.cpu.intel"
#define INJECT_EMEM	"ereport.cpu.intel.mem_dev"
#define INJECT_EDISK	"ereport.disk"
#define INJECT_ETOPO	"ereport.topo"

#define INJECT_CPU	 2
#define INJECT_MEM	0x100000000
#define INJECT_DISK	"disk"
#define INJECT_DISKNAME "/dev/sda"

/**
 * cpu_create
 *
 * TODO nvlist operations
 */
static struct list_head *
inject_probe(evtsrc_module_t *emp)
{
	struct fms_disk *fms_disk;
	fms_disk = def_calloc(0,sizeof(struct fms_disk));
	char 			*buff = NULL;
	struct mq_attr 	attr;
	ssize_t 		size;
	fmd_t 			*pfmd = ((fmd_module_t *)emp)->p_fmd;
	char 			*rscid = NULL;
	char			*diskname = NULL;
	struct list_head *head = nvlist_head_alloc();
	
	mqd_t 			mqd = mq_open("/fmd", O_RDWR | O_CREAT | O_NONBLOCK, 0644, NULL);

	wr_log("inject_evtsrc", WR_LOG_DEBUG, "start inject event\n");

	if(mqd == -1){
		wr_log("inject_evtsrc", WR_LOG_ERROR, "failed to mq_open\n");
		return NULL;
	}
	
	mq_getattr(mqd, &attr);
	buff = (char *)def_calloc(attr.mq_msgsize, 1);
	size = mq_receive(mqd, buff, attr.mq_msgsize, NULL);
	if(size == -1){
		if(errno == EAGAIN){
			wr_log("inject_evtsrc", WR_LOG_WARNING, "so module mq has not message,please waiting for message.\n");
			mq_close(mqd);
			free(buff);
			return NULL;
		} else {
			wr_log("inject_evtsrc", WR_LOG_ERROR, "so module mq_receive error.\n");
			mq_close(mqd);
			free(buff);
			return NULL;
		}
	}

	wr_log("inject_evtsrc", WR_LOG_DEBUG, "inject receive is [%s].......", buff);
	mq_close(mqd);

	rscid = INJECT_DISK;  /* disk ereport */
	diskname = INJECT_DISKNAME;

	nvlist_t 	*nvl = NULL;
	nvl = nvlist_alloc();
    sprintf(nvl->name, rscid);
	
	sprintf(fms_disk->detail, diskname);
	
	nvl->data = fms_disk;
    strcpy(nvl->value, buff);
	if(strcmp(buff,"ereport.disk.unhealthy") == 0)
	{
		nvl->evt_id = 1;
	}else if(strcmp(buff,"ereport.disk.space-insufficient") == 0){
		nvl->evt_id = 2;
	}else if(strcmp(buff,"ereport.disk.over-temperature") == 0){
		nvl->evt_id = 3;
	}else if(strcmp(buff,"ereport.disk.badblocks") == 0){
		nvl->evt_id = 4;
	}else{
		nvl->evt_id = 0;
	}

	

    nvlist_add_nvlist(head, nvl);

	return head;
	
}


static evtsrc_modops_t inject_mops = {
	.evt_probe = inject_probe,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&inject_mops, path, pfmd);
}

void
fmd_module_fini(fmd_module_t *mp)
{
	return;
}
