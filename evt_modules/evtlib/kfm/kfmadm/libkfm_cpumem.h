#ifndef __LIBKFM_CPUMEM_H__
#define __LIBKFM_CPUMEM_H__

#undef __KERNEL__

#include "kfm_nl.h"

extern struct nla_policy kfm_cache_nla_policy[];
extern struct nla_policy kfm_mempage_nla_policy[];

/* for cache */
extern int kfm_cache_disable(struct kfm_cache_u *cache);
extern int kfm_cache_disable_all(void);
extern int kfm_cache_enable(struct kfm_cache_u *cache);
extern int kfm_cache_enable_all(void);
extern int kfm_cache_flush(struct kfm_cache_u *cache);
extern int kfm_cache_flush_all(void);
extern int kfm_cache_get_info(struct kfm_cache_u *cache);

/* for mem */
extern int kfm_mempage_get_info(struct kfm_mempage_u *mempage);
extern int kfm_mempage_online(struct kfm_mempage_u *mempage);


#endif