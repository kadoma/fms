/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    disk_evtsr.c
 * Author:      Inspur OS Team 
                	guomeisi@inspur.com
 * Date:        2015-08-04
 * Description: disk error evtsrc module.
 *
 ************************************************************/

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <errno.h>

#include "fmd.h"
#include "fmd_module.h"
#include "fmd_nvpair.h"
#include "fm_diskspacedect.h"  /* for disk space insufficient error */
#include "fm_disksmartdect.h"
#include "fm_badblockdect.h"
#include "fm_cmd.h"
#include "logging.h"
#include "evt_src.h"
#include "fmd_common.h"
#include "fm_disk.h"

static fmd_t *p_fmd;

void
disk_fm_event_post(struct list_head *head, nvlist_t * nvl, char * fullclass, char *path)
{
    strcpy(nvl->name, path);
    strcpy(nvl->value, fullclass);
    nvl->data=strdup("disk description info");
    nvlist_add_nvlist(head, nvl);
}

char* cparray(char *dest, const char *src)
{
    int i;
    char* tmp = dest;
    const char* s = src;
    for(i = 0; i < strlen(src); i++)
    {
        tmp[i] = s[i];
    }
	tmp[i] = '\0';
    return dest;
}

struct list_head *
disk_probe(evtsrc_module_t * emp)
{
	struct list_head *head = nvlist_head_alloc();

	char fullpath[64] = {0};
    char buff[LINE_MAX] = {0};
    char fullclass[128] = {0};
    char cmd[128] = {0};
    
    sprintf(cmd, "df -h|awk '{print $1}'|grep '/dev'");
    FILE *fstream = popen(cmd,"r");
    if(fstream == NULL)
    {
        wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
		return NULL;
    }	
		
    while(fgets(buff, sizeof(buff),fstream) != NULL)
	{
	    memset(fullpath, 0, sizeof(fullpath));
		strncpy(fullpath, buff, strlen(buff)-1);

	    if(disk_space_check(fullpath) == 1)
	    {
		    memset(fullclass, 0, sizeof(fullclass));
		    sprintf(fullclass,"%s.disk.space-insufficient", FM_EREPORT_CLASS);
		    nvlist_t *nvl = nvlist_alloc();
            
		    if(nvl == NULL)
            {
		        wr_log("stderr",WR_LOG_ERROR, "DISK: out of memory\n");
			    return NULL;
		    }
            nvl->evt_id = 1;
		    disk_fm_event_post(head, nvl, fullclass, fullpath);
        }

        int len = strlen(fullpath);
        char n = fullpath[len - 1];
        int nn = atoi(&n);
        if(nn >= 1 && nn <= 9)
            fullpath[len -1] = '\0';
        
		if(disk_unhealthy_check(fullpath))
        {
			memset(fullclass, 0, sizeof(fullclass));
            sprintf(fullclass,"%s.disk.unhealthy", FM_EREPORT_CLASS);
            
            nvlist_t *nvl = nvlist_alloc();
			if(nvl == NULL) {
				wr_log("stderr",WR_LOG_ERROR,"DISK: out of memory\n");
				return NULL;
			}
            nvl->evt_id = 2;
			disk_fm_event_post(head, nvl, fullclass, fullpath);
		}
				
		if(disk_temperature_check(fullpath))
        {
		    memset(fullclass, 0, sizeof(fullclass));
			sprintf(fullclass,"%s.disk.over-temperature", FM_EREPORT_CLASS);
                
			nvlist_t *nvl = nvlist_alloc();
			if(nvl == NULL){
			    wr_log("stderr",WR_LOG_ERROR,"DISK: out of memory\n");
				return NULL;
			}
            // 3 temp fault
            nvl->evt_id = 3;
			disk_fm_event_post(head, nvl, fullclass, fullpath);
		}
    }
    
	pclose(fstream);
	return head;
}

static evtsrc_modops_t disk_mops = {
		.evt_probe = disk_probe,
};

fmd_module_t *
fmd_module_init(char * path, fmd_t * pfmd)
{
 		return (fmd_module_t *)evtsrc_init(&disk_mops, path, pfmd);
}
void 
fmd_module_finit(fmd_module_t * mp)
{
	return;
}
