/*
 * evtsrc.adm.c
 *
 *  Created on: Oct 18, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	evtsrc for fmadm CMD
 */

#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <linux/types.h>
#include <fmd.h>
#include <nvpair.h>
#include <fmd_api.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <fmd_fmadm.h>

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
		case_num++;
	}

	acl->acl_secs = case_num;               /* number of sections */
	acl->acl_size = case_num * sizeof(faf_case_t) + sizeof(faf_hdr_t);
}


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
		fafc->fafc_fire = (uint64_t)cp->cs_fire;
		fafc->fafc_close = (uint64_t)cp->cs_close;
		snprintf(fafc->fafc_fault, 128, "%s", hash_get_key(phash, cp->cs_type->fault));

		if (cp->cs_flag == CASE_CREATE)
			fafc->fafc_state = FAF_CASE_CREATE;
		else if (cp->cs_flag == CASE_FIRED)
			fafc->fafc_state = FAF_CASE_FIRED;
		else if (cp->cs_flag == CASE_CLOSED)
			fafc->fafc_state = FAF_CASE_CLOSED;
		else {
			printf("FMD: case %p (%d) has invalid state %u\n",
				(void *)cp, cp->cs_uuid, cp->cs_flag);
			return NULL;
		}
		cnt++;
		fafc = (void *)(acl->acl_buf + acl->acl_hdr->fafh_hdrsize + cnt * sizeof (faf_case_t));
	}
	*size = acl->acl_size;
	return (void *)acl->acl_hdr;
}


struct list_head *
adm_probe(evtsrc_module_t *emp)
{
	fmd_debug;
	fmd_t *pfmd = ((fmd_module_t *)emp)->mod_fmd;

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
//printf("##DEBUG##: fmd_adm_module: after mq_receive() size=%d\n", a);
//FIXME:if (strncmp(buf, FAF_GET_CASELIST, 12))
	sendbuf = fmd_get_caselist(pfmd, &size);

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

static evtsrc_modops_t adm_mops = {
	.evt_probe = adm_probe,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)evtsrc_init(&adm_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}
