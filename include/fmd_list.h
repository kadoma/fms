/*
 * fmd_list.h
 *
 *  Created on: Jan 14, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	list operation collections
 */

#ifndef _FMD_LIST_H
#define _FMD_LIST_H

struct list_head {
	struct list_head *prev, *next;
};

#define LIST_HEAD(name) \
	struct list_head name = {&name, &name};


#define offset(type, member) ((size_t)&((type *)0)->member)

#define container_of(ptr, type, member) ({ \
	const typeof(((type *)0)->member) *mptr = (ptr); \
	(type *)((char *)mptr - offset(type, member)); \
})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

static inline void
INIT_LIST_HEAD(struct list_head *list)
{
	list->prev = list;
	list->next = list;
}

static inline void
__list_add(struct list_head *new,
			struct list_head *prev,
			struct list_head *next)
{
	prev->next = new;
	new->prev = prev;
	new->next = next;
	next->prev = new;
}

static inline void
list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void
__list_del(struct list_head *prev, struct list_head *next)
{
	prev->next = next;
	next->prev = prev;
}

static inline void
list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

#define list_for_each(pos, head) \
	for(pos = (head)->next; pos != (head); pos = pos->next)

static inline int
list_empty(struct list_head *head)
{
	return head->next == head;
}

static inline struct list_head *
list_next(struct list_head *head)
{
	return head->next;
}

#endif
