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
    char *tmp = malloc(128);
    if(access(path,0))
    {
        printf("%s is not exist!\n",path);
        wr_log("",WR_LOG_ERROR,"%s: is not exist \n",path);
        return -1;
    }else{
        if(strstr(path,"./") != NULL){
            char *tmp1 = strstr(path,"./");
            sprintf(tmp,"/usr/lib/fms/plugins/%s",tmp1+2);
        }else
            sprintf(tmp,"%s",path);
    }

    if(fmd_adm_load_module(adm,tmp) != 0){
        die("FMD:failed to load module");
        free(tmp);
        tmp = NULL;
    }
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
        return(-1 );//(FMADM_EXIT_SUCCESS);
    }
    
    int rt = load_module(adm,argv[1]);

    return rt;
}
