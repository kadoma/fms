/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fm_disksmartdect.c
 * Author:      Inspur OS Team 
                	guomeisi@inspur.com
 * Date:        2015-08-04
 * Description: check if the smart tool is supported.
 *
 ************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "fm_disksmartdect.h"
#include "fm_cmd.h"
#include "logging.h"

/*
 * disk_healthy_check
 *     check   if  disk is healthy 
 *     use "smartctl", make sure  smartctl is enable.
 *
 *  return value
 *   0       if disk is healthy
 *   1       if disk is unhealthy
 * 
 **/
int disk_unhealthy_check(const char *path)
{
    FILE *fstream = NULL;
    char buff[LINE_MAX] = {0};
    
    char cmd[128];
    smarthealthycmd(path, cmd);
    
    fstream = popen(cmd,"r");
    if(fstream == NULL)
    {  
        wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
		return 0;
    }

    while(fgets(buff, sizeof(buff),fstream) != NULL)
    {
        char *p = strstr(buff, "OK");
        char *p_n = strstr(buff, "Unable");
        
        if((p != NULL)||(p_n != NULL))
           return 0;
    }
    
    pclose(fstream);
	return 1;
	
}

/*
 * disk_temperature_check
 *     check   if  disk`s temperature is over-temperature 
 *     use "smartctl", make sure  smartctl is enable.
 *
 *  return value
 *   0       if disk`s temperature is normal
 *   1       if disk`s temperature is over-temperature
 */
 
int disk_temperature_check(const char *path)
{
    char buff[LINE_MAX] = {0};
	char cmd[128] = {0};
	smarttempcmd(path, cmd);
    
	char* result = NULL; 			
    FILE *fstream = popen(cmd,"r");
    if(fstream == NULL)
   	{
		wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
		return 0;
    }	
	
    while(fgets(buff, sizeof(buff),fstream) != NULL)
    {
        result = buff;
        //int a = atoi(result);
        if(atoi(result) > TEMPERATURE_LIMIT ) /* if disk temperature>70C, disk is over-temperature error */
        {
            pclose(fstream);
            return 1;
        }               
    }
    pclose(fstream);
    return 0;	
}
