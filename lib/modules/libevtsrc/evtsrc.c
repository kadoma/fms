/*
 * evtsrc.c
 *
 *  Created on: Jan 14, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	EventSource Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <fmd.h>
#include <nvpair.h>
#include <protocol.h>
#include <fmd_module.h>
#include <fmd_evtsrc.h>

/**
 * evt_handle
 * 1. extract nvlist from @head
 * 2. create fmd_event_t *
 * 3. xprt the new created fmd_event_t *
 */
static void
evt_handle(evtsrc_module_t *emp, struct list_head *head)
{
	struct list_head *pos = NULL;
	nvlist_t *nvl = NULL;
//	static fmd_event_t *evt = NULL;
//	fmd_event_t *evt = NULL;
	char *eclass = NULL;
    
	fmd_t *pfmd = ((fmd_module_t *)emp)->mod_fmd;

	if (list_empty(head)) {
		free(head);
		return;
	}

	list_for_each(pos, head) {
		nvl = list_entry(pos, nvlist_t, list);

		eclass = nvl->value;
		printf("##DEBUG##: FM_CLASS: %s\n", eclass);

#if 0
		struct fmd_event *p = (struct fmd_event *)fmd_create_ereport(pfmd, eclass, NULL, nvl);
		printf("[%p]\n", p);
#if 0
		evt = fmd_create_ereport(pfmd, eclass, NULL, nvl);
		printf("##DEBUG##: Attention here!\n");
		printf("##DEBUG##: evt = %p\n", evt);
		printf("##DEBUG##: evt->ev_class = %s\n", evt->ev_class);
#endif

		if(evt == NULL)
			continue;

		/* dispatch protocol event */
		fmd_xprt_dispatch(emp, evt);
#endif
		if (eclass[0] == '\0')
			continue;

		fmd_xprt_dispatch(emp, fmd_create_ereport(pfmd, eclass, NULL, nvl));
	}
}


/**
 * TODO ASYNC
 *
 * @param
 * @return
 */
void *
do_evtsrc()
{
	struct list_head *rawevt;
	fmd_event_t *evt;
	evtsrc_module_t *mp;
	evtsrc_modops_t *mops;

	/* Begins to callback */
	mp = pthread_getspecific(t_module);
	mops = mp->mops;

	fmd_debug;

	/* get raw event */
	rawevt = mops->evt_probe(mp);
	
	if(mops->evt_create == NULL) {
		/* build protocol event & xprt */
		if (rawevt != NULL)
			evt_handle(mp, rawevt);
	} else {
		evt = mops->evt_create(mp, NULL);
		if(evt != NULL) {
			/* dispatch protocol event */
			fmd_xprt_dispatch(mp, evt);
		}
	}

	return NULL;
}


/**
 * @param
 * @return
 */
static void *
__evtsrc_start(void *arg)
{
	sigset_t fullset, oldset;
	evtsrc_module_t *emp;
	fmd_timer_t *timer;
	evtsrc_modops_t *mops;

	/* set TSD */
	pthread_setspecific(t_module, arg);

	/* signal */
	sigfillset(&fullset);
	sigdelset(&fullset, SIGKILL);
	sigdelset(&fullset, SIGSTOP);
	pthread_sigmask(SIG_BLOCK, &fullset, &oldset);

	/* container */
	emp = (evtsrc_module_t *)pthread_getspecific(t_module);
	timer = &emp->timer;

	/* Thread starts working... */
	while(1) {
		fmd_debug;
		pthread_mutex_lock(&timer->mutex);

		fmd_debug;

		pthread_cond_wait(&timer->cv, &timer->mutex);
		fmd_debug;
		pthread_mutex_unlock(&timer->mutex);

		do_evtsrc();
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
	evtsrc_module_t *mp = (evtsrc_module_t *)arg;

	/* Initialize with default attribute */
	ret = pthread_attr_init(&attr);
	if(ret < 0) {
		syslog(LOG_ERR, "%s\n", strerror(errno));
		exit(errno);
	}

	/* create new thread */
	ret = pthread_create(&pid, &attr, __evtsrc_start, mp);
	if(ret < 0) {
		syslog(LOG_ERR, "pthread_create");
		exit(errno);
	}

	pthread_attr_destroy(&attr);

	/* Thread specific settings */
	mp->timer.tid = pid;
	((fmd_module_t *)mp)->mod_pid = pid;

	return pid;
}


/**
 * @param
 * @return
 */
static int
timer_init(evtsrc_module_t *emp, fmd_t *pfmd)
{
	fmd_module_t *mp = (fmd_module_t *)emp;
	time_t tv = mp->mod_interval;
	fmd_timer_t *ptimer = &emp->timer;

	/* FIXME: There is a big bug, Ben!
	   mp->mod_interval is always 0 for each evtsrc module.
	ptimer->interval = mp->mod_interval; */
	ptimer->interval = 1;
	ptimer->ticks = 0;
	pthread_cond_init(&ptimer->cv, NULL);
	pthread_mutex_init(&ptimer->mutex, NULL);

	/* Add it to global list for further scheduling */
	list_add(&ptimer->list, &pfmd->fmd_timer);

	return 0;
}


/**
 * Evtsrc modules's initialization
 *
 * @param
 * @return
 */
fmd_module_t *
evtsrc_init(evtsrc_modops_t *mops, const char* path, fmd_t *pfmd)
{
	evtsrc_module_t *mp;

	fmd_debug;

	/* alloc module */
	mp = (evtsrc_module_t *)malloc(sizeof(evtsrc_module_t));
	if(mp == NULL) {
		syslog(LOG_ERR, "malloc");
		exit(EXIT_FAILURE);
	}
	memset(mp, 0, sizeof(evtsrc_module_t));

	/* setup mops */
	mp->mops = mops;

	/* setup fmd */
	((fmd_module_t *)mp)->mod_fmd = pfmd;

	/* read config file */
	if(fmd_module_conf((fmd_module_t *)mp, path, EVTSRC_CONF) < 0) {
		free(mp);
		return NULL;
	}

	/* timer */
	timer_init(mp, pfmd);

	/* create thread */
	pthread_init(mp);

	return (fmd_module_t *)mp;
}
