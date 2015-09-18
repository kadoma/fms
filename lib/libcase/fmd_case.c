
#include "wrap.h"
#include "fmd.h"
#include "fmd_api.h"
#include "list.h"
#include "fmd_case.h"
#include "fmd_event.h"
#include "logging.h"

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

	pcreate->dev_name = pevt->dev_name;
	pcreate->err_id  =  pevt->ev_err_id;

	pcreate->cs_flag = CASE_CREATE;
	pcreate->cs_create = time(NULL);
	
	
	INIT_LIST_HEAD(&pcreate->cs_event);
	INIT_LIST_HEAD(&pcreate->cs_list);
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

/**
 * fmd_case_fire
 *
 * @param
 * @return
 */
void
fmd_case_fire(fmd_case_t *pcase)
{
	fmd_event_t *pfault = NULL;

	if(pcase->cs_fire_times == 0)
		pcase->cs_first_fire = time(NULL);
	else
		pcase->cs_last_fire = time(NULL);
	
	pcase->cs_flag = CASE_FIRED;
	pcase->cs_fire_times += 1;

	pfault = (fmd_event_t *)fmd_create_casefault(&fmd, pcase);
	put_to_agents(pfault);
	
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

	if(fmd_case_canfire(pcase, pevt->ev_class))
	{
		fmd_case_fire(pcase);
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
	
	fmd_case_t *pcase = NULL;
	fmd_case_t *p_rep_case = NULL;
	struct list_head *pos;

	list_for_each(pos, fmd_rep_case)
	{
		p_rep_case = list_entry(pos, fmd_case_t, cs_list);
		if((strcmp(pcase->dev_name, pevt->dev_name) == 0) && (pcase->err_id == pevt->ev_err_id))
		{
			wr_log("", WR_LOG_DEBUG, "have repaired and again occur");
			fmd_case_fire(p_rep_case);
			goto done;
		}
	}
	// not direct fault event. agent proc end . delete case to add rep_case
	list_for_each(pos, fmd_fault)
	{
		fmd_fault_type_t *p_fault = list_entry(pos, fmd_fault_type_t, list);
		if(strncmp(p_fault->eclass, pevt->ev_class, strlen(pevt->ev_class)-1) == 0)
		{
			wr_log("",WR_LOG_DEBUG, "esc fault event turn to case once event");
			fmd_case_t *p_fault_case = fmd_case_create(pevt);
			list_add(&p_fault_case->cs_list, fmd_case);
			put_to_agents(pevt);
			goto done;
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
					goto done;
				}
			}
			wr_log("", WR_LOG_DEBUG, "serd case new and create add.");
			fmd_case_t *p_new_case = fmd_case_create(pevt);
			list_add(&p_new_case->cs_list, fmd_case);
			fmd_case_add(p_new_case, pevt);
			goto done;
		}
	}

	//list event
	wr_log("", WR_LOG_DEBUG, "event only list log.");
	put_to_agents(pevt);

done:
	return 0;
}
