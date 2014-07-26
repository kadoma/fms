/*
 * fmd_dispq.h
 *
 *  Created on: Jan 16, 2010
 *      Author: Inspur OS Team
 *
 *  Description:
 *  	Dispatch Queue
 */

#ifndef _FMD_DISPQ_H
#define _FMD_DISPQ_H

#include <pthread.h>
#include <fmd_ring.h>
#include <fmd_list.h>

typedef struct fmd_dispq {
	pthread_t dq_pid;	/* dispq's pid */
	pthread_mutex_t dq_mutex;
	ring_t dq_ring;	/* buffer ring */

	/* list of agent_module_t */
	struct list_head list_agent;
} fmd_dispq_t;


static int inline
fmd_dispq_init(fmd_dispq_t *p)
{
	p->dq_pid = 0;
	pthread_mutex_init(&p->dq_mutex, NULL);
	ring_init(&p->dq_ring);
	INIT_LIST_HEAD(&p->list_agent);

	return 0;
}

#endif /* FMD_DISPQ_H_ */
