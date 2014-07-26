/*
 * fmd_serd.c
 *
 *  Created on: Feb 23, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_serd.c
 */

#include <syslog.h>
#include <fmd.h>
#include <fmd_serd.h>
#include <fmd_case.h>

/* global reference */
extern fmd_t fmd;

/**
 * fmd_query_serd
 *
 * @param
 * @return
 */
struct fmd_serd_type *
fmd_query_serd(uint64_t sid)
{
	struct list_head *pos;
	struct fmd_serd_type *ptype;
	struct list_head *plist = &fmd.fmd_esc.list_serdtype;

	list_for_each(pos, plist) {
		ptype = list_entry(pos, struct fmd_serd_type, list);
		if(ptype->fmd_serdid == sid) return ptype;
	}

	return NULL;
}


/**
 * fmd_serd_canfire
 *
 * @param
 * @return
 */
int
fmd_serd_canfire(fmd_case_t *pcase)
{
	uint64_t serdid;
	struct fmd_case_type *ptype = NULL;
	struct fmd_serd_type *pserd = NULL;

	/* check for fired */
	ptype = pcase->cs_type;
	serdid = ptype->serd;

	if(serdid == 0) {
		/* deduce fault.* immediately */
		return 1;
	}

	pserd = fmd_query_serd(serdid);
	if(pserd == NULL) {
		syslog(LOG_ERR, "fmd_query_serd");
		exit(-1);
	}
	if((pserd->N == pcase->cs_count) &&
			(pserd->T >= (time(NULL) - pcase->cs_create))) {
		return 1;
	}

	return 0;
}
