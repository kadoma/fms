/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    unload.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-09-30
 * Description: unload specified fault manager module
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
#include <fmd_msg.h>

static void usage(void)
{
    fputs("Usage: fmadm unload <module>\n"
          , stderr);
}

int unload_module(fmd_adm_t *adm,char* path)
{

    if((strstr(path,".so")) == NULL){
        wr_log("",WR_LOG_ERROR,"%s:illegal input module end with .so\n",path);
        return (-1);
    }

    if((strstr(path,"adm_src.so")) != NULL){
        wr_log("",WR_LOG_ERROR,"%s: adm module can not unload -- %s\n");
                return (-1);
    }

    if(fmd_adm_unload_module(adm,path) != 0)
        die("FMD:failed to load module");
    return 0;
}

/*
 * fmadm faulty command
 * load specified fault manager module
 *
 */

int
cmd_unload(fmd_adm_t *adm, int argc, char *argv[])
{
    if (argc != 2) {
        usage();
    return (FMADM_EXIT_USAGE);
    }

    int rt = unload_module(adm,argv[1]);

    return rt;
}
