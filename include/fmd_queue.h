#ifndef __FMD_QUEUE_H__
#define __FMD_QUEUE_H__ 1

#include <pthread.h>
#include "fmd_ring.h"
#include "list.h"

typedef struct _list_repaired_{
    sem_t              evt_repaired_sem;
    pthread_mutex_t    evt_repaired_lock;
    struct list_head    list_repaired_head;

}list_repaired;


typedef struct _fmd_queue_{
    pthread_t           queue_pid;
    pthread_mutex_t     queue_lock;
    ring_t              queue_ring;
	list_repaired       queue_repaired_list;

    struct list_head    list_agent;
}fmd_queue_t;


static int
fmd_queue_init(fmd_queue_t *p)
{
    p->queue_pid = 0;
    pthread_mutex_init(&p->queue_lock, NULL);
    ring_init(&p->queue_ring);
	
	/* list repaired init */
	list_repaired  *plist_repaired = &p->queue_repaired_list;
    sem_init(&plist_repaired->evt_repaired_sem, 0, 0);   
    pthread_mutex_init(&plist_repaired->evt_repaired_lock, NULL);
	INIT_LIST_HEAD(&plist_repaired->list_repaired_head);

	// list all agent so.
    INIT_LIST_HEAD(&p->list_agent);

	return 0;
}

#endif // fmd_queue.h
