/*
 * fmd_api.h
 *
 *  Created on: Jan 14, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	fmd_api.h
 */

#ifndef _FMD_API_H
#define _FMD_API_H

#include <pthread.h>
#include <fmd.h>
#include <fmd_event.h>
#include <fmd_module.h>
#include <fmd_case.h>

const char *plugindir();

fmd_module_t *
getmodule(fmd_t *, pthread_t);

unsigned 
fmd_timer(unsigned);

/* event */
uint64_t
fmd_event_getid(fmd_t *, const char *);

const char *
fmd_event_getclass(fmd_t *, uint64_t);

fmd_event_t *
fmd_create_ereport(fmd_t *, const char *, uint64_t, void *);

fmd_event_t *
fmd_create_fault(fmd_t *, fmd_case_t *);

fmd_event_t *
fmd_create_listevent(fmd_event_t *, int);

#endif
