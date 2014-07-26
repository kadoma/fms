/*
 * ring.h
 *
 *  Created on: Jan 16, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Ring buffer
 */

#ifndef _RING_H
#define _RING_H

#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define RING_SIZE 0x10

typedef struct ring {
	sem_t r_full;
	sem_t r_empty;
	pthread_mutex_t r_mutex;
	int front;
	int rear;
	int count;			/* current element number */
	void *r_item[RING_SIZE];
} ring_t;


static int inline
ring_init(ring_t *ring)
{
	sem_init(&ring->r_full, 0, 0);
	sem_init(&ring->r_empty, 0, RING_SIZE);
	pthread_mutex_init(&ring->r_mutex, NULL);
	ring->front = ring->rear = 0;
	ring->count = 0;
	memset(ring->r_item, 0, sizeof(ring->r_item));

	return 0;
}


static int inline
ring_empty(ring_t *ring)
{
	return (ring->count == 0) ? 1 : 0;
}


static int inline
ring_full(ring_t *ring)
{
	return (ring->count == RING_SIZE) ? 1 : 0;
}


static inline int
ring_add(ring_t *ring, void *p)
{
	if(ring_full(ring)) {
		syslog(LOG_ERR, "ring_add_full");
		exit(EXIT_FAILURE);
	}
#ifdef FMD_DEBUG
	if ( p == NULL )
		syslog(LOG_ERR, "Empty item add to ring");
#endif

	ring->r_item[ring->rear] = p;
	ring->rear = (ring->rear + 1) % RING_SIZE;
	ring->count++;

	return 0;
}


static inline void *
ring_del(ring_t *ring)
{
	int curpos = ring->front;

	if(ring_empty(ring)) {
		syslog(LOG_ERR, "ring_del");
		exit(EXIT_FAILURE);
	}

	ring->front = (ring->front + 1) % RING_SIZE;
	ring->count--;

	return ring->r_item[curpos];
}

#endif /* RING_H_ */
