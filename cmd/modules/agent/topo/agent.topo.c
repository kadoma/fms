/*
 * agent.topo.c
 *
 *  Created on: Nov 15, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for topo module
 */

#include <stdio.h>
#include <dlfcn.h>
#include <limits.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>

/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
topo_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;
	char path[PATH_MAX], *error;
	void *handle;
	int (*update_topo)(fmd_t *), ret;

	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", BASE_DIR, "libfmd_topo.so");

	handle = dlopen(path, RTLD_LAZY);
	if (handle == NULL) {
		syslog(LOG_ERR, "dlopen");
		exit(-1);
	}

	dlerror();
	update_topo = dlsym(handle, "_update_topo");
	if ((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}

	/* update topology */
	if (strstr(e->ev_class, "fault.topo") != NULL) {
		if ((ret = (*update_topo)(pfmd)) == 0) {
			dlclose(handle);
			return fmd_create_listevent(e, LIST_ISOLATED_SUCCESS);
		} else {
			dlclose(handle);
			return fmd_create_listevent(e, LIST_ISOLATED_FAILED);
		}
	}

	dlclose(handle);
	return fmd_create_listevent(e, LIST_LOG);
}


static agent_modops_t topo_mops = {
	.evt_handle = topo_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&topo_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
