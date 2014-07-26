/*
 * fmd_case.c
 *
 *  Created on: Feb 23, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_case.c
 */

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <fmd.h>
#include <fmd_api.h>
#include <fmd_list.h>
#include <fmd_case.h>
#include <fmd_event.h>
#include <fmd_xprt.h>
#include <fmd_log.h>

/* global reference */
extern fmd_t fmd;

/**
 * fmd_casetype_contains
 *
 * @param
 * @return
 */
int
fmd_casetype_contains(struct fmd_case_type *ptype, uint64_t eid)
{
	int ite = 0;

	for(; ite < ptype->size; ite++) {
		if(ptype->ereport[ite] == eid) return 1;
	}

	return 0;
}


/**
 * create a new case from casetype tmplate
 *
 * @param
 * @return
 */
static fmd_case_t *
template_create_case(struct fmd_case_type *ptype)
{
	fmd_case_t *pcase = NULL;

	pcase = (fmd_case_t *)malloc(sizeof(fmd_case_t));
	assert(pcase != NULL);
	memset(pcase, 0, sizeof(fmd_case_t));

	pcase->cs_uuid = (uint64_t)pcase;
	pcase->cs_flag = CASE_CREATE;
	pcase->cs_type = ptype;
	pcase->cs_create = time(NULL);
	INIT_LIST_HEAD(&pcase->cs_event);
	INIT_LIST_HEAD(&pcase->cs_list);

	return pcase;
}


/**
 * create a new case
 *
 * @param
 * @return
 * 	the number of new created cases.
 * TODO
 * 	The maximum cases are (predefined) 4.
 */
static fmd_case_t **
fmd_case_create(fmd_event_t *pevt, int *csize)
{
	int count = 0;
	uint64_t eid = pevt->ev_eid;
	struct list_head *pcstype = &fmd.fmd_esc.list_casetype;
	struct list_head *pos = NULL;
	struct list_head *pcase = &fmd.list_case;
	struct fmd_case_type *p = NULL;
	fmd_case_t *pcreate = NULL;
	fmd_case_t **pnew;

	pnew = (fmd_case_t **)malloc(sizeof(fmd_case_t *) * 4);
	assert(pnew != NULL);

	*csize = 0;
	list_for_each(pos, pcstype) {
		p = list_entry(pos, struct fmd_case_type, list);

		if (!fmd_casetype_contains(p, eid)) continue;

		pcreate = template_create_case(p);
		pcreate->cs_rscid = pevt->ev_rscid;
		pnew[*csize] = pcreate;

		(*csize)++;
	}

	return pnew;
}

/**
 * fmd_case_contains
 *
 * @param
 * @return
 */
static int
fmd_case_contains(fmd_case_t *pcase, fmd_event_t *pevt)
{
	int ite = 0;
	struct list_head *pos;
	uint64_t eid = pevt->ev_eid;
	struct fmd_case_type *ptype = pcase->cs_type;

	if(pcase->cs_flag == CASE_FIRED ||
			pcase->cs_rscid != pevt->ev_rscid)
		return 0;

	for(; ite < ptype->size; ite++) {
		if(eid == ptype->ereport[ite])
			return 1;
	}

	return 0;
}


/**
 * fmd_case_fire
 *
 * @param
 * @return
 */
int
fmd_case_fire(fmd_case_t *pcase)
{
	fmd_event_t *pfault;

	pcase->cs_fire = time(NULL);
	pcase->cs_flag = CASE_FIRED;

	/* create fault event */
	pfault = (fmd_event_t *)fmd_create_fault(&fmd, pcase);

	fmd_debug;
	fmd_log_event(pfault);

	pull_to_agents(pfault);

	return 1;
}


/**
 * fmd_case_canfire
 *
 * @param
 * @return
 */
int
fmd_case_canfire(fmd_case_t *pcase)
{
	return fmd_serd_canfire(pcase);
}


/**
 * fmd_case_add
 *
 * @param
 * @return
 * @TODO
 * 	1.check if pcase contains pevt
 *  2.record every event's count number
 */
static int
fmd_case_add(fmd_case_t *pcase, fmd_event_t *pevt)
{
	int ret = 0;

	pcase->cs_count++;
	list_add(&pevt->ev_list, &pcase->cs_event);

	if(fmd_case_canfire(pcase)) {
		fmd_case_fire(pcase);
	}

	return ret;
}


/**
 * fmd_case_close
 *
 * @param
 * 	list.* event
 * @return
 */
static int
fmd_case_close(fmd_event_t *pevt)
{
	fmd_debug;
	fmd_case_t *pnode = (fmd_case_t *)pevt->ev_uuid;
	list_del(&pnode->cs_list);
	return 1;
}


/**
 * fmd_case_insert
 *
 * @param
 * 	list.*
 * 	ereport.*
 * @return
 * 	1: found
 * 	0: not found
 */
int
fmd_case_insert(fmd_event_t *pevt)
{
	int flag = 0;
	struct list_head *pos;
	struct list_head *pcase = &fmd.list_case;
	fmd_case_t *pnode = NULL;

	/* list.* */
	if((pevt->ev_eid == 0) &&
			(strncmp(pevt->ev_class, "list.", 5) == 0)) {
		return fmd_case_close(pevt);
	}

	/* ereport.* */
	list_for_each(pos, pcase) {
		pnode = list_entry(pos, fmd_case_t, cs_list);
		if(fmd_case_contains(pnode, pevt)) {
			flag = 1;
			fmd_case_add(pnode, pevt);
		}
	}

	fmd_case_t **pnew;
	int ite, size = 0;
	if(flag == 0) {
		pnew = fmd_case_create(pevt, &size);	/* one event for many cases */
		for(ite = 0; ite < size; ite++) {
			pnode = pnew[ite];
			fmd_case_add(pnode, pevt);
			list_add(&pnode->cs_list, pcase);
		}
	}

	return 0;
}
