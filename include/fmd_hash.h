
#ifndef __FMD_HASH_H__
#define __FMD_HASH_H__ 1

#include "wrap.h"
#include "list.h"

struct fmd_hash{
	char             *key;
	uint64_t         value;

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

	list_for_each(pos, &p->list) {
		pnew = list_entry(pos, struct fmd_hash, list);
		if(strcmp(pnew->key, key) == 0) 
			return -1;
	}

	pnew = (struct fmd_hash *)def_calloc(sizeof(struct fmd_hash), 1);

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

	if(key == NULL) 
		return (uint64_t)0;

	/* check for singleton */
	list_for_each(pos, &p->list) {
		phash = list_entry(pos, struct fmd_hash, list);
		if(strcmp(phash->key, key) == 0) 
			return phash->value;
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

	if(value == (uint64_t)0)
		return NULL;

	list_for_each(pos, &p->list) {
		phash = list_entry(pos, struct fmd_hash, list);
		if(phash->value == value)
			return phash->key;
	}

	return NULL;
}

#endif /* __FMD_HASH_H__ */
