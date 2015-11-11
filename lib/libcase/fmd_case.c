
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

static fmd_case_t *
fmd_case_create(fmd_event_t *pevt)
{
    struct list_head *fmd_case = &fmd.list_case;
    
    fmd_case_t *pcreate = (fmd_case_t *)def_calloc(sizeof(fmd_case_t), 1);

    pcreate->dev_name = strdup(pevt->dev_name);
    sprintf(pcreate->last_eclass, "%s", pevt->ev_class);
    pcreate->dev_id = pevt->dev_id;

    pcreate->cs_flag = CASE_CREATE;
    pcreate->cs_create = time(NULL);
    
    INIT_LIST_HEAD(&pcreate->cs_event);
    INIT_LIST_HEAD(&pcreate->cs_list);
    return pcreate;
}


void
fmd_case_fire(fmd_case_t *pcase, fmd_event_t *pevt)
{
    fmd_event_t *pfault = (fmd_event_t *)def_calloc(sizeof(fmd_event_t), 1);

    pfault->dev_name = strdup(pevt->dev_name);
    pfault->ev_class = strdup(pevt->ev_class);
    pfault->dev_id = pevt->dev_id;
    pfault->evt_id = pevt->evt_id;
    pfault->data = pevt->data;
	pfault->repaired_N = pevt->repaired_N;
	pfault->repaired_T = pevt->repaired_T;
	
    if(pcase->cs_fire_counts == 0){
        pcase->cs_first_fire = time(NULL);
        pcase->cs_last_fire = time(NULL);
    }
    else
        pcase->cs_last_fire = time(NULL);
    
    pfault->ev_flag = AGENT_TODO;
    pfault->event_type = EVENT_FAULT;
    
    pcase->cs_fire_counts += 1;
    pfault->ev_refs = pcase->cs_fire_counts;
    

    pcase->cs_flag = CASE_FIRED;
    
    put_to_agents(pfault);

    return;
}

int
fmd_case_canfire(fmd_event_t *pevt)
{
    if(pevt->event_type == EVENT_FAULT)
        return 1;

    if((pevt->N <= pevt->ev_count)&&(pevt->T > (time(NULL) - pevt->ev_create)))
    {
        return 1;
    }
    
    return 0; 
}

void
fmd_event_add(fmd_case_t *pcase, fmd_event_t *pevt)
{
    int hit= 0;
    fmd_event_t *p_event = NULL;
    pevt->p_case = pcase;
    
    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &pcase->cs_event)
    {
        p_event = list_entry(pos, fmd_event_t, ev_list);
        if(p_event->evt_id == pevt->evt_id)
        {
            hit = 1;
            p_event->ev_count ++;
        }
    }

    if(!hit)
    {
        list_add(&pevt->ev_list, &pcase->cs_event);
        strcpy(pcase->last_eclass, pevt->ev_class);
        pevt->ev_count ++;
        pevt->ev_create = time(NULL);
        p_event = pevt;
    }
    
    if(fmd_case_canfire(p_event))
        fmd_case_fire(pcase, pevt);
    
    return;
}

void
add_case_list(fmd_event_t *pevt)
{
    struct list_head *fmd_case = &fmd.list_case;
    fmd_event_t *p = NULL;
    
    struct list_head *pos;
    list_for_each(pos, fmd_case)
    {
        fmd_case_t *p_case = list_entry(pos, fmd_case_t, cs_list);
        if((strcmp(p_case->dev_name, pevt->dev_name) == 0) &&
                                        (p_case->dev_id == pevt->dev_id))
        {
            fmd_event_add(p_case, pevt);
            return;
        }
    }

    fmd_case_t *p_new_case = fmd_case_create(pevt);
    list_add(&p_new_case->cs_list, fmd_case);
    fmd_event_add(p_new_case, pevt);
    
    return;
}


int
fmd_case_insert(fmd_event_t *pevt)
{
    int type = 0;
    fmd_event_t *p = NULL;
    struct list_head *fmd_fault = &fmd.fmd_esc.list_fault;
    struct list_head *fmd_serd  = &fmd.fmd_esc.list_serd;

    struct list_head *pos;
    list_for_each(pos, fmd_fault)
    {
        fmd_fault_type_t *p_fault = list_entry(pos, fmd_fault_type_t, list);
        if(strncmp(p_fault->eclass, pevt->ev_class, strlen(p_fault->eclass)) == 0)
        {
            pevt->event_type = EVENT_FAULT;
        }
    }

    struct list_head *ppos;
    list_for_each(ppos, fmd_serd)
    {
        fmd_serd_type_t *pserd = list_entry(ppos, fmd_serd_type_t, list);
        if(strncmp(pserd->eclass, pevt->ev_class, strlen(pserd->eclass)) == 0)
        {
            pevt->N = pserd->N;
            pevt->T = pserd->T;
            pevt->event_type = EVENT_SERD;
        }
    }
    if(pevt->event_type == 0){
        pevt->event_type = EVENT_REPORT;
    }

    add_case_list(pevt);
        
    wr_log("", WR_LOG_DEBUG, "event dispatch to agent.");
    put_to_agents(pevt);
    return 0;
}


int fmd_case_find_and_delete(fmd_event_t *pevt)
{
	struct list_head *fmd_case = &fmd.list_case;
	fmd_event_t *p = NULL;

	struct list_head *pos, *pos_n;
	list_for_each_safe(pos, pos_n, fmd_case)
	{
	    fmd_case_t *p_case = list_entry(pos, fmd_case_t, cs_list);
	    if((strcmp(p_case->dev_name, pevt->dev_name) == 0) &&
	                                    (p_case->dev_id == pevt->dev_id))
		{
            if(pevt->agent_result == LIST_ISOLATED_SUCCESS)
            {
				fmd_event_t *p_event = NULL;

				struct list_head *pos_evt, *n;
				list_for_each_safe(pos_evt, n, &p_case->cs_event)
				{
				    p_event = list_entry(pos_evt, fmd_event_t, ev_list);
					def_free(p_event->dev_name);
			        def_free(p_event->ev_class);
			        def_free(p_event->data);
			        list_del(&p_event->ev_list);
			        def_free(p_event);
					

				}
				def_free(p_case->dev_name);
				list_del(&p_case->cs_list);
				def_free(p_case);
				return 0;
			}
			
	   		else
	   		{	
				fmd_event_t *p_event = NULL;

				struct list_head *pos_evt, *n;
				list_for_each_safe(pos_evt, n, &p_case->cs_event)
				{
				    p_event = list_entry(pos_evt, fmd_event_t, ev_list);
				    if(p_event->evt_id == pevt->evt_id)
				    {
							
						def_free(p_event->dev_name);
				        def_free(p_event->ev_class);
				        def_free(p_event->data);
				        list_del(&p_event->ev_list);
				        def_free(p_event);

						/* case is empty or not */
						if(list_empty(pos))
						{
							def_free(p_case->dev_name);
							list_del(&p_case->cs_list);
							def_free(p_case);
					
						}

						return 0;
				    }
				}
			}

		}
	}

	return  -1;
}