#ifndef __EVT_AGENT_H__
#define __EVT_AGENT_H__ 1

#include "wrap.h"
#include "fmd_nvpair.h"
#include "fmd_module.h"
#include "fmd_event.h"
#include "fmd.h"

typedef struct agent_modops {
	fmd_modops_t  modops;	/* base module operations */
	fmd_event_t *(*evt_handle)(fmd_t *, fmd_event_t *);
}agent_modops_t;

typedef struct agent_module {
	fmd_module_t 	module;
	agent_modops_t *mops;
	ring_t *		ring;
}agent_module_t;

fmd_module_t *agent_init(agent_modops_t *agent_module, char *so_path, fmd_t *p_fmd);

#endif  // evt_agent.h
