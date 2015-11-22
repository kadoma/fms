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
 *     check if disk space is full or not
 *
 *  return value
 *   0       if disk space is not full
 *   1       if disk space is full
 */
 
int disk_space_check(char *full_path)
{
    char buff[8] = {0};
    char cmd[64] = {0};
    char num[8] = {0};

    sprintf(cmd, "df -h|grep %s |awk '{print $5}'", full_path);
     
    FILE *fstream = popen(cmd,"r");
    if(fstream == NULL)
    {
        wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
        return 0;
    }	
    fgets(buff, sizeof(buff), fstream);
    strncpy(num, buff, strlen(buff)-2);

    if(atoi(num) > 90){
        pclose(fstream);
        return 1;
    }
    pclose(fstream);
    return 0;
}