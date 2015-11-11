/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fm_diskspacedect.c
 * Author:      Inspur OS Team 
                	guomeisi@inspur.com
 * Date:        2015-08-04
 * Description: check if disk space is sufficient.
 *
 ************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "fm_diskspacedect.h"
#include "logging.h"


/*
 *  disk_space_check
 *     check if disk space is sufficient 
 *     use "df -P"
 *
 *  return value
 *   0       if disk space is enough
 *   1       if disk space is insufficient
 */
 
int disk_space_check(void)
{
		
        FILE *fstream = NULL;
        char buff[1024];
        memset(buff,0,sizeof(buff));

		/* get disk use% */
        char* cmd = "df -P | grep /dev | awk '{print $5}' | cut -f 1 -d '%'";

        char* result = "";
     
        fstream = popen(cmd,"r");
        if(fstream == NULL)
        {
				wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
				return 0;
        }	
		
        while(fgets(buff, sizeof(buff),fstream) != NULL)
        {
                result = buff;
                if(atoi(result) > 95) /* if disk space >95%, disk space is insufficient error */
                {
                		pclose(fstream);
						return 1;
                }
                
        }

        pclose(fstream);
		return 0;
	
}