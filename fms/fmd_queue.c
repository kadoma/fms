
/************************************************************
 * Copyright (C) 2015, inspur Inc. <http://www.inspur.com>
 * FileName:    fmd_queue.c
 * Author:      wanghuan
 * Date:        2015-7-16
 * Description: distribute queue, and it is a bridage for agent & srcevent
 *              ./fms-2.0/fms/fmd_queue.c
 *
 ************************************************************/
#include <unistd.h>

#include "wrap.h"
#include "fmd_event.h"
#include "fmd.h"
#include "fmd_queue.h"
#include "list.h"
#include "fmd_ring.h"
#include "fmd_case.h"
#include "fmd_module.h"
#include "evt_agent.h"
#include "evt_src.h"
#include "fmd_esc.h"

static int
put_to_agent(fmd_event_t *event, agent_module_t *pm)
{
    ring_t *ring = pm->ring;

    sem_wait(&ring->ring_vaild);
    pthread_mutex_lock(&ring->ring_lock);
    ring_add(ring, event);
    pthread_mutex_unlock(&ring->ring_lock);
    sem_post(&ring->ring_contain);

    return 0;
}

/**
 * fmd_sub_matches
 *
 *
 * @param
 *     eclass: ereport.pcie.derr
 *
 *     sub:ereport.pcie.*
 *         ereport.pcie.derr
 * @return
 */
static int
fmd_sub_matches(char *eclass, char *sub)
{
    if(eclass == NULL || sub == NULL)
    {
        return -1;
        wr_log("", WR_LOG_ERROR, "fmd sub match eclass is null.");
    }
    char *p_sub_head = strchr(sub, '.');
    char *p_class_head = strchr(eclass, '.');
    
    char *p = strrchr(sub, '.');
    int size = p - p_sub_head;

    if(strcmp(p, ".*") == 0){
        return strncmp(p_class_head, p_sub_head, size) == 0;
    } else {
        return strcmp(p_class_head, p_sub_head) == 0;
    }
}

void
put_to_agents(fmd_event_t *p_evt)
{
    fmd_queue_t *p_queue = (fmd_queue_t *)pthread_getspecific(key_module);
    struct list_head *pos, *ppos;
    struct subitem *si;
    fmd_module_t *pmodule;

    /* search all subscribed modules */
    pthread_mutex_lock(&p_queue->queue_lock);
    
    list_for_each(pos, &p_queue->list_agent){
        pmodule = list_entry(pos, fmd_module_t, list_queue);

        /* search a single module */
        list_for_each(ppos, &pmodule->list_eclass){
            si = list_entry(ppos, struct subitem, si_list);
            if(fmd_sub_matches(p_evt->ev_class, si->si_eclass))
            {
                wr_log("", WR_LOG_DEBUG, "find agent to proc the event.[%s]", p_evt->ev_class);
                put_to_agent(p_evt, (agent_module_t *)pmodule);
                pthread_mutex_unlock(&p_queue->queue_lock);
                return;
            }
        }
    }
    pthread_mutex_unlock(&p_queue->queue_lock);
    wr_log("", WR_LOG_ERROR, "not find agent module to process event.[%s]",p_evt->ev_class);

    return;

}


int
event_disp_dest(fmd_event_t *p)
{
    if(strncmp(p->ev_class, "list", 4) == 0)
        return 0;
    else if(strncmp(p->ev_class, "ereport.", 8) == 0){
        // default to log event . if libcase . turn to other type.
        p->ev_flag = AGENT_TOLOG;
        return 1;
    }
    else
        return -1;
}

static int
case_close(fmd_case_t *pcase)
{
    return 0;
    
    TCHDB *hdb;
    bool ret = 0;
    int ecode;
    
    char key[128] = {0};
    char value[128] = {0};
    
    sprintf(key, "%s-%ld", pcase->dev_name, pcase->dev_id);
    sprintf(value, "eclass:%s, first time:%ld, last time:%ld, ereport total count:%u.", 
                    pcase->last_eclass, pcase->cs_create, pcase->cs_last_fire,
                    pcase->cs_total_count);

    hdb = tchdbnew();
    tchdbopen(hdb, CASE_DB, HDBOWRITER|HDBOCREAT);

    tchdbput2(hdb,key,value);
    tchdbclose(hdb);

    tchdbdel(hdb);

    struct list_head *pos, *n;
    list_for_each_safe(pos, n, &pcase->cs_event)
    {
        fmd_event_t *p_evt = list_entry(pos, fmd_event_t, ev_list);
        
        def_free(p_evt->dev_name);
        def_free(p_evt->ev_class);
        def_free(p_evt->data);
        list_del(&p_evt->ev_list);
        def_free(p_evt);
    }
    
    wr_log("", WR_LOG_DEBUG, "");
    return 0;
}


static int
fmd_case_close(fmd_event_t *pevt)
{
    fmd_case_t *pcase = (fmd_case_t *)pevt->p_case;
    wr_log("", WR_LOG_DEBUG, "LIST event close. [%s]", pevt->ev_class);

    if(!pcase){
        wr_log("",WR_LOG_ERROR, "fmd close case .case is null. big bug");
        return 0;
    }

    if(pevt->ev_refs >=2)
    {
        wr_log("event close", WR_LOG_DEBUG, "event refs is [%d], close event.", pevt->ev_refs);
        case_close(pcase);
    }
    else if((pevt->ev_count >= pevt->N) || (pcase->cs_last_fire - pcase->cs_create) > pevt->T)
    {
        wr_log("event close", WR_LOG_DEBUG, "event [%s] count is [%d], close event.", 
                                                                    pevt->ev_class, pevt->ev_count);
        case_close(pcase);
    }
/*
    if(pevt->ev_flag == AGENT_TODO){
        def_free(pevt->dev_name);
        def_free(pevt->ev_class);
        def_free(pevt->data);
        def_free(pevt);
    }
*/
    return 0;
}

int
fmd_proc_event(fmd_event_t *p_event)
{
    int ret= event_disp_dest(p_event);
    wr_log("dispatch event", WR_LOG_DEBUG, "dispatch event is %d type.", ret);
    if(ret == -1)
        wr_log("proc event", WR_LOG_ERROR, "unknow event and lost.");

    if(ret == 0)
        fmd_case_close(p_event);
        
    if(ret > 0)
    {
        fmd_case_insert(p_event);
    }
    return 0;
}

void *
queue_start(void *pp)
{
    fmd_event_t *evt = NULL;
    fmd_queue_t *p = (fmd_queue_t *)pp;
    pthread_setspecific(key_module, p);

    ring_t *p_ring = &p->queue_ring;
    
RETRY:

    while(ring_stat(p_ring) != 0)
    {
    sem_wait(&p_ring->ring_contain);
    wr_log("", WR_LOG_DEBUG, "queue start wait ring contain.");
    pthread_mutex_lock(&p_ring->ring_lock);
    evt = (fmd_event_t *)ring_del(p_ring);
    pthread_mutex_unlock(&p_ring->ring_lock);
    sem_post(&p_ring->ring_vaild);
    
    wr_log("", WR_LOG_DEBUG, "queue start wait ring contain get event is [%s]", evt->ev_class);
    if(evt != NULL)
        fmd_proc_event(evt);
    wr_log("", WR_LOG_DEBUG, "queue stat count is [%d]", p_ring->count);
    }
    sleep(1);
    goto RETRY;
}

/* begin --- by guanhj */
#define DELETE_CIRCLE    90 

void * queue_delete_start(void *pp)
{

	int ret = 0;
	fmd_queue_t *p_queue = (fmd_queue_t *)pp;
	pthread_setspecific(key_module, p_queue);
	
	list_repaired *plist_repaired = &p_queue->queue_repaired_list;

RETRY:
	wr_log("", WR_LOG_DEBUG, " queue delete start ");
	sem_wait(&plist_repaired->evt_repaired_sem);

    pthread_mutex_lock(&plist_repaired->evt_repaired_lock);
	struct list_head *prepaired_head = &plist_repaired->list_repaired_head;
	
    struct list_head *pos;
    struct list_head *listn;

    list_for_each_safe(pos, listn, prepaired_head)
    {
		fmd_event_t *pevt = list_entry(pos, fmd_event_t, ev_repaired_list);
		
		/* repair failed */
		if((pevt->agent_result & LIST_RESULT_MASK) != 0 )
		{
			//fault not repaired;
			if((pevt->agent_result == LIST_ISOLATED_FAILED) && (fmd_case_find_and_delete(pevt) != 0))		
			{
				wr_log("", WR_LOG_DEBUG, " isolate fail, no case list event, maybe device offline");
				def_free(pevt->dev_name);
		        def_free(pevt->ev_class);
		        def_free(pevt->data);
		        list_del(&pevt->ev_repaired_list);
		        def_free(pevt);
								
			}
			else
			{
				//fault not repaired;
			// to do 
			wr_log("", WR_LOG_DEBUG, " repaired  result err  \n");
			}
			continue;
		}
		/* device isolated */
		if(pevt->agent_result == LIST_ISOLATED_SUCCESS)
		{
			//delete related listevt and case 
			ret = fmd_case_find_and_delete(pevt);
			if(ret != 0)
			{
				wr_log("", WR_LOG_DEBUG, "delete case list event err");
			}	

			def_free(pevt->dev_name);
	        def_free(pevt->ev_class);
	        def_free(pevt->data);
	        list_del(&pevt->ev_repaired_list);
	        def_free(pevt);
			continue;
		}
		
		/* event had been repaired */	
		if((pevt->repaired_T <= (time(NULL) - pevt->ev_create)) && (pevt->ev_refs < pevt->repaired_N))
		{		
			/* find and delete in fmd.list_case */
			ret = fmd_case_find_and_delete(pevt);
			if(ret != 0)
			{
				wr_log("", WR_LOG_DEBUG, " no case list event, maybe device offline");
			}
			/* delete repaired list event */	
			def_free(pevt->dev_name);
	        def_free(pevt->ev_class);
	        def_free(pevt->data);
	        list_del(&pevt->ev_repaired_list);
	        def_free(pevt);

		}

		/*  not been repaired   */
		else if((pevt->repaired_T > (time(NULL) - pevt->ev_create)) && (pevt->ev_refs >= pevt->repaired_N)) 
			
		{
			//fault not repaired;
			// to do  
			//wr_log("", WR_LOG_DEBUG, "fault not repaired ");
		}
		/* not been repaired */
		else if((pevt->repaired_T <= (time(NULL) - pevt->ev_create)) && (pevt->ev_refs >= pevt->repaired_N))
		{
			//to do
			//wr_log("", WR_LOG_DEBUG, "fault timeout and ev_refs more ");
                       
		}
		
		//else /* < repaired_T and < repaired_N : continue */
					
		
	}
 
    pthread_mutex_unlock(&plist_repaired->evt_repaired_lock);

	sleep(DELETE_CIRCLE);
	goto RETRY;

	

}

/* end --- by guanhj */


int
fmd_queue_load(fmd_t *pfmd)
{
    int ret = 0;
    fmd_queue_t *p = &pfmd->fmd_queue;
    pthread_t  pid;

    ret = pthread_create(&pid, NULL, queue_start, (void *)p);
    if(ret < 0){
        wr_log("fmd", WR_LOG_ERROR, "fmd queue thread error.");
        return -1;
    }

	/* delete repaired event thread by guanhj */
	ret = pthread_create(&pid, NULL, queue_delete_start, (void *)p);
    if(ret < 0){
        wr_log("fmd", WR_LOG_ERROR, "fmd queue delete repaired event thread error.");
        return -1;
    }

    return 0;
}
