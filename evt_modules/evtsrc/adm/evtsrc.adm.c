
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
#include "evt_src.h"
#include "fmd_case.h"
#include "fmd_fmadm.h"
#include "fmd_module.h"

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
fmd_resv_caselist(fmd_t *pfmd, fmd_acl_t *acl)
{
	struct list_head *pos = NULL;
	fmd_case_t *cp = NULL;
	int case_num = 0;

	list_for_each(pos, &pfmd->list_case) {
		cp = list_entry(pos, fmd_case_t, cs_list);
		uint64_t cs_uuid = cp->cs_uuid;
		cp->cs_uuid = cs_uuid;
		case_num++;
	}

printf("hapen = %d \n",case_num);
	acl->acl_secs = case_num;               /* number of sections */
	acl->acl_size = case_num * sizeof(faf_case_t) + sizeof(faf_hdr_t);
}

static void
fmd_resv_recaselist(fmd_t *pfmd, fmd_acl_t *acl)
{
        struct list_head *pos = NULL;
        fmd_case_t *cp = NULL;
        int case_num = 0;

        list_for_each(pos, &pfmd->list_repaired_case) {
                cp = list_entry(pos, fmd_case_t, cs_list);
                uint64_t cs_uuid = cp->cs_uuid;
                cp->cs_uuid = cs_uuid;
                case_num++;
        }
printf("repaired  = %d \n",case_num);
        acl->acl_secs = case_num;               /* number of sections */
        acl->acl_size = case_num * sizeof(faf_case_t) + sizeof(faf_hdr_t);
}


#if 1
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

#endif


/**
 * fmd_get_caselist
 *
 * @param
 * @return
 */
static void *
fmd_get_caselist(fmd_t *pfmd, int *size)
{
	struct list_head *pos = NULL;
	struct fmd_hash *phash = &pfmd->fmd_esc.hash_clsname;
	faf_case_t *fafc = NULL;
	fmd_case_t *cp = NULL;
	int cnt = 0;

	fmd_acl_t *acl = malloc(sizeof (fmd_acl_t));
	assert(acl != NULL);
	memset(acl, 0, sizeof(fmd_acl_t));

	fmd_resv_caselist(pfmd, acl);

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

	list_for_each(pos, &pfmd->list_case) {
		cp = list_entry(pos, fmd_case_t, cs_list);

		fafc->fafc_uuid = cp->cs_uuid;
		fafc->fafc_rscid = cp->cs_rscid;
		fafc->fafc_count = cp->cs_count;
		fafc->fafc_create = (uint64_t)cp->cs_create;
		fafc->fafc_fire = (uint64_t)cp->cs_last_fire;
		fafc->fafc_close = (uint64_t)cp->cs_close;
		sprintf(fafc->fafc_fault,"%s:%s",cp->dev_name,cp->last_eclass);
	//	snprintf(fafc->fafc_fault, 128, "%s", hash_get_key(phash, cp->cs_type->fault));

		if (cp->cs_flag == CASE_CREATE)
			fafc->fafc_state = FAF_CASE_CREATE;
		else if (cp->cs_flag == CASE_FIRED)
			fafc->fafc_state = FAF_CASE_FIRED;
		else if (cp->cs_flag == CASE_CLOSED)
			fafc->fafc_state = FAF_CASE_CLOSED;
		else {
			printf("FMD: case %p (%ld) has invalid state %u\n",
				(void *)cp, cp->cs_uuid, cp->cs_flag);
			return NULL;
		}
		cnt++;
		fafc = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize + cnt * sizeof (faf_case_t));
	}
	*size = acl->acl_size;
	return (void *)acl->acl_hdr;
}

static void *
fmd_get_recaselist(fmd_t *pfmd, int *size)
{
        struct list_head *pos = NULL;
        faf_case_t *fafc = NULL;
        fmd_case_t *cp = NULL;
        int cnt = 0;

        fmd_acl_t *acl = malloc(sizeof (fmd_acl_t));
        assert(acl != NULL);
        memset(acl, 0, sizeof(fmd_acl_t));

        fmd_resv_recaselist(pfmd, acl);

        /*
 *          * Allocate memory for FAF.
 *                   */
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

        list_for_each(pos, &pfmd->list_repaired_case) {
                cp = list_entry(pos, fmd_case_t, cs_list);

                fafc->fafc_uuid = cp->cs_uuid;
                fafc->fafc_rscid = cp->cs_rscid;
                fafc->fafc_count = cp->cs_count;
                fafc->fafc_create = (uint64_t)cp->cs_create;
                fafc->fafc_fire = (uint64_t)cp->cs_last_fire;
                fafc->fafc_close = (uint64_t)cp->cs_close;

		sprintf(fafc->fafc_fault,"%s:%s",cp->dev_name,cp->last_eclass);
                if (cp->cs_flag == CASE_CREATE)
                        fafc->fafc_state = FAF_CASE_CREATE;
                else if (cp->cs_flag == CASE_FIRED)
                        fafc->fafc_state = FAF_CASE_FIRED;
                else if (cp->cs_flag == CASE_CLOSED)
                        fafc->fafc_state = FAF_CASE_CLOSED;
                else {
                        printf("FMD: case %p (%ld) has invalid state %u\n",
                                (void *)cp, cp->cs_uuid, cp->cs_flag);
                        return NULL;
                }
                cnt++;
                fafc = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize + cnt * sizeof (faf_case_t));
        }
        *size = acl->acl_size;
        return (void *)acl->acl_hdr;
}



#if 1
static void *
fmd_get_modlist(fmd_t *pfmd, int *size)
{
	struct list_head *pos = NULL;
	struct fmd_hash *phash = &pfmd->fmd_esc.hash_clsname;
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
        
	snprintf(fafm->mod_path,128,"%s",mp->mod_path);
	
	mnt++;
	fafm = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize + mnt * sizeof (faf_module_t));
	}
	*size = acl->acl_size;
	return (void *)acl->acl_hdr;
}

#endif

struct list_head *
adm_probe(evtsrc_module_t *emp)
{
	fmd_debug;
	//fmd_t *pfmd = ((fmd_module_t *)emp)->p_fmd;
	
	
	fmd_t *pfmd = &fmd;
	mqd_t mqd;
	struct mq_attr attr;
	void *buf = NULL;
	void *sendbuf = NULL;
	int size = 0;
	int ret = 0;

	/* mqueue */
//	mqd = mq_open("/fmadm", O_RDWR | O_CREAT | O_NONBLOCK, 0644, NULL);
	mqd = mq_open("/fmadm", O_RDWR | O_CREAT, 0644, NULL);

	if (mqd == -1) {
		perror("fmadm mq_open");
		syslog(LOG_ERR, "fmadm mq_open");
		return NULL;
	}

	buf = malloc(128 * 1024);
	if (buf == NULL) {
		perror("fmadm malloc");
		syslog(LOG_ERR, "fmadm malloc");
		mq_close(mqd);
		return NULL;
	}

	/* Determine max. msg size; initial buffer to receive msg */
	if (mq_getattr(mqd, &attr) == -1) {
		perror("fmadm mq_getattr");
		syslog(LOG_ERR, "fmadm mq_getattr");
		free(buf);
		mq_close(mqd);
		return NULL;
	}

	memset(buf, 0, 128 * 1024);

//printf("##DEBUG##: fmd_adm_module: before mq_receive()\n");
	if ((ret = mq_receive(mqd, buf, attr.mq_msgsize, NULL)) < 0) {
		perror("fmd_adm_module:");
		free(buf);
		mq_close(mqd);
		return NULL;
	}
//FIXME:if (strncmp(buf, FAF_GET_CASELIST, 12))
#if 1
	char getcase[] = "GET CASELIST";
	char getmod[] = "GET MODLIST";
    if(strncmp(buf,getcase,12) == 0){
	char *path = NULL;
	path = strstr((char*)buf,":");
	if(strncmp(path+1,"happening",9) == 0)
	     sendbuf = fmd_get_caselist(pfmd,&size);
	else
             sendbuf = fmd_get_recaselist(pfmd, &size);
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
    }else if(strcmp(buf,getmod) == 0){
         sendbuf = fmd_get_modlist(pfmd, &size);
    }else{
                void *ret = NULL;
                ret = malloc(128);
                sprintf(ret, "%s is illegal cmd ",(char*)buf);
		size = strlen((char*)ret);
                sendbuf = (void*)ret;
    }
#endif
//	printf("FMD: send the caselist message to fmd through message queue.\n");

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

int check_mod_load(fmd_t *fmd, char *path)
{
	if(path == NULL)
		return (-1);
	struct list_head *pos = NULL;
	fmd_module_t *mp = NULL;
	
	list_for_each(pos,&fmd->fmd_module){
		mp = list_entry(pos, fmd_module_t, list_fmd);
		if(strcmp(mp->mod_path, path)== 0)
		return (-1);
	}
	return 0;
}

int evt_load_module(fmd_t *fmd, char *path)
{
	if((check_mod_load(fmd,path))== -1){
		printf("module : %s has loaded \n",path);
		return (-1);
	}
	
	if((fmd_init_module(fmd,path)) == -1){
		printf("module : %s load failed \n",path);
		return -1;
	}else{
		struct list_head *pos = NULL;
		fmd_module_t *mp = NULL;
		list_for_each(pos, &fmd->fmd_module){
			mp = list_entry(pos, fmd_module_t, list_fmd);
			if((mp->mod_path)== NULL)
				mp->mod_path = "/usr/lib/fms/plugins/adm_src.so";
		}
	}
	return 0;	
}

int evt_unload_module(fmd_t *fmd, char* module)
{
	if(module == NULL){
		printf("module is null\n");
		return (-1);
	}
	struct list_head *pos,*n;
	fmd_module_t *mp = NULL;

	list_for_each_safe(pos,n,&fmd->fmd_module){
		mp = list_entry(pos,fmd_module_t,list_fmd);
		if(strstr(mp->mod_path,module)!= NULL)
			list_del(&mp->list_fmd);
	}
	return 0;
}

static evtsrc_modops_t adm_mops = {
	.evt_probe = adm_probe,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)evtsrc_init(&adm_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	return;
}
