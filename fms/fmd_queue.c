
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
    if(strncmp(p->ev_class, "ereport.", 8) == 0)
    {
        p->ev_flag = AGENT_TOLOG;
        return 1;
    }
    else
        return -1;
}

int
fmd_proc_event(fmd_event_t *p_event)
{
    int ret= event_disp_dest(p_event);
    
    wr_log("dispatch event", WR_LOG_DEBUG, "dispatch event is %d type.", ret);
    
    if(ret == -1)
        wr_log("proc event", WR_LOG_ERROR, "unknow event and lost.");

    if(ret > 0)
    {
        fmd_case_insert(p_event);
    }
    return 0;
}


/* begin --- by guanhj */
#define DELETE_CIRCLE    20 

void * queue_delete_proc(fmd_queue_t *pp)
{

	int ret = 0;
	fmd_queue_t *p_queue = (fmd_queue_t *)pp;

    list_repaired *plist_repaired = &p_queue->queue_repaired_list;

	wr_log("", WR_LOG_DEBUG, " queue delete proc ");

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

                //double free .
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

			//def_free(pevt->dev_name);
	        //def_free(pevt->ev_class);
	        //def_free(pevt->data);
	        //list_del(&pevt->ev_repaired_list);
	        //def_free(pevt);
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
	        //def_free(pevt->data);
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
	return NULL;
}

/* end --- by guanhj */



void *
queue_start(void *pp)
{
	int count = 0;
    fmd_event_t *evt = NULL;
    fmd_queue_t *p = (fmd_queue_t *)pp;
    pthread_setspecific(key_module, p);

    ring_t *p_ring = &p->queue_ring;
    ras_event_opendb();
    
RETRY:
	count++;
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
//wanghuan
	if(count == DELETE_CIRCLE)
	{
		queue_delete_proc(p);
		count = 0;
	}

    sleep(1);
    goto RETRY;
}




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


    return 0;
}
