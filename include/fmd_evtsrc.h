/*
 * fmd_evtsrc.h
 *
 *  Created on: Jan 14, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	EventSource Module
 */

#ifndef _FMD_EVTSRC_H
#define _FMD_EVTSRC_H

#include <nvpair.h>
#include <fmd_module.h>
#include <fmd_event.h>

struct evtsrc_module;


typedef struct evtsrc_modops {
	fmd_modops_t  modops;	/* base module operations */
	struct list_head * (*evt_probe)(struct evtsrc_module *);
	
	/* obsolete interface */
	fmd_event_t * (*evt_create)(struct evtsrc_module *, nvlist_t *);
} evtsrc_modops_t;


typedef struct evtsrc_module {
	fmd_module_t module;	/* base module properties */
	fmd_timer_t timer;	/* attached timer */
	evtsrc_modops_t *mops;
} evtsrc_module_t;


/*--------------FUCTIONS--------------*/
/**
 * Evtsrc modules's initialization
 *
 * @param
 * @return
 */
fmd_module_t *
evtsrc_init(evtsrc_modops_t *, const char *, fmd_t *);

#endif /* _FMD_EVTSRC_H */
