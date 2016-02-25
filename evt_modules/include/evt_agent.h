#ifndef __EVT_AGENT_H__
#define __EVT_AGENT_H__ 1

#include "wrap.h"
#include "fmd_nvpair.h"
#include "fmd_module.h"
#include "fmd_event.h"
#include "fmd.h"

#define AGENT_KEY_EVT_HANDLE_MODE	"event_handle_mode"

typedef struct agent_modconfs {
	/* 
	 * manual, auto, user-define etc. 
	 * default: EVENT_HANDLE_MODE_AUTO
	 */
	int evt_handle_mode;

	/* add other configuration parameter */
}agent_modconfs_t;

typedef struct agent_modops {
	fmd_modops_t  modops;	/* base module operations */
	fmd_event_t *(*evt_handle)(fmd_t *, fmd_event_t *);
}agent_modops_t;

typedef struct agent_module {
	fmd_module_t 	module;
	agent_modops_t *mops;
	agent_modconfs_t mconfs;
	ring_t *		ring;
}agent_module_t;

fmd_module_t *agent_init(agent_modops_t *agent_module, char *so_path, fmd_t *p_fmd);
int agent_module_update_conf(fmd_t *fmd, char *mod_name, 
	char *conf_path, char *direct, char *value);

#endif  // evt_agent.h
