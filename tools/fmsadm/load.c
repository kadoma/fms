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
#include <fmd_msg.h>

static void usage(void)
{
	fputs("Usage: fmadm load <path>\n"
	      , stderr);
}

int load_module(fmd_adm_t *adm,char* path)
{
	if(access(path,0))
	{
		printf("%s is not exist \n",path);
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

    int rt = load_module(adm,argv[1]);

    return rt;
}
