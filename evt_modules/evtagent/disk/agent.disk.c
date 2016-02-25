/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    agent_disk.c
 * Author:      Inspur OS Team 
			guoms@inspur.com
 * Date:        2015-08-04
 * Description: Process disk check error, and log
 *
 ************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <evt_agent.h>
#include <fmd_event.h>
#include <unistd.h>
#include <errno.h>

#include <fmd_api.h>
#include "logging.h"
#include "fmd_module.h"
#include "fmd_log.h"
#include "fm_disk.h"

#include "wrap.h"


int getmasternum(const char *path, char *master){
    char buff[8] = {0};
    char cmd[64] = {0};
    char num[8] = {0};

    sprintf(cmd, "ls -l %s |awk '{print $5}' | cut -f 1 -d ','", path);
     
    FILE *fstream = popen(cmd,"r");
    if(fstream == NULL)
    {
        wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
        return 0;
    }	
    if(fgets(buff, sizeof(buff), fstream) !=  NULL);
	{
		strncpy(master, buff, strlen(buff)-1);
    }
    pclose(fstream);
    
    return 0;

}

int getslavenum(const char *path,  char *slave){
    char buff[8] = {0};
    char cmd[64] = {0};
    char num[8] = {0};

    sprintf(cmd, "ls -l %s |awk '{print $6}'", path);
     
    FILE *fstream = popen(cmd,"r");
    if(fstream == NULL)
    {
        wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
        return 0;
    }	
    if(fgets(buff, sizeof(buff), fstream) !=  NULL);
	{
		strncpy(slave, buff, strlen(buff)-1);
    }
    pclose(fstream);
    
    return 0;

}

int getprocid(const char *path) {
	char buff[8] = {0};
    char cmd[64] = {0};
    char num[8] = {0};
	char *slave = "0";

    sprintf(cmd, "ps -aux | grep 'dd if=%s of=/dev/null' | awk '{print $2}'", path);
     
    FILE *fstream = popen(cmd,"r");
    if(fstream == NULL)
    {
        wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
        return 0;
    }	
	int fd = 0;
	char *file = "/sys/fs/cgroup/blkio/tasks";
	if((fd = open(file, O_RDWR)) == -1) {
		wr_log("disk_agent", WR_LOG_ERROR,
			"failed to open file !!!");
		return -1;
	}
	
	
	while(fgets(buff, sizeof(buff),fstream) != NULL)
    {
       write(fd, buff, strlen(buff));
	   
	}
	
    close(fd);
    pclose(fstream);
    
    return 0;
}
/** 
*	throttle read disk 
* 
*	@result: *		
		#echo '8:0   1048576'> /sys/fs/cgroup/blkio/blkio.throttle.read_bps_device
		8:masternum
		0:slavenum
*/
int
throttle_read_disk(char *file, char *dev_name)
{
	int fd = 0;
	char masternum[8] = {0};
	char slavenum[8] = {0};
	char *readthread = "204800";
	
	if((fd = open(file, O_RDWR)) == -1) {
		wr_log("disk_agent", WR_LOG_ERROR,
			"failed to open file !!!");
		return -1;
	}
	char buf[64];
	memset(buf, 0, 64 * sizeof(char));
	memset(masternum, 0, sizeof(masternum));
	memset(slavenum, 0, sizeof(slavenum));
	
	getmasternum(dev_name, masternum);
	getslavenum(dev_name, slavenum);
	
	sprintf(buf,"%s:%s %s", masternum,slavenum,readthread);
	
	write(fd, buf, strlen(buf));
	close(fd);
	return 0;
}

static inline int
handle_disk_fault_event(fmd_event_t * e)
{
	int ret = 0;
	char *evt_class;
	char *evt_devname = e->dev_name;
	char *overtemp = "over-temperature";
	char *unspace = "space-insufficient";
	char *unhealthy = "unhealthy";
	
	
	evt_class = e->ev_class;
	
	
	if(strstr(evt_class, "over-temperature") != NULL) {
		char buf[64];
		memset(buf, 0, 64 * sizeof(char));
		char *throttleread = "blkio.throttle.read_bps_device"; 
		sprintf(buf,"/sys/fs/cgroup/blkio/%s", throttleread);
		if ((ret = throttle_read_disk(buf, evt_devname)) == 0 && (ret = getprocid(evt_devname)) == 0) {
			//return LIST_ISOLATED_SUCCESS;
			return LIST_REPAIRED_SUCCESS;
		}else{
			return LIST_REPAIRED_FAILED;
		}
	}else if(strstr(evt_class, "space-insufficient") != NULL){
		//handle.....
		return LIST_REPAIRED_SUCCESS;
	}else if(strstr(evt_class, "unhealthy") != NULL){
		//handle.....
		return LIST_REPAIRED_SUCCESS;
	}else {
		//handle.....
		return LIST_REPAIRED_FAILED;
	}
	
}

static int
disk_log_event(fmd_event_t *pevt)
{
	int type, fd;
	int tsize, ecsize, bufsize;
	char *dir;
	char *eclass = NULL;
	char times[26];
	char buf[128];

	char dev_name[32];
	char dev_detail[32];
	time_t evttime;

	evttime = pevt->ev_create;
	eclass = pevt->ev_class;

	strcpy(dev_name, pevt->dev_name);

	if(pevt->event_type == EVENT_LIST)
	{
		dir = "/var/log/fms/disk/list";
		type = FMD_LOG_LIST;
        
	}else if(pevt->event_type == EVENT_FAULT)
	{
		dir = "/var/log/fms/disk/fault";
		type = FMD_LOG_FAULT;
        
	}else{
		dir = "/var/log/fms/disk/serd";
		type = FMD_LOG_ERROR;
	}


	if ((fd = fmd_log_open(dir, type)) < 0) {
		wr_log("disk_agent", WR_LOG_ERROR,
			"FMD:failed to record log for event: %s\n", eclass);
		return -1;
	}

	fmd_get_time(times, evttime);
	memset(buf, 0, 128);
	snprintf(buf, sizeof(buf), "%s\t%s\t%s\n", times, eclass, dev_name);

	if (fmd_log_write(fd, buf, strlen(buf)) != 0) {
		wr_log("disk_agent", WR_LOG_ERROR, "FMD: failed to write log file for event: %s\n", eclass);
		return -1;
	}
    
	fmd_log_close(fd);
	return 0;
}

/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
disk_handle_event(fmd_t *pfmd, fmd_event_t *event)
{
	int action = 1;
	int ret = 0;
	wr_log("disk_agent", WR_LOG_DEBUG, "disk agent ev_handle fault event to list event.");

	// every fault, ereport ,serd to log.
	disk_log_event(event);

	if(event->ev_flag == AGENT_TODO)
	{
        //todo some
        wr_log("disk_agent", WR_LOG_DEBUG, "disk agent to do something and return list event.");
		ret = handle_disk_fault_event(event);

		event->event_type = EVENT_LIST;
        // to log list event
        disk_log_event(event);
		
        action = 1;
	}
    if(!action) {
		wr_log("disk_agent", WR_LOG_DEBUG, "disk agent log event.");
		return NULL;
	}
	
	return (fmd_event_t *)fmd_create_listevent(event, ret);
	
}


static agent_modops_t disk_mops = {
	.evt_handle = disk_handle_event,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	wr_log("disk_agent", WR_LOG_DEBUG, "disk_agent thread init");
	return (fmd_module_t *)agent_init(&disk_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	return;
}
