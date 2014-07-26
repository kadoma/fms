/*
 * fmd_event.c
 *
 *  Created on: Feb 23, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_event.c
 */

#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <fmd.h>
#include <fmd_event.h>
#include <fmd_list.h>
#include <fmd_case.h>

/**
 * fmd_event_getid
 *
 * @param
 * @return
 * 	0: error
 * 	!0:success
 */
static uint64_t
fmd_event_getid(fmd_t *pfmd, const char *eclass)
{
	uint64_t eid = 0;
	struct fmd_hash *phash = &pfmd->fmd_esc.hash_clsname;
	struct fmd_hash *pnode;
	struct list_head *pos;

	list_for_each(pos, &phash->list) {
		pnode = list_entry(pos, struct fmd_hash, list);

		if(strcmp(pnode->key, eclass) == 0) {
			eid = pnode->value;
			break;
		}
	}

	return eid;
}


/**
 * fmd_event_getclass
 *
 * @param
 * @return
 * 	0: error
 * 	!0:success
 */
static const char *
fmd_event_getclass(fmd_t *pfmd, uint64_t eid)
{
	struct fmd_hash *phash = &pfmd->fmd_esc.hash_clsname;
	struct fmd_hash *pnode;
	struct list_head *pos;

	list_for_each(pos, &phash->list) {
		pnode = list_entry(pos, struct fmd_hash, list);

		if(pnode->value == eid) return pnode->key;
	}

	return NULL;
}


/**
 * fmd_create_event
 *
 * @param
 * @return
 */
fmd_event_t *
fmd_create_ereport(fmd_t *pfmd,
		const char *eclass,
		uint64_t rscid,
		nvlist_t *data)
{
	fmd_event_t *pevt = NULL;
	uint64_t eid;

	pevt = (fmd_event_t *)malloc(sizeof(fmd_event_t));
	printf("##DEBUG##: [%p]\n", pevt);
	assert(pevt != NULL);

	eid = fmd_event_getid(pfmd, eclass);
	if(eid == 0) {
		syslog(LOG_ERR, "Cannot Resolve Event: %s\n", eclass);
		free(pevt);
		return NULL;
	}

	pevt->ev_create = time(NULL);
	pevt->ev_refs = 0;
	pevt->ev_class = strdup(eclass);
	pevt->ev_eid = eid;
	pevt->ev_uuid = 0;
	pevt->ev_rscid = rscid;
	pevt->ev_data = data;
	INIT_LIST_HEAD(&pevt->ev_list);

	printf("##DEBUG##: [%p]\n", pevt);
	return pevt;
}


/**
 * fmd_create_event
 *
 * @param
 * @return
 */
fmd_event_t *
fmd_create_fault(fmd_t *pfmd, fmd_case_t *pcase)
{
	fmd_event_t *pevt = NULL;
	struct fmd_case_type *pcstype = pcase->cs_type;
	uint64_t eid = pcstype->fault;
	const char *eclass = fmd_event_getclass(pfmd, eid);

	pevt = (fmd_event_t *)malloc(sizeof(fmd_event_t));
	assert(pevt != NULL);
	memset(pevt, 0, sizeof(fmd_event_t));

	pevt->ev_create = time(NULL);
	pevt->ev_refs = 0;
	pevt->ev_class = strdup(eclass);
	pevt->ev_eid = eid;
	pevt->ev_uuid = (uint64_t)pcase;
	pevt->ev_rscid = pcase->cs_rscid;
	pevt->ev_data = NULL;
	INIT_LIST_HEAD(&pevt->ev_list);

	return pevt;
}


/**
 * fmd_create_list
 *
 * @param
 * @return
 */
fmd_event_t *
fmd_create_listevent(fmd_event_t *fault, int action)
{
	fmd_event_t *pevt = NULL;
	uint64_t eid;
	char eclass[64];

	pevt = (fmd_event_t *)malloc(sizeof(fmd_event_t));
	assert(pevt != NULL);
	memset(pevt, 0, sizeof(fmd_event_t));

	memset(eclass, 0, 64);
	sprintf(eclass, "list.%s", &fault->ev_class[6]);

	pevt->ev_create = time(NULL);
	pevt->ev_refs = 0;
	pevt->ev_class = strdup(eclass);
	pevt->ev_uuid = fault->ev_uuid;
	pevt->ev_rscid = fault->ev_rscid;
	pevt->ev_data = (void *)((int64_t)action);
	INIT_LIST_HEAD(&pevt->ev_list);

	return pevt;
}
