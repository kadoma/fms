/*
 * fmd_module.h
 *
 *  Created on: Jan 14, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Module
 */

#ifndef	_FMD_MODULE_H
#define	_FMD_MODULE_H

#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <fmd.h>
#include <fmd_list.h>

struct fmd_modops;
struct fmd_module;

struct subitem {
	char *si_eclass;	/*event class name */

	/**
	 * subscribed events' list of a module
	 *
	 * link to fmd_module_t.mod_subclasslist
	 */
	struct list_head si_list;
};

typedef struct fmd_modops {
	struct fmd_module * (*fmd_init)(const char *path, fmd_t *pfmd);
	void 				(*fmd_fini)(struct fmd_module *);
} fmd_modops_t;

typedef struct fmd_module {
	char *mod_name;			/* basename of module (ro) */
	char *mod_path;			/* full pathname of module file (ro) */
	float mod_vers;			/* a copy of module version string */
	int	  mod_interval;
	pthread_t mod_pid;	/* thread associated with module (ro) */
	const fmd_modops_t *mod_ops;	/* module class ops vector (ro) */
	fmd_t *mod_fmd;	/* global section */

	/* link to fmd.fmd_module */
	struct list_head list_fmd;

	/* link to fmd.fmd_dispq.dq_list */
	struct list_head list_dispq;

	/* head of event class list */
	struct list_head list_eclass;
} fmd_module_t;

extern fmd_module_lock(pthread_mutex_t *mutex);
extern fmd_module_unlock(pthread_mutex_t *mutex);

#endif	/* _FMD_MODULE_H */
