/*
 * agent.h
 *
 *  Created on: Jan 18, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent module
 */

#ifndef _FMD_AGENT_H
#define _FMD_AGENT_H

#include <fmd_module.h>
#include <fmd_event.h>

struct agent_module;

typedef struct agent_modops {
	fmd_modops_t  modops;	/* base module operations */

	/**
	 *	handle the fault.* event
	 *
	 *	@result:
	 *		list.* event
	 */
	fmd_event_t * (*evt_handle)(fmd_t *, fmd_event_t *);
} agent_modops_t;

typedef struct agent_module {
	fmd_module_t 	module;
	agent_modops_t *mops;
	ring_t *		ring;
} agent_module_t;


/*------------FUNCTION-------------*/
/**
* Default init
*
* @param
* @return
*/
fmd_module_t *
agent_init(agent_modops_t *, const char *, fmd_t *);

#endif /* _FMD_AGENT_H */
