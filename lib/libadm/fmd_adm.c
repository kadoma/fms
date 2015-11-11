/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    libfmd_adm.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-08-21
 * Description: adm get faulty from fms
 *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <fmd_adm.h>
#include "fmd_topo.h"



static const char _url_fallback[] = "wang.leibj@inspur.com";

fmd_adm_t *
fmd_adm_open(void)
{
    fmd_adm_t *ap;
    mqd_t mqd;

    /* mqueue */
//    mqd = mq_open("/fmadm", O_RDWR | O_CREAT | O_NONBLOCK, 0644, NULL);
    mqd = mq_open("/fmadm", O_RDWR | O_CREAT, 0644, NULL);
    if (mqd == -1) {
        perror("fmadm mq_open");
        return NULL;
    }

    if ((ap = malloc(sizeof (fmd_adm_t))) == NULL)
        return (NULL);

    memset(ap, 0, sizeof (fmd_adm_t));
    ap->adm_mqd = mqd;
    ap->adm_svcerr = 0;
    ap->adm_errno = 0;
    INIT_LIST_HEAD(&ap->cs_list);
    INIT_LIST_HEAD(&ap->mod_list);
    return (ap);
}

void
fmd_adm_close(fmd_adm_t *ap)
{
    mqd_t mqd = ap->adm_mqd;
    if (ap == NULL)
        return; /* permit NULL to simply caller code */

    mq_close(mqd);
    free(ap);
}

static const char *
fmd_adm_svc_errmsg(enum fmd_adm_error err)
{
    switch (err) {
    case FMD_ADM_ERR_NOMEM:
        return ("unable to perform request due to allocation failure");
    case FMD_ADM_ERR_PERM:
        return ("operation requires additional privilege");
    case FMD_ADM_ERR_MODSRCH:
        return ("specified module is not loaded in fault manager");
    case FMD_ADM_ERR_MODBUSY:
        return ("module is in use and cannot be unloaded");
    case FMD_ADM_ERR_MODFAIL:
        return ("module failed and can no longer export statistics");
    case FMD_ADM_ERR_MODNOENT:
        return ("file missing or cannot be accessed by fault manager");
    case FMD_ADM_ERR_MODEXIST:
        return ("module using same name is already loaded");
    case FMD_ADM_ERR_MODINIT:
        return ("module failed to initialize (consult fmd(1M) log)");
    case FMD_ADM_ERR_MODLOAD:
        return ("module failed to load (consult fmd(1M) log)");
    case FMD_ADM_ERR_RSRCSRCH:
        return ("specified resource is not cached by fault manager");
    case FMD_ADM_ERR_RSRCNOTF:
        return ("specified resource is not known to be faulty");
    case FMD_ADM_ERR_SERDSRCH:
        return ("specified serd engine not present in module");
    case FMD_ADM_ERR_SERDFIRED:
        return ("specified serd engine has already fired");
    case FMD_ADM_ERR_ROTSRCH:
        return ("invalid log file name");
    case FMD_ADM_ERR_ROTFAIL:
        return ("failed to rotate log file (consult fmd(1M) log)");
    case FMD_ADM_ERR_ROTBUSY:
        return ("log file is too busy to rotate (try again later)");
    case FMD_ADM_ERR_CASESRCH:
        return ("specified UUID is invalid or has been repaired");
    case FMD_ADM_ERR_CASEOPEN:
        return ("specified UUID is still being diagnosed");
    case FMD_ADM_ERR_XPRTSRCH:
        return ("specified transport ID is invalid or has been closed");
    case FMD_ADM_ERR_CASEXPRT:
        return ("specified UUID is owned by a different fault manager");
    case FMD_ADM_ERR_RSRCNOTR:
        return ("specified resource has not been replaced");
    default:
        return ("unknown fault manager error");
    }
}

const char *
fmd_adm_errmsg(fmd_adm_t *ap)
{
    if (ap == NULL) {
        switch (errno) {
        case ENOTSUP:
            return ("client requires newer libfmd_adm version");
        case EPROTO:
            return ("failed to connect to fmd");
        }
    }

    switch (ap ? ap->adm_errno : errno) {
    case EPROTO:
        return ("message queue request failed");
    case EREMOTE:
        return (fmd_adm_svc_errmsg(ap->adm_svcerr));
    default:
        return (strerror(ap->adm_errno));
    }
}
#if 0
static int
fmd_adm_set_svcerr(fmd_adm_t *ap, enum fmd_adm_error err)
{
    if (err != 0) {
        ap->adm_svcerr = err;
        ap->adm_errno = EREMOTE;
        return (-1);
    } else {
        ap->adm_svcerr = err;
        ap->adm_errno = 0;
        return (0);
    }
}
#endif
static int
fmd_adm_set_errno(fmd_adm_t *ap, int err)
{
    ap->adm_errno = err;
    errno = err;
    return (-1);
}

#if 0
static int
fmd_adm_case_cmp(const void *lp, const void *rp)
{
    return (strcmp(*(char **)lp, *(char **)rp));
}
#endif

static int
fmd_adm_get_list(fmd_adm_t *ap,char* cmd)
{
    mqd_t mqd = ap->adm_mqd;
    struct mq_attr attr;
    void *recvbuf = NULL;
    int len = strlen(cmd);
    //char cmd[] = "GET CASELIST";

    int ret = 0;
    ret = mq_send(mqd, (void *)cmd, len * sizeof (char), 0);
//    sleep(1);
    if (ret < 0) {
        perror("Clnt message posted");
        perror("fmadm faulty:");
        return (-1);
    }
    /* Determine max. msg size; allocate buffer to receive msg */
    if (mq_getattr(mqd, &attr) == -1) {
        perror("fmadm mq_getattr");
        return (-1);
    }

    recvbuf = malloc(attr.mq_msgsize);
    if (recvbuf == NULL) {
        perror("fmadm malloc");
        return (-1);
    }

    memset(recvbuf, 0, attr.mq_msgsize);

    ret = mq_receive(mqd, recvbuf, attr.mq_msgsize, NULL);
//    sleep(1);
    if (ret < 0) {
        perror("fmadm msgrcv");
        return (-1);
    }
    memcpy(ap->adm_buf, recvbuf, attr.mq_msgsize);
//    printf("FMD ADM: Clnt received the message about caselist.\n");
//    printf("FMD ADM: Clnt received %d message.\n", attr.mq_msgsize);

    return 0;
}


static fmd_adm_caseinfo_t *
fmd_adm_alloc_aci(void)
{
    fmd_adm_caseinfo_t *aci =
        (fmd_adm_caseinfo_t *)malloc(sizeof (fmd_adm_caseinfo_t));

    assert(aci != NULL);
    memset(aci, 0, sizeof (fmd_adm_caseinfo_t));

    INIT_LIST_HEAD(&aci->aci_list);

    return aci;
}

static fmd_adm_modinfo_t *
fmd_adm_alloc_mod(void)
{
        fmd_adm_modinfo_t *mod =
                (fmd_adm_modinfo_t *)malloc(sizeof (fmd_adm_modinfo_t));

        assert(mod != NULL);
        memset(mod, 0, sizeof (fmd_adm_modinfo_t));

        INIT_LIST_HEAD(&mod->mod_list);

        return mod;
}

static int
fmd_adm_get_caseinfo(fmd_adm_t *ap)
{
    faf_hdr_t *fafh = NULL;
    faf_case_t *fafc = NULL;
    int secnum = 0;
    int i = 0;

    fafh = (void *)ap->adm_buf;
    fafc = (void *)(ap->adm_buf + fafh->fafh_hdrsize);
    secnum = fafh->fafh_secnum;

    if (secnum == 0)
        return (-1);    /* receive nothing */
    for (i = 1; i < secnum + 1; i++) {
        fmd_adm_caseinfo_t *aci = fmd_adm_alloc_aci();
        memcpy(&aci->fafc, fafc, sizeof (faf_case_t));

        /* create url */
//        fmd_adm_case_url(aci);
        sprintf(aci->aci_fru,"%s",fafc->fafc_fault);
        list_add(&aci->aci_list, &ap->cs_list);

 //       fafc = (void *)(tmp + fafh->fafh_hdrsize + i * sizeof (faf_case_t));
        fafc = (void *)(ap->adm_buf + fafh->fafh_hdrsize + i * sizeof (faf_case_t));
    }

    return secnum;
}

static int
fmd_adm_get_modinfo(fmd_adm_t *ap)
{
    faf_hdr_t *fafh = NULL;
    faf_module_t *fafm = NULL;
    int secnum = 0;
    int i = 0;

    fafh = (void *)ap->adm_buf;
    fafm = (void *)(ap->adm_buf + fafh->fafh_hdrsize);
    secnum = fafh->fafh_secnum;

    if (secnum == 0)
        return (-1);    /* receive nothing */

    for (i = 1; i < secnum + 1; i++) {
        fmd_adm_modinfo_t *mod = fmd_adm_alloc_mod();
        memcpy(&mod->fafm, fafm, sizeof (faf_module_t));
        /* create url */
        list_add(&mod->mod_list, &ap->mod_list);

        fafm = (void *)(ap->adm_buf + fafh->fafh_hdrsize + i * sizeof (faf_module_t));
    }

    return (0);
}


/*
 * Our approach to cases is the same as for resources: we first obtain a
 * list of UUIDs, sort them, then obtain the case information for each.
 */
int
fmd_adm_case_iter(fmd_adm_t *ap)
{
    int ret = 0;
    int num = 20;
    int index = 0;
    char cmd[32];

    while ((num > 0 && index == 0) || num == 20){

        sprintf(cmd,"GET CASELIST:%d",FAF_NUM*index);
        index++;

        ret = fmd_adm_get_list(ap,cmd);
        if (ret < 0)
            return (fmd_adm_set_errno(ap, EPROTO));

        num = fmd_adm_get_caseinfo(ap);

        sleep(1);

        if (num < 0)
            return (-1);
    }

    return (0);
}

/*
 *
 *
 */
int
fmd_adm_mod_iter(fmd_adm_t *ap)
{
    int ret = 0;
    char cmd[] = "GET MODLIST";

    ret = fmd_adm_get_list(ap,cmd);

    if (ret != 0)
        return (fmd_adm_set_errno(ap, EPROTO));

    if (fmd_adm_get_modinfo(ap) != 0)
        return (-1);

    return (0);
}

int
fmd_adm_load_module(fmd_adm_t *ap ,char *path){
    char cmd[128] = "LOAD:";
    strcpy(cmd+5,path);
    if((fmd_adm_get_list(ap,cmd)) != 0)
        return (fmd_adm_set_errno(ap, EPROTO));

    if(strstr((char*)ap->adm_buf,"LOAD:") != NULL){
        return (-1);
    }

    printf("%s\n",(char*)ap->adm_buf);

    if(strstr(ap->adm_buf,"failed") != 0){
        printf("please view /var/log/fms/fms.log for load failed reason\n");
        return (-1);
    }
    return 0;
}

int
fmd_adm_unload_module(fmd_adm_t *ap ,char *path){
    char cmd[128]= "UNLOAD:";
    strcpy(cmd+7,path);

    if((fmd_adm_get_list(ap,cmd)) != 0)
        return (fmd_adm_set_errno(ap, EPROTO));

    if(strstr((char*)ap->adm_buf,"UNLOAD:") != NULL){
        return (-1);
    }

    printf("%s\n",(char*)ap->adm_buf);

    if(strstr(ap->adm_buf,"failed")!= 0){
        printf("please view /var/log/fms/fms.log for unload failed  reason\n");
        return (-1);
    }
    return 0;
}
