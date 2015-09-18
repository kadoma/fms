
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

	sem_wait(&ring->ring_empty);
	pthread_mutex_lock(&ring->ring_lock);
	ring_add(ring, event);
	pthread_mutex_unlock(&ring->ring_lock);
	sem_post(&ring->ring_full);

	return 0;
}

/**
 * fmd_sub_matches
 *
 *
 * @param
 * 	eclass: ereport.io.pcie.derr
 *
 * 	sub:ereport.io.pcie.*
 * 		ereport.io.pcie.derr
 * @return
 */
static int
fmd_sub_matches(char *eclass, char *sub)
{
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
	if(strncmp(p->ev_class, "list.", 5) == 0)
		return 0;
	else if(strncmp(p->ev_class, "fault.", 6)== 0){
		p->ev_flag = AGENT_TODO;
		return 1;
	}
	else if(strncmp(p->ev_class, "ereport.", 8) == 0){
		p->ev_flag = AGENT_TOLOG;
		return 2;
	}
	else
		return -1;
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
	struct list_head *pcase_head = &fmd.list_case;
	
	struct list_head *pos, *n;
	list_for_each_safe(pos, n, pcase_head)
	{
		fmd_case_t *pcase = list_entry(pos, fmd_case_t, cs_list);
		if((strcmp(pcase->dev_name, pevt->dev_name) == 0)&&(pcase->err_id == pevt->ev_err_id))
		{
			list_del(&pcase->cs_list);
			list_add(&pcase->cs_list, &fmd.list_repaired_case);
			wr_log("", WR_LOG_NORMAL, "case had been processed, delete and add.");
			return 0;
		}
	}
	wr_log("", WR_LOG_ERROR, "case close error.");
	return -1;
}


int
fmd_proc_event(fmd_event_t *p_event)
{
	int ret= event_disp_dest(p_event);
	wr_log("dispatch event", WR_LOG_DEBUG, "dispatch event is %d.", ret);
	if(ret == -1)
		wr_log("proc event", WR_LOG_ERROR, "unknow event and lost.");

	if(ret == 0)
		fmd_case_close(p_event);
		
	if(ret > 0)
	{
		put_to_agents(p_event);
		if(ret == 2)
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
        pthread_mutex_lock(&p_ring->ring_lock);
        evt = (fmd_event_t *)ring_del(p_ring);
        pthread_mutex_unlock(&p_ring->ring_lock);

        if(evt != NULL)
        	fmd_proc_event(evt);
		sleep(1);
		wr_log("", WR_LOG_DEBUG, "queue stat count is [%d]", p_ring->count);
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
