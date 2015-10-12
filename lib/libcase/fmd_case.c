
#include "wrap.h"
#include "fmd.h"
#include "fmd_api.h"
#include "list.h"
#include "fmd_case.h"
#include "fmd_event.h"
#include "logging.h"

extern void put_to_agents(fmd_event_t *p_evt);

/* global reference */
fmd_t fmd;

/**
 * fmd_query_serd
 *
 * @param
 * @return
 */
struct fmd_serd_type *
fmd_query_serd(char *eclass)
{
	struct list_head *pos;
	struct fmd_serd_type *ptype = NULL;
	struct list_head *plist = &fmd.fmd_esc.list_serd;

	list_for_each(pos, plist)
	{
		ptype = list_entry(pos, struct fmd_serd_type, list);
		if(strncmp(eclass, ptype->eclass, strlen(eclass)) == 0)
			return ptype;
	}
	wr_log("", WR_LOG_ERROR, "ESC read error. not find serd configure");
	return NULL;
}

int
fmd_case_canfire(fmd_case_t *pcase, char *ev_class)
{
	uint64_t serdid;
	struct fmd_case_type *ptype = NULL;
	struct fmd_serd_type *pserd = NULL;

	pserd = fmd_query_serd(ev_class);
	if((pserd->N == pcase->cs_count) && (pserd->T >= (time(NULL) - pcase->cs_create)))
	{
		return 1;
	}
	return 0;
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
static fmd_case_t *
fmd_case_create(fmd_event_t *pevt)
{
	struct list_head *fmd_case = &fmd.list_case;
	
	fmd_case_t *pcreate = NULL;

	pcreate = (fmd_case_t *)def_calloc(sizeof(fmd_case_t), 1);

	pcreate->dev_name = strdup(pevt->dev_name);
	// wanglei add
	//pcreate->last_eclass = strdup(pevt->ev_class);
	//sprintf(pcreate->last_eclass,"%s",pevt->ev_class);
	pcreate->err_id  =  pevt->ev_err_id;

	pcreate->cs_flag = CASE_CREATE;
	pcreate->cs_create = time(NULL);
	
	
	INIT_LIST_HEAD(&pcreate->cs_event);
	//INIT_LIST_HEAD(&pcreate->cs_list);
	return pcreate;
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
	struct fmd_case_type *ptype = pcase->cs_type;

	if((pcase->cs_flag == CASE_FIRED )||(strcmp(pcase->dev_name, pevt->dev_name)!= 0))
		return 0;

	return 1;
}

void
fmd_case_fire(fmd_event_t *pevt, fmd_case_t *pcase)
{
	fmd_event_t *pfault = NULL;

	if(pcase->cs_fire_times == 0)
		pcase->cs_first_fire = time(NULL);
	else
		pcase->cs_last_fire = time(NULL);
	
	pcase->cs_flag = CASE_FIRED;
	pcase->cs_fire_times += 1;

	pevt->ev_flag = AGENT_TODO;
	pevt->ev_refs = pcase->cs_fire_times;
	
	put_to_agents(pevt);
	return;
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
	strcpy(pcase->last_eclass, pevt->ev_class);
	if(fmd_case_canfire(pcase, pevt->ev_class))
	{
		fmd_case_fire(pevt, pcase);
	}

	return ret;
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

// pevt --> pcase --> fmd_case
int
fmd_case_insert(fmd_event_t *pevt)
{
	int flag = 0;

	struct list_head *fmd_case = &fmd.list_case;
	struct list_head *fmd_rep_case = &fmd.list_repaired_case;

	struct list_head *fmd_fault = &fmd.fmd_esc.list_fault;
	struct list_head *fmd_serd  = &fmd.fmd_esc.list_serd;
	struct list_head *fmd_event = &fmd.fmd_esc.list_event;

	struct list_head *pos;
	
	// repaired event.
	list_for_each(pos, fmd_rep_case)
	{
		fmd_case_t *p_rep_case = list_entry(pos, fmd_case_t, cs_list);
		if((strcmp(p_rep_case->dev_name, pevt->dev_name) == 0) && (p_rep_case->err_id == pevt->ev_err_id))
		{
			wr_log("", WR_LOG_DEBUG, "have repaired and again occur");
			list_add(&pevt->ev_list, &p_rep_case->cs_event);
			p_rep_case->cs_count ++;
			fmd_case_fire(pevt, p_rep_case);
			goto DONE;
		}
	}

	list_for_each(pos, fmd_fault)
	{
		fmd_fault_type_t *p_fault = list_entry(pos, fmd_fault_type_t, list);
		if((strncmp(p_fault->eclass, pevt->ev_class, strlen(pevt->ev_class)-1) == 0)
														||(pevt->ev_flag == AGENT_TODO))
		{
			wr_log("",WR_LOG_DEBUG, "fault event turn to case........");
			fmd_case_t *p_fault_case = fmd_case_create(pevt);
			list_add(&p_fault_case->cs_list, fmd_case);
			list_add(&pevt->ev_list, &p_fault_case->cs_event);
			pevt->ev_flag = AGENT_TODO;
			put_to_agents(pevt);
			goto DONE;
		}			
	}
	
	// serd event
	list_for_each(pos, fmd_serd)
	{
		fmd_serd_type_t *pserd = list_entry(pos, fmd_serd_type_t, list);
		if(strncmp(pserd->eclass, pevt->ev_class, strlen(pevt->ev_class)-1) == 0)
		{
			list_for_each(pos, fmd_case)
			{
				fmd_case_t *pcase = list_entry(pos, fmd_case_t, cs_list);
				if(strncmp(pcase->dev_name, pevt->dev_name, strlen(pevt->ev_class)-1) == 0 &&
					(pcase->err_id == pevt->ev_err_id))
				{
					wr_log("", WR_LOG_DEBUG, "serd case exist and add.");
					fmd_case_add(pcase, pevt);
					goto LOG;
				}
			}
			//not find exist , and create new one
			wr_log("", WR_LOG_DEBUG, "serd case new and create add.");
			fmd_case_t *p_new_case = fmd_case_create(pevt);
			list_add(&p_new_case->cs_list, fmd_case);
			fmd_case_add(p_new_case, pevt);
			goto LOG;
		}
	}

LOG:
	wr_log("", WR_LOG_DEBUG, "event dispatch to agent.");
	pevt->ev_flag = AGENT_TOLOG;
	put_to_agents(pevt);
	return 0;
	
DONE:
	wr_log("", WR_LOG_DEBUG, "event dispatch to agent.");
	return 0;
}
