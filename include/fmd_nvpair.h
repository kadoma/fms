
#ifndef __NVPAIR_H__
#define __NVPAIR_H__ 1

#include <stdint.h>
#include "wrap.h"
#include "list.h"

typedef struct _nvlist_{
    struct list_head nvlist;
    char name[32];  /* cpu, memory, disk, etc */ 
    uint64_t dev_id; /*cpu: 0, 1, 2..; memory: todo; disk: uuid */
    char value[128]; /* ereport.cpu.xxx */ 
    uint64_t evt_id; /* event id */
    void *data; /* private data */
}nvlist_t;

nvlist_t *nvlist_alloc(void);

struct list_head *nvlist_head_alloc(void);

void nvlist_free(nvlist_t *root);

void nvlist_add_nvlist(struct list_head *root, nvlist_t *new);

#endif  // fmd_nvparir.h
