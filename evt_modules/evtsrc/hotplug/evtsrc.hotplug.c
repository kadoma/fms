 /************************************************************
  *  Copyright (C) inspur Inc. <http://www.inspur.com>
  *  FileName:    evtsrc.hotplug.c
  *  Author:      Inspur OS Team
  *               wang.leibj@inspur.com
  *  Date:        2015-08-21
  *  Description: clear caselist
  *
  *************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <linux/types.h>

#include <wrap.h>
#include <evt_src.h>
#include <fmd_module.h>
#include <fmd_fmadm.h>

#define MODULE_NAME "evtsrc.hotplug"

/**
 *
 * @param
 * @return
 */
static int
fmd_clear_caselist(fmd_t *pfmd, char* dev)
{
    struct list_head *pos = NULL;
    struct list_head *n = NULL;
    fmd_case_t *cp = NULL;
    char tmp[64];

    list_for_each_safe(pos,n, &pfmd->list_repaired_case) {
        cp = list_entry(pos, fmd_case_t, cs_list);
        sprintf(tmp,"%s%ld",cp->dev_name,cp->dev_id);
        if(strcmp(dev,tmp) == 0){
            list_del(&cp->cs_list);
            return 0;
        }
    }
    return -1;
}

struct list_head *
hotplug_probe(evtsrc_module_t *emp)
{
    fmd_t *pfmd = ((fmd_module_t *)emp)->p_fmd;

    mqd_t mqd;
    struct mq_attr attr;
    void *buf = NULL;
    char sendbuf[16];
    int ret = 0,ret1 = -1;
    int len= 0;

    /* mqueue */
    mqd = mq_open("/hotplug", O_RDWR | O_CREAT, 0644, NULL);

    if (mqd == -1) {
        perror("hotplug mq_open");
        syslog(LOG_ERR, "hotplug mq_open");
        return NULL;
    }

    buf = def_calloc(128, 1);
    if(buf == NULL){
        mq_close(mdq);
        return NULL;
    }
        

    /* Determine max. msg size; initial buffer to receive msg */
    if (mq_getattr(mqd, &attr) == -1){
        def_free(buf);
        mq_close(mqd);
        return NULL;
    }

    if ((ret = mq_receive(mqd, buf, attr.mq_msgsize, NULL)) < 0) {
        perror("fmd_adm_module:");
        free(buf);
        mq_close(mqd);
        return NULL;
    }
    ret1 = fmd_clear_caselist(pfmd, buf);
    if(ret1 == 0)
        sprintf(sendbuf,"success");
    else
        sprintf(sendbuf,"fail");

    if ((mq_send(mqd, sendbuf,15 , 0)) < 0) {
        goto ERR;
    }
    
ERR:
    mq_close(mqd);
    def_free(buf);
    
OUT:
    return NULL;
}

static evtsrc_modops_t hotplug_mops = {
    .evt_probe = hotplug_probe,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
    return (fmd_module_t *)evtsrc_init(&hotplug_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
}
