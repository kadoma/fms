/*
 * agent.c
 *
 *  Created on: Jan 18, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent module
 */

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <syslog.h>
#include <fmd.h>
#include <fmd_list.h>
#include <fmd_agent.h>
#include <fmd_dispq.h>


/**
 * @param
 * @return
 *
 * TODO return value should be valuable
 */
int
do_agent(ring_t *ring)
{
	fmd_event_t *evt = NULL, *eresult;
	agent_module_t *mp = pthread_getspecific(t_module);
	agent_modops_t *mops = mp->mops;
	fmd_t *pfmd = ((fmd_module_t *)mp)->mod_fmd;

	/* consumer all events in the ring buffer */
	while(!ring_empty(ring)) {
		/* 1. retrieve event */
		sem_wait(&ring->r_full);
		pthread_mutex_lock(&ring->r_mutex);
		evt = (fmd_event_t *)ring_del(ring);
		pthread_mutex_unlock(&ring->r_mutex);
		sem_post(&ring->r_empty);
		if(evt == NULL) {
			syslog(LOG_ERR, "ring_del");
			exit(EXIT_FAILURE);
		} //if

		/* handle event, eresult is of type "list.*" */
		eresult = mops->evt_handle(pfmd, evt);
		if ( eresult == NULL ) {	/* don't add empty item into ring */
#ifdef FMD_DEBUG
			syslog(LOG_ERR, "Empty handle result");
#endif
			return 0;
		}
		fmd_xprt_put(pfmd, eresult);
	}

	return 0;
}


/**
 * @param
 * @return
 */
static void *
__agent_start(void *arg)
{
	int ret;
	sigset_t fullset, oldset;
	agent_module_t *emp;
	agent_modops_t *mops;
	ring_t *ring = ((agent_module_t *)arg)->ring;

	/* set TSD */
	pthread_setspecific(t_module, arg);

	/* signal */
	sigfillset(&fullset);
	sigdelset(&fullset, SIGKILL);
	sigdelset(&fullset, SIGSTOP);
	pthread_sigmask(SIG_BLOCK, &fullset, &oldset);

	/* container */
	emp = (agent_module_t *)pthread_getspecific(t_module);

	/* Thread starts working... */
	while(1) {
		do_agent(ring);
	}

	return NULL;
}


/**
 * Create a thread serving Event-Source module.
 *
 */
static pthread_t
pthread_init(void *arg)
{
	pthread_t pid;
	int ret;
	pthread_attr_t attr;
	agent_module_t *mp = (agent_module_t *)arg;

	/* Initialize with default attribute */
	ret = pthread_attr_init(&attr);
	if(ret < 0) {
		syslog(LOG_ERR, "%s\n", strerror(errno));
		exit(errno);
	}

	/* create new thread */
	ret = pthread_create(&pid, &attr, __agent_start, mp);
	if(ret < 0) {
		syslog(LOG_ERR, "pthread_create");
		exit(errno);
	}

	pthread_attr_destroy(&attr);

	/* Thread specific settings */
	((fmd_module_t *)mp)->mod_pid = pid;

	return pid;
}


/**
 * Default init
 *
 * @param
 * @return
 */
fmd_module_t *
agent_init(agent_modops_t *mops, const char *path, fmd_t *pfmd)
{
	agent_module_t *mp;
	fmd_module_t *pmodule;
	ring_t *ring;
	struct list_head *pos;
	fmd_dispq_t *pdisp = &pfmd->fmd_dispq;

	fmd_debug;

	/* alloc module */
	mp = (agent_module_t *)malloc(sizeof(agent_module_t));
	if(mp == NULL) {
		syslog(LOG_ERR, "malloc");
		exit(EXIT_FAILURE);
	}
	memset(mp, 0, sizeof(agent_module_t));
	pmodule = (fmd_module_t *)mp;

	/* setup mops */
	mp->mops = mops;

	/* setup ring */
	ring = (ring_t *)malloc(sizeof(ring_t));
	assert(ring != NULL);
	ring_init(ring);
	mp->ring = ring;

	/* setup fmd */
	((fmd_module_t *)mp)->mod_fmd = pfmd;

	/* read config file */
	if(fmd_module_conf((fmd_module_t *)mp, path, AGENT_CONF) < 0) {
		free(ring);
		free(mp);
		return NULL;
	}

	/* subscribng events ... */
	list_add(&pmodule->list_dispq, &pdisp->list_agent);

	/* create thread */
	pthread_init(mp);

	return (fmd_module_t *)mp;
}
