/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    disk_evtsr.c
 * Author:      Inspur OS Team 
                guomeisi@inspur.com
 * Date:        20xx-xx-xx
 * Description: 
 *
 ************************************************************/

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>

#include <fmd.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>
#include <errno.h>

#include "fm_diskspacedect.h"  /* for disk space insufficient error */
#include "fm_disksmartdect.h"
#include "fm_badblockdect.h"
#include "fm_cmd.h"
#include "logging.h"



static fmd_t *p_fmd;

/*
 * Input
 * head   -  list of all event
 * nvl    -  current event
 * fullclass
 * ena
 *
 */
int 
disk_fm_event_post(struct list_head *head, nvlist_t * nvl, char * fullclass, char * ena)
{
	sprintf(nvl->name, ena);
	strcpy(nvl->value, fullclass);
	
	nvlist_add_nvlist(head, nvl);

	return 0;
	
}


struct list_head *
disk_probe(evtsrc_module_t * emp)
{
	char 	*ena = NULL;
	char fullclass[PATH_MAX];
	char *fault;
	nvlist_t *nvl;
	struct list_head *head = nvlist_head_alloc();

	p_fmd = emp->module.p_fmd;

	char *diskname = NULL;//for disk space insufficient
	char *fullpath = NULL;
	char fullpathtmp[9];
	char *result = NULL;
	FILE *fstream = NULL;
    char buff[LINE_MAX];
    memset(buff,0,sizeof(buff));
	char* cmd = NULL;
	char cmdresult[PATH_MAX];
	
	/* get disk`s name */
	cmd = disknamecmd(cmdresult);  
				
    fstream = popen(cmd,"r");
    if(fstream == NULL)
    {
        fprintf(stderr,"execute command failed: %s",strerror(errno));
    }	
		
    while(fgets(buff, sizeof(buff),fstream) != NULL)
	{
		//get device name ,eg:/dev/sda
        strncpy(fullpathtmp,buff,sizeof(fullpathtmp)-1);
        fullpathtmp[sizeof(fullpathtmp)-1] = '\0';

		// the same disk, check  only  once
        if(strcmp(fullpathtmp,result)!=0)
        {
            fullpath = fullpathtmp;
        
			
			// if smart is Available and Enable
			if(DoesSmartWork(fullpath) == 1){
				
				 /* ////////////////////////////////////////////////////////
				  * smartctl check : disk unhealthy error
				  * ////////////////////////////////////////////////////////  */
				if(disk_unhealthy_check(fullpath)== 1){
					memset(fullclass, 0, sizeof(fullclass));
					sprintf(fullclass,"%s.io.disk.unhealthy", FM_EREPORT_CLASS);
					ena = fullpath;
					nvl = nvlist_alloc();
					if (nvl == NULL) {
						fprintf(stderr,"DISK: out of memory\n");
						return NULL;
					}
					if ( disk_fm_event_post(head, nvl, fullclass, ena) != 0 ) {
						nvlist_free(nvl);
						nvl = NULL;
					}
				}


				/* ////////////////////////////////////////////////////
				 * smartctl check : disk over-temperature error
			 	 * /////////////////////////////////////////////////////  */
				if (disk_temperature_check(fullpath)== 1){
					memset(fullclass, 0, sizeof(fullclass));
					sprintf(fullclass,"%s.io.disk.over-temperature", FM_EREPORT_CLASS);
					ena = fullpath;
					nvl = nvlist_alloc();
					if (nvl == NULL) {
						fprintf(stderr,"DISK: out of memory\n");
						return NULL;
					}
					if ( disk_fm_event_post(head, nvl, fullclass, ena) != 0 ) {
						nvlist_free(nvl);
						nvl = NULL;
					}
				}
			}

			
		/* ////////////////////////////////////////////////////
		 * disk badblocks chenck : disk`s badblocks error
		 * ////////////////////////////////////////////////////  */
			if (disk_badblocks_check(fullpath) == 1){		
				memset(fullclass, 0, sizeof(fullclass));
				sprintf(fullclass,"%s.io.disk.badblocks", FM_EREPORT_CLASS);
				ena = fullpath;
				nvl = nvlist_alloc();
				if (nvl == NULL) {
					fprintf(stderr,"DISK: out of memory\n");
					return NULL;
				}
				if ( disk_fm_event_post(head, nvl, fullclass, ena) != 0 ) {
					nvlist_free(nvl);
					nvl = NULL;
				}
			}

			result = fullpath;  //flag  the device name  for  strcmp  (Avoid a repeat of the disk check)

		}else{
        		continue;
        }
    }
	pclose(fstream);

/* ////////////////////////////////////////////////////
 * disk space chenck : disk space insufficient error
 * ////////////////////////////////////////////////////  */

	if (disk_space_check() == 1){
		memset(fullclass, 0, sizeof(fullclass));
		sprintf(fullclass,"%s.io.disk.space-insufficient", FM_EREPORT_CLASS);
		ena = NULL;
		nvl = nvlist_alloc();
		if (nvl == NULL) {
			fprintf(stderr,"DISK: out of memory\n");
			return NULL;
		}
		if ( disk_fm_event_post(head, nvl, fullclass, ena) != 0 ) {
			nvlist_free(nvl);
			nvl = NULL;
		}
	}
}

static evtsrc_modops_t disk_mops = {
		.evt_probe = disk_probe,
};

fmd_module_t *
fmd_init(char * path, fmd_t * pfmd)
{
 		return (fmd_module_t *)evtsrc_init(&disk_mops, path, pfmd);
}
void 
fmd_fini(fmd_module_t * mp)
{
	return;
}
