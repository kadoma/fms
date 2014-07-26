/*
 * nvpair.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Dec 22, 2010
 *      Author: Inspur OS Team
 *  
 * Description: 
 *      nvpair.h
 */

#ifndef _NVPAIR_H_
#define _NVPAIR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fmd_list.h>

typedef struct nvlist {
	char name[10];
	char value[100];
	uint64_t rscid;
	void *data;
	struct list_head list;
} nvlist_t;

/* list management */
nvlist_t *nvlist_alloc(void);

struct list_head *nvlist_head_alloc(void);

void nvlist_free(nvlist_t *);

void nvlist_add_nvlist(struct list_head *, nvlist_t *);

int nvlist_lookup_nvlist(nvlist_t *, const char *, nvlist_t *);

#endif	/* _NVPAIR_H */
