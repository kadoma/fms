/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    load.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-09-25
 * Description: load specified fault manager module
 *
 ************************************************************/

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include "fmadm.h"
#include <list.h>
#include <fmd_adm.h>

static void usage(void)
{
    fputs("Usage: fmadm load <path>\n"
          , stderr);
}

int load_module(fmd_adm_t *adm,char* path)
{
    if(access(path,0))
    {
        printf("%s is not exist!\n",path);
        wr_log("",WR_LOG_ERROR,"%s: is not exist \n",path);
        return -1;
    }
    if(fmd_adm_load_module(adm,path) != 0)
        die("FMD:failed to load module");
    return 0;
}

/*
 * fmadm faulty command
 * load specified fault manager module
 *
 */

int
cmd_load(fmd_adm_t *adm, int argc, char *argv[])
{
    if (argc != 2) {
        usage();
    return (FMADM_EXIT_SUCCESS);
    }
#if 0
    char fmd_thread[8];
    FILE * fp ;
    fp = popen("ps -aux | grep fmd |wc -l","r");
    fgets(fmd_thread,sizeof(fmd_thread),fp);
    if(strncmp(fmd_thread,"3",1)!= 0)
        printf("fmd does not start ,exe command :'fmd start'\n");
    pclose(fp);
#endif

    int rt = load_module(adm,argv[1]);

    return rt;
}
