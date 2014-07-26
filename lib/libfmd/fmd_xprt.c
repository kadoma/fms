/*
 * fmd_xprt.c
 *
 *  Created on: Feb 23, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_xprt.c
 */

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include <fmd.h>
#include <fmd_ring.h>
#include <fmd_event.h>
#include <fmd_xprt.h>
#include <fmd_module.h>

/**
 * fmd_xprt_put
 *
 * @param
 * @return
 */
int
fmd_xprt_put(fmd_t *pfmd, fmd_event_t *pevt)
{
	fmd_dispq_t *dispq = &pfmd->fmd_dispq;

	ring_t *ring = &dispq->dq_ring;

	fmd_debug;
	sem_wait(&ring->r_empty);
	fmd_debug;
	pthread_mutex_lock(&ring->r_mutex);
	fmd_debug;
	ring_add(ring, pevt);
	fmd_debug;
	pthread_mutex_unlock(&ring->r_mutex);
	fmd_debug;
	sem_post(&ring->r_full);
	fmd_debug;
}


/**
 * fmd_xprt_dispatch
 *
 * @param
 * @return
 */
int
fmd_xprt_dispatch(fmd_module_t *emp, fmd_event_t *ep)
{
	fmd_debug;
	fmd_t *pfmd = ((fmd_module_t *)emp)->mod_fmd;
	return fmd_xprt_put(pfmd, ep);
}
