
#ifndef __FMD_MODULE_H__
#define __FMD_MODULE_H__ 1

#include <time.h>
#include <pthread.h>

#include "wrap.h"
#include "list.h"
#include "fmd_queue.h"
#include "fmd_ring.h"
#include "fmd.h"

/* conf file */
#define EVTSRC_CONF	0
#define AGENT_CONF	1

typedef struct fmd_conf{
	time_t cf_interval;	/* timer intervals */
	float cf_ver;		/* a copy of module version string */
	struct list_head *list_eclass;
}fmd_conf_t;

struct subitem {
	char *si_eclass;	/*event class name */

	/**
	 * subscribed events' list of a module
	 *
	 * link to fmd_module_t.mod_subclasslist
	 */
	struct list_head si_list;
};

typedef struct _fmd_modops_{

    void              *(*fmd_module_init)(const char *path, fmd_t *pfmd);
    void              *(*fmd_module_finit)(fmd_t *pfmd);

}fmd_modops_t;

typedef struct _fmd_module_{
    fmd_t            *p_fmd;
	char             *mod_name;
	int				  mod_vers;
	int               mod_interval;
	char             *mod_path;
    char             *module_name;
    fmd_modops_t     p_ops;

	struct list_head list_queue;
	struct list_head list_fmd;

	/* head of event class list */
	struct list_head list_eclass;
	pthread_t        mod_pid;
	
}fmd_module_t;

int fmd_module_load(fmd_t *p_fmd);

#endif  // fmd_module.h
