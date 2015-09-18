#ifndef __FMD_QUEUE_H__
#define __FMD_QUEUE_H__ 1

#include <pthread.h>
#include "fmd_ring.h"
#include "list.h"

typedef struct _fmd_queue_{
    pthread_t           queue_pid;
    pthread_mutex_t     queue_lock;
    ring_t              queue_ring;
    struct list_head    list_agent;
}fmd_queue_t;


static int
fmd_queue_init(fmd_queue_t *p)
{
    p->queue_pid = 0;
    pthread_mutex_init(&p->queue_lock, NULL);
    ring_init(&p->queue_ring);

	// list all agent so.
    INIT_LIST_HEAD(&p->list_agent);

	return 0;
}

#endif // fmd_queue.h
