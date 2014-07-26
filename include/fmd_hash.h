/*
 * fmd_hash.h
 *
 *  Created on: Feb 11, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_hash.h
 */

#ifndef FMD_HASH_H_
#define FMD_HASH_H_

#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <fmd_list.h>


/**
 * key & value are ALL NULL-terminated-strings.
 *
 */
struct fmd_hash {
	/* key & value */
	char *key;
	uint64_t value;

	struct list_head list;
};


static inline void
INIT_HASH_HEAD(struct fmd_hash *p)
{
	p->key = NULL;
	p->value = (uint64_t)0;
	INIT_LIST_HEAD(&p->list);
}


/**
 * put
 *
 * @return
 * 0: insert success
 * !0:insert unsuccessful, maybe key conflicts.
 */
static inline int
hash_put(struct fmd_hash *p, char *key, uint64_t value)
{
	struct fmd_hash *pnew = NULL;
	struct list_head *pos;

	/* check for singleton */
	list_for_each(pos, &p->list) {
		pnew = list_entry(pos, struct fmd_hash, list);
		if(strcmp(pnew->key, key) == 0) return -1;
	}

	pnew = (struct fmd_hash *)
		malloc(sizeof(struct fmd_hash));
	assert(pnew != NULL);
	pnew->key = key;
	pnew->value = value;
	list_add(&pnew->list, &p->list);
	return 0;
}


/**
 * get
 *
 */
static inline uint64_t
hash_get(struct fmd_hash *p, char *key)
{
	struct fmd_hash *phash = NULL;
	struct list_head *pos;

	if(key == NULL) return (uint64_t)0;

	/* check for singleton */
	list_for_each(pos, &p->list) {
		phash = list_entry(pos, struct fmd_hash, list);
		if(strcmp(phash->key, key) == 0) return phash->value;
	}

	return (uint64_t)0;
}

/**
 * get key
 *
 */
static inline char *
hash_get_key(struct fmd_hash *p, uint64_t value)
{
	struct fmd_hash *phash = NULL;
	struct list_head *pos;

	if(value == (uint64_t)0) return NULL;

	/* check for singleton */
	list_for_each(pos, &p->list) {
		phash = list_entry(pos, struct fmd_hash, list);
		if(phash->value == value) return phash->key;
	}

	return NULL;
}

#endif /* FMD_HASH_H_ */
