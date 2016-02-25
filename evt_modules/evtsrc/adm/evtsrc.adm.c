
/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    evtsrc.adm.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-08-21
 * Description: send to adm faulty info
 *
 ************************************************************/


#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <linux/types.h>
#include <dlfcn.h>

#include "wrap.h"
#include "fmd_fmadm.h"
#include "fmd_module.h"
#include "evt_agent.h"
#include "evt_src.h"

#define MODULE_NAME "evtsrc.adm"


typedef struct fmd_acl {
    uint8_t *acl_buf;       /* message buffer base address */
    faf_hdr_t *acl_hdr;     /* message header pointer */
    uint64_t acl_size;      /* message buffer size */
    uint8_t acl_secs;       /* number of sections */
} fmd_acl_t;

/**
 * fmd_resv_caselist
 *
 * @param
 * @return
 */
static void
fmd_resv_caselist(fmd_t *pfmd, fmd_acl_t *acl,int num,int index)
{
    struct list_head *pos = NULL, *epos = NULL, *listn = NULL;
    fmd_case_t *cp = NULL;
    int case_num = 0;
    int tmp = 0;
    fmd_event_t *p_event = NULL;

    if(index == 1){
        list_for_each(pos, &pfmd->list_case) {

            cp = list_entry(pos, fmd_case_t, cs_list);

            list_for_each(epos,&cp->cs_event){

                p_event = list_entry(epos,fmd_event_t,ev_list);
                ++tmp;

                if((tmp > num) && (tmp <= num+ FAF_NUM)){
                    case_num++;
                }
            }
        }
    }else{
        fmd_queue_t *p_queue = &pfmd->fmd_queue;
        list_repaired *plist_repaired = &p_queue->queue_repaired_list;
        struct list_head *prepaired_head = &plist_repaired->list_repaired_head;
        pthread_mutex_lock(&plist_repaired->evt_repaired_lock);

        list_for_each_safe(pos, listn, prepaired_head){

            p_event = list_entry(epos,fmd_event_t,ev_list);
            ++tmp;

            if((tmp > num) && (tmp <= num+ FAF_NUM)){
                case_num++;
            }
        }
        pthread_mutex_unlock(&plist_repaired->evt_repaired_lock);
    }
    wr_log("",WR_LOG_WARNING,"FMD:GETCASELIST happenning  caselist num = %d get num = %d\n",tmp,case_num);
    acl->acl_secs = case_num;               /* number of sections */
    acl->acl_size = case_num * sizeof(faf_case_t) + sizeof(faf_hdr_t);
}

/**
 * fmd_resv_modlist
 *
 * @param
 * @return
 */
static void
fmd_resv_modlist(fmd_t *pfmd, fmd_acl_t *acl)
{
    struct list_head *pos = NULL;
    fmd_module_t *mp = NULL;
    int mod_num = 0;

    list_for_each(pos, &pfmd->fmd_module) {
        mp = list_entry(pos, fmd_module_t, list_fmd);
        int vers = mp->mod_vers;
        mp->mod_vers = vers;
        mod_num++;
    }

    acl->acl_secs = mod_num;               /* number of sections */
    acl->acl_size = mod_num * sizeof(faf_module_t) + sizeof(faf_hdr_t);
}



/**
 * fmd_get_caselist
 *
 * @param
 * @return
 */
static void *
fmd_get_caselist(fmd_t *pfmd, int *size,int num, int index)
{
    struct list_head *pos = NULL, *epos = NULL, *listn = NULL;
    faf_case_t *fafc = NULL;
    fmd_case_t *cp = NULL;
    fmd_event_t *ep = NULL;
    int cnt = 0;
    int tmp_num = 0;

    fmd_acl_t *acl = malloc(sizeof (fmd_acl_t));
    assert(acl != NULL);
    memset(acl, 0, sizeof(fmd_acl_t));

    fmd_resv_caselist(pfmd, acl, num,index);

    /*
     * Allocate memory for FAF.
     */
    acl->acl_buf = malloc(acl->acl_size);
    assert(acl->acl_buf != NULL);
    memset(acl->acl_buf, 0, acl->acl_size);

    if (acl->acl_buf == NULL)
        return NULL; /* errno is set for us */

    acl->acl_hdr = (void *)acl->acl_buf;
    acl->acl_hdr->fafh_hdrsize = sizeof (faf_hdr_t);
    acl->acl_hdr->fafh_secnum = acl->acl_secs;
    acl->acl_hdr->fafh_msgsz = acl->acl_size;
    snprintf(acl->acl_hdr->fafh_cmd, 32, FAF_GET_CASELIST);

    fafc = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize);

    if( index == 1){
        list_for_each(pos, &pfmd->list_case) {
            cp = list_entry(pos, fmd_case_t, cs_list);
            list_for_each(epos,&cp->cs_event){
                ep = list_entry(epos,fmd_event_t,ev_list);
                ++tmp_num;
                if((tmp_num > num) && (tmp_num <= num + FAF_NUM)){
                    fafc->fafc_count = ep->ev_count;
                    fafc->fafc_create = (uint64_t)ep->ev_create;
                    fafc->fafc_fire = (uint64_t)ep->ev_last_occur;
                   // fafc->fafc_close = (uint64_t)ep->ev_close;
                    if(strstr(ep->ev_class,"disk") != NULL)
                    {
                        sprintf(fafc->fafc_fault,"%s:%s",ep->dev_name,ep->ev_class);
                    }else{
                        sprintf(fafc->fafc_fault,"%s%ld:%s",ep->dev_name,ep->dev_id,ep->ev_class);
                    }
                    if (cp->cs_flag == CASE_CREATE)
                        fafc->fafc_state = FAF_CASE_CREATE;
                    else if (cp->cs_flag == CASE_FIRED)
                        fafc->fafc_state = FAF_CASE_FIRED;
                    else if (cp->cs_flag == CASE_CLOSED)
                        fafc->fafc_state = FAF_CASE_CLOSED;
                    else {
                        wr_log("",WR_LOG_ERROR,"FMD:case %phas invalid state%u\n",cp, cp->cs_flag);
                        return NULL;
                    }
                    cnt++;
                    fafc = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize + cnt * sizeof (faf_case_t));
                }
            }
        }
    }
    else{
        fmd_queue_t *p_queue = &pfmd->fmd_queue;
        list_repaired *plist_repaired = &p_queue->queue_repaired_list;
        struct list_head *prepaired_head = &plist_repaired->list_repaired_head;
        pthread_mutex_lock(&plist_repaired->evt_repaired_lock);

        list_for_each_safe(pos, listn, prepaired_head)
        {
            ep = list_entry(pos, fmd_event_t, ev_repaired_list);
            ++tmp_num;
            if((tmp_num > num) && (tmp_num <= num + FAF_NUM)){
                fafc->fafc_count = ep->ev_count;
                fafc->fafc_create = (uint64_t)ep->ev_create;
                fafc->fafc_fire = (uint64_t)ep->ev_last_occur;
                //fafc->fafc_fire = (uint64_t)cp->cs_last_fire;
                //fafc->fafc_close = (uint64_t)ep->cs_close;
                    if(strstr(ep->ev_class,"disk") != NULL)
                    {
                        sprintf(fafc->fafc_fault,"%s:%s",ep->dev_name,ep->ev_class);
                    }else{
                        sprintf(fafc->fafc_fault,"%s%ld:%s",ep->dev_name,ep->dev_id,ep->ev_class);
                    }

                /*if (ep->ev_flag == CASE_CREATE)
                    fafc->fafc_state = FAF_CASE_CREATE;
                else if (ep->ev_flag == CASE_FIRED)
                    fafc->fafc_state = FAF_CASE_FIRED;
                else if (ep->ev_flag == CASE_CLOSED)
                    fafc->fafc_state = FAF_CASE_CLOSED;
                else {
                    wr_log("",WR_LOG_DEBUG,"FMD:case %phas invalid state%u\n",ep, ep->ev_flag);
                    return NULL;
                }*/
                cnt++;
                fafc = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize + cnt * sizeof (faf_case_t));
            }
        }
        pthread_mutex_unlock(&plist_repaired->evt_repaired_lock);
    }
    *size = acl->acl_size;
    return (void *)acl->acl_hdr;
}


    static void *
fmd_get_modlist(fmd_t *pfmd, int *size)
{
    struct list_head *pos = NULL;
    faf_module_t *fafm = NULL;
    fmd_module_t *mp = NULL;
    int mnt = 0;

    fmd_acl_t *acl = malloc(sizeof (fmd_acl_t));
    assert(acl != NULL);
    memset(acl, 0, sizeof(fmd_acl_t));

    fmd_resv_modlist(pfmd, acl);

    /*
     * Allocate memory for FAF.
     */
    acl->acl_buf = malloc(acl->acl_size);
    assert(acl->acl_buf != NULL);
    memset(acl->acl_buf, 0, acl->acl_size);

    if (acl->acl_buf == NULL)
        return NULL; /* errno is set for us */

    acl->acl_hdr = (void *)acl->acl_buf;
    acl->acl_hdr->fafh_hdrsize = sizeof (faf_hdr_t);
    acl->acl_hdr->fafh_secnum = acl->acl_secs;
    acl->acl_hdr->fafh_msgsz = acl->acl_size;
    snprintf(acl->acl_hdr->fafh_cmd, 32, FAF_GET_MODLIST);

    fafm = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize);

    list_for_each(pos, &pfmd->fmd_module) {
        mp = list_entry(pos, fmd_module_t, list_fmd);

        fafm->mod_vers = mp->mod_vers;
        fafm->mod_interval = mp->mod_interval;

   // snprintf(fafm->mod_name,128,"%s",mp->mod_name);
    snprintf(fafm->mod_name,128,"%s",mp->mod_name);

    mnt++;
    fafm = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize + mnt * sizeof (faf_module_t));
    }
    *size = acl->acl_size;
    return (void *)acl->acl_hdr;
}

int check_mod_load(fmd_t *fmd, char *path)
{
    if(path == NULL)
        return (-1);
    struct list_head *pos = NULL;
    fmd_module_t *mp = NULL;

    list_for_each(pos,&fmd->fmd_module){
        mp = list_entry(pos, fmd_module_t, list_fmd);
        if(strcmp(mp->mod_name, path)== 0)
        return (-1);
    }
    return 0;
}

void free_module( fmd_module_t * module)
{

    if(strstr(module->mod_name,"agent") != NULL)
    {
        agent_module_t * agent_module = (agent_module_t *)module;
        struct list_head *pos ,*n;
        struct subitem *p = NULL;

        list_for_each_safe(pos,n,&(agent_module->module.list_eclass)){
            p = list_entry(pos,struct subitem,si_list);
            def_free(p->si_eclass);
            list_del(&p->si_list);
        }

        list_del(&(agent_module->module.list_queue));
        list_del(&(agent_module->module.list_fmd));

        def_free(agent_module->ring);
        def_free(agent_module->module.mod_name);
        def_free(agent_module);

    }else{
        evtsrc_module_t * src_module = (evtsrc_module_t *)module;
        list_del(&(src_module->module.list_fmd));
        list_del(&(src_module->timer.timer_list));
        def_free(src_module->module.mod_name);
        def_free(src_module);
    }
}

int evt_load_module(fmd_t *fmd, char *path)
{
    if((check_mod_load(fmd,path))== -1){
        wr_log("",WR_LOG_WARNING,"module :%s has loaded",path);
        return (-1);
    }

    if((fmd_init_module(fmd,path)) == -1){
        wr_log("",WR_LOG_ERROR,"%s can't load failed",path);
        return (-1);
    }
    struct list_head *pos = NULL;
    fmd_module_t *mp = NULL;
    list_for_each(pos, &fmd->fmd_module){
        mp = list_entry(pos, fmd_module_t, list_fmd);
        if(strstr(mp->mod_name,".so")== NULL)
            mp->mod_name = "/usr/lib/fms/plugins/adm_src.so";
    }
    return 0;
}

int module_update_conf(fmd_t *fmd, char *module,char* mode)
{
    int ret = agent_module_update_conf(fmd,module,
	NULL, AGENT_KEY_EVT_HANDLE_MODE,mode);
    return ret;
}


int evt_unload_module(fmd_t *fmd, char* module)
{
    int ret = -1;
    char *tmp = NULL;
    if(module == NULL){
        wr_log("",WR_LOG_WARNING,"module is null");
        return (-1);
    }
    if(strrchr(module,'/'))
        tmp = strrchr(module,'/') + 1;
    else
        tmp = module;

    struct list_head *pos,*n;
    fmd_module_t *mp = NULL;

    list_for_each_safe(pos,n,&fmd->fmd_module){
        mp = list_entry(pos,fmd_module_t,list_fmd);
        char *tmpmodule = strrchr(mp->mod_name,'/') + 1;
        int len = strlen(tmpmodule);
        if(strncmp(tmpmodule,tmp,len) == 0){
            ret = 0;
            free_module(mp);
           // list_del(&mp->list_fmd);
        }
    }
    if(ret == -1){
         wr_log("",WR_LOG_WARNING,"module :%s has not loaded",module);
    }
    return ret;
}

struct list_head *
adm_probe(evtsrc_module_t *emp)
{
    //fmd_t *pfmd = ((fmd_module_t *)emp)->p_fmd;
    fmd_t *pfmd = &fmd;
    mqd_t mqd;
    struct mq_attr attr;
    void *buf = NULL;
    void *sendbuf = NULL;
    int size = 0;
    int ret = 0;

    /* mqueue */
//    mqd = mq_open("/fmadm", O_RDWR | O_CREAT | O_NONBLOCK, 0644, NULL);
    mqd = mq_open("/fmadm", O_RDWR | O_CREAT, 0644, NULL);

    if (mqd == -1) {
        perror("fmadm mq_open");
        wr_log("",WR_LOG_ERROR,"fmsadm mq_open failed");
        return NULL;
    }

    buf = malloc(128 * 1024);
    if (buf == NULL) {
        perror("fmadm malloc");
        wr_log("",WR_LOG_ERROR,"fmsadm malloc failed");
        mq_close(mqd);
        return NULL;
    }

    /* Determine max. msg size; initial buffer to receive msg */
    if (mq_getattr(mqd, &attr) == -1) {
        perror("fmadm mq_getattr");
        wr_log("",WR_LOG_ERROR,"fmsadm mq_getattr failed");
        free(buf);
        mq_close(mqd);
        return NULL;
    }

    memset(buf, 0, 128 * 1024);

    if ((ret = mq_receive(mqd, buf, attr.mq_msgsize, NULL)) < 0) {
        perror("fmd_adm_module:");
        free(buf);
        mq_close(mqd);
        return NULL;
    }

    if(strncmp(buf,"GET CASELIST",12) == 0){
        char *tmp,*tmp1;
        int num = 0 ,index = 0;
        tmp = strtok((char*)buf,":");
        if(tmp)
            tmp = strtok(NULL,":");
        if(tmp)
            tmp1 = strtok(NULL,":");
        num = atoi(tmp);
        index = atoi(tmp1);
        sendbuf = fmd_get_caselist(pfmd, &size,num,index);
    }else if(strncmp(buf,"LOAD",4) == 0){
        char  *path = NULL;
        path = strstr((char*)buf,":");
        if((evt_load_module(pfmd,path+1)) == 0){
            void *ret = NULL;
            ret = malloc(32);
            sprintf(ret, "load successed ");
            size = strlen((char*)ret);
            sendbuf = (void*)ret;
        }else{
            void *ret = NULL;
            ret = malloc(32);
            sprintf(ret, "load failed ");
            size = strlen((char*)ret);
            sendbuf = (void*)ret;
        }
    }else if(strncmp(buf,"UPDATE",6) == 0){
        char *module,*mode,*tmp;
        tmp = strtok((char*)buf,":");
        if(tmp)
            module = strtok(NULL,":");
        if(module)
            mode = strtok(NULL,":");
        if((module_update_conf(pfmd,module+1,mode+1)) == 0 ){
            void *ret = NULL;
            ret = malloc(32);
            sprintf(ret, "undate successed ");
            size = strlen((char*)ret);
            sendbuf = (void*)ret;
        }else{
            void *ret = NULL;
            ret = malloc(32);
            sprintf(ret, "update failed ");
            size = strlen((char*)ret);
            sendbuf = (void*)ret;
        }
    }else if(strncmp(buf,"UNLOAD",6) == 0){
        char *module = NULL;
        module = strstr((char*)buf,":");
        if((evt_unload_module(pfmd,module+1)) == 0 ){
            void *ret = NULL;
            ret = malloc(32);
            sprintf(ret, "unload successed ");
            size = strlen((char*)ret);
            sendbuf = (void*)ret;
        }else{
            void *ret = NULL;
            ret = malloc(32);
            sprintf(ret, "unload failed ");
            size = strlen((char*)ret);
            sendbuf = (void*)ret;
        }
    }else if(strncmp(buf,"GET MODLIST",11) == 0){
        sendbuf = fmd_get_modlist(pfmd, &size);
    }else{
        void *ret = NULL;
        ret = malloc(128);
        sprintf(ret, "%s is illegal cmd ",(char*)buf);
        size = strlen((char*)ret);
        sendbuf = (void*)ret;
    }
//    printf("FMD: send the caselist message to fmd through message queue.\n");

    if ((mq_send(mqd, sendbuf, size, 0)) < 0) {
        perror("fmadm message posted");
        syslog(LOG_ERR, "fmadm message posted");
        free(buf);
        mq_close(mqd);
        return NULL;
    }

    free(sendbuf);
    mq_close(mqd);
    free(buf);

    return NULL;
}

static evtsrc_modops_t adm_mops = {
    .evt_probe = adm_probe,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
    return (fmd_module_t *)evtsrc_init(&adm_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
    return;
}
