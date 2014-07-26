/*
 * nvpair.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Dec 22, 2010
 *      Author: Inspur OS Team
 *  
 * Description: 
 *      nvpair.c
 */

#include <assert.h>
#include "nvpair.h"

/*
 * nvpair.c - Provides userland interfaces for manipulating
 *	name-value pairs.
 */

/*
 * nvlist_alloc - Allocate nvlist.
 */
nvlist_t *
nvlist_alloc(void)
{
	nvlist_t *nvlp = NULL;

	/* malloc */
	nvlp = (nvlist_t *)malloc(sizeof (nvlist_t));
	assert(nvlp != NULL);
	memset(nvlp, 0, sizeof (nvlist_t));

	nvlp->data = NULL;
	INIT_LIST_HEAD(&nvlp->list);

	return nvlp;
}

/*
 * nvlist_head_alloc - Allocate head of nvlist.
 */
struct list_head *
nvlist_head_alloc(void)
{
	struct list_head *head =
		(struct list_head *)malloc(sizeof(struct list_head));
	assert(head != NULL);
	INIT_LIST_HEAD(head);

	return head;
}


/*
 * nvlist_free - free an unpacked nvlist
 */
void
nvlist_free(nvlist_t *nvl)
{
	free(nvl);
}

void
nvlist_add_nvlist(struct list_head *head, nvlist_t *nvl)
{
	list_add(&nvl->list, head);
}

int
nvlist_lookup_nvlist(nvlist_t *nvl1, const char *a, nvlist_t *nvl2)
{
	return 0;
}
