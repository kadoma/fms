#ifndef __FMD_RING_H__
#define __FMD_RING_H__ 1

#include <semaphore.h>
#include <syslog.h>

#include "wrap.h"


#define RING_SIZE 128

typedef struct _ring_{
    sem_t              ring_contain;
    sem_t              ring_vaild;
    pthread_mutex_t    ring_lock;
    int                front;
    int                rear;
    int                count;
    void               *ring_item[RING_SIZE];
}ring_t;

static inline int
ring_init(ring_t *ring)
{
    // init all front rear count item.
    memset(ring, 0, sizeof(ring_t));
    sem_init(&ring->ring_contain, 0, 0);   //now contain nums 
    sem_init(&ring->ring_vaild, 0, RING_SIZE); // now can vaild nums
    pthread_mutex_init(&ring->ring_lock, NULL);
    return 0;
}

static inline int
ring_stat(ring_t *ring)
{
	return ring->count;
}

static inline int
ring_add(ring_t *ring, void *p)
{
    if(ring_stat(ring) == RING_SIZE){
        return -1;
    }

    ring->ring_item[ring->rear] = p;
    ring->rear = (ring->rear + 1) % RING_SIZE;
    ring->count ++;

    return 0;
}

static inline void *
ring_del(ring_t *ring)
{
    if(ring_stat(ring) == 0)
	{
        return NULL;
    }

    int cur_pos = ring->front;
    ring->front = (ring->front + 1) % RING_SIZE;
    ring->count --;

    return ring->ring_item[cur_pos];
}

#endif // fmd_ring.h
