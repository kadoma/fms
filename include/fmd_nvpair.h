
#ifndef __NVPAIR_H__
#define __NVPAIR_H__ 1

#include <stdint.h>
#include "wrap.h"
#include "list.h"

typedef struct _nvlist_{
    struct list_head nvlist;
	int    node_num;
    char name[32];
    char value[128];
    uint64_t dev_id;
    void *data;
}nvlist_t;

nvlist_t *nvlist_alloc(void);

struct list_head *nvlist_head_alloc(void);

void nvlist_free(nvlist_t *root);

void nvlist_add_nvlist(struct list_head *root, nvlist_t *new);

#endif  // fmd_nvparir.h
