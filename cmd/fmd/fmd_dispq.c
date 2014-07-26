/*
 * fmd_dispq.c
 *
 *  Created on: Jan 16, 2010
 *      Author: Inspur OS Team
 *
 *  Description:
 *  	Dispatch Queue
 *
 *  TODO:
 *  	Soft TLB
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <syslog.h>
#include <fmd_dispq.h>
#include <fmd_event.h>
#include <fmd_agent.h>
#include <fmd_list.h>

/* global reference */
extern fmd_t fmd;

/**
 * fmd_sub_matches
 *
 * @param
 * 	eclass: ereport.io.pcie.derr
 * 	sub:ereport.io.pcie.*
 * 		ereport.io.pcie.derr
 * @return
 */
static int
fmd_sub_matches(char *eclass, char *sub)
{
	char *p = strrchr(sub, '.');
	int size = p - sub;

	if(strcmp(p, ".*") == 0) {
		return strncmp(eclass, sub, size) == 0;
	} else {
		return strcmp(eclass, sub) == 0;
	}
}


/**
 * pull to an agent module
 *
 * @param
 * @return
 */
static int
pull_to_agent(fmd_event_t *event, agent_module_t *pm)
{
	ring_t *ring = pm->ring;

	sem_wait(&ring->r_empty);
	pthread_mutex_lock(&ring->r_mutex);
	ring_add(ring, event);
	pthread_mutex_unlock(&ring->r_mutex);
	sem_post(&ring->r_full);

	return 0;
}


/**
 * Pull to all subscribes
 *	list.* -> self
 *	fault.* -> agent
 *	ereport.* -> self
 *
 * @param
 * @return
 */
int
pull_to_agents(fmd_event_t *evt)
{
	fmd_dispq_t *disp = (fmd_dispq_t *)
			pthread_getspecific(t_module);
	struct list_head *pos, *ppos;
	struct subitem *si;
	fmd_module_t *pmodule;
	char *eclass;

	/* search all subscribed modules */
	pthread_mutex_lock(&disp->dq_mutex);
	list_for_each(pos, &disp->list_agent) {
		pmodule = list_entry(pos, fmd_module_t, list_dispq);

		/* search a single module */
		list_for_each(ppos, &pmodule->list_eclass) {
			si = list_entry(ppos, struct subitem, si_list);
			eclass = si->si_eclass;
			if(fmd_sub_matches(evt->ev_class, eclass)) /* found it */
				pull_to_agent(evt, (agent_module_t *)pmodule);
		} /* inner list_for_entry */
	} /* outer list_for_entry */
	pthread_mutex_unlock(&disp->dq_mutex);

	return 0;
}


/**
 * pull to a single module
 *
 * @param
 * @return
 */
static int
fmd_disp_event(fmd_event_t *event)
{
	int dest = event_disp_dest(event);
	fmd_log_event(event);

	switch(dest) {
	case DESTO_SELF: /* send to a diagnosis module */
		fmd_case_insert(event);
		break;
	case DESTO_AGENT: /* send to an agent module */
		pull_to_agents(event);
		break;
	default:
		//TODO default(invalid) actions
		break;
	}

	return 0;
}


/**
 * The real worker
 * is just lying here...
 * hehe :-)
 */
int
do_dispq(ring_t *ring)
{
	fmd_event_t *evt = NULL;

	/* consumer all events in the ring buffer */
	while(!ring_empty(ring)) {
		/* 1. retrieve event */
		sem_wait(&ring->r_full);
		pthread_mutex_lock(&ring->r_mutex);
		evt = (fmd_event_t *)ring_del(ring);
		pthread_mutex_unlock(&ring->r_mutex);
		sem_post(&ring->r_empty);
		if(evt == NULL) {
			syslog(LOG_ERR, "ring_del empty event");
			exit(EXIT_FAILURE);
		} //if

		/* 2. pull to subscribes */
		fmd_disp_event(evt);
	}

	return 0;
}


/**
 * Starts the disq...
 *
 * TODO
 * 	Multiple events arrives in the same time.
 */
void *
__dispq_start(void *arg)
{
	fmd_dispq_t *p = (fmd_dispq_t *)arg;
	ring_t *ring = &p->dq_ring;

	/* tsd */
	pthread_setspecific(t_module, p);

	/* bigins to work... */
	while(1) {
//		dispq_count = 0;
		do_dispq(ring);
	}

	return NULL;
}


/**
 * load dispq module
 *
 * @param
 * @return
 */
int
fmd_dispq_load()
{
	int ret;
	fmd_dispq_t *p = &fmd.fmd_dispq;
	pthread_t *pid = &p->dq_pid;
	pthread_attr_t attr;

	pthread_attr_init(&attr);

	ret = pthread_create(pid, &attr, __dispq_start, (void *)p);
	if(ret < 0) {
		syslog(LOG_ERR, "pthread_create");
		exit(EXIT_FAILURE);
	}

	return 0;
}
