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
    fputs("Usage: fmadm modinfo\n"
          , stderr);
}

int get_module_info(fmd_adm_t *adm)
{
    if(fmd_adm_mod_iter(adm) != 0)
        die("FMD:failed to load module");
    struct list_head *pos = NULL;
    fmd_adm_modinfo_t  *mp = NULL;
    printf("fmd load module:\n");
    list_for_each(pos,&adm->mod_list){
        mp = list_entry(pos,fmd_adm_modinfo_t,mod_list);
        faf_module_t *fafm =  NULL;
        fafm = &mp->fafm;
        printf("%s\n",fafm->mod_name);
    }
    return 0;
}

/*
 * get specified fault manager module info
 *
 */

int
cmd_modinfo(fmd_adm_t *adm, int argc, char *argv[])
{
    if (argc != 1) {
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

    int rt = get_module_info(adm);

    return rt;
}
