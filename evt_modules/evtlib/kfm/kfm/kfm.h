#ifndef _KFM_H
#define _KFM_H

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <net/sock.h>
#include <linux/sysctl.h>	/* for ctl_table */

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#endif

#include <asm/types.h>		/* For __uXX types */


#define KFM_MODULE_DESC				"KFM update 20150825-17:17"
#define KFM_VERSION_CODE			0x030101
#define NVERSION(version)            	\
		(version >> 16) & 0xFF,         \
		(version >> 8) & 0xFF,          \
		version & 0xFF


struct kfm_cache_u {
	__u32 cpu;
	__u32 status;	/* 1: disable, 0: enable */
};

struct kfm_mempage_u {
	__u64 addr;		/* memory physical address */
	__u64 pfn;		/* page offset number */
	__u64 flags;	/* page flags */
};

/*
 *
 * KFM Generic Netlink interface definitions
 *
 */

/* Generic Netlink family info */

#define KFM_GENL_NAME		"KFM"
#define KFM_GENL_VERSION	0x1

/* kfm multicast groups */
enum kfm_multicast_groups {
	KFM_MCGRP_CPUMEM,
#define KFM_MCGRP_CPUMEM	KFM_MCGRP_CPUMEM
	KFM_MCGRP_DISK,
#define KFM_MCGRP_DISK		KFM_MCGRP_DISK
	KFM_MCGRP_NET,
#define KFM_MCGRP_NET		KFM_MCGRP_NET

};

/* Generic Netlink command attributes */
enum {
	KFM_CMD_UNSPEC = 0,
	
	KFM_CMD_GET_INFO,			/* get general KFM info */
	KFM_CMD_GET_INFO_REPLY,		/* only used in GET_INFO reply */
	
	/* for cache, begin */
	KFM_CMD_CACHE_FLUSH,			/* flush cache on per cpu */
	KFM_CMD_ALL_CACHE_FLUSH,		/* flush cache on all cpus */
	KFM_CMD_CACHE_DISABLE,			/* disable cache on per cpu */
	KFM_CMD_ALL_CACHE_DISABLE,		/* disable cache on all cpus */
	KFM_CMD_CACHE_ENABLE,			/* enable cache on per cpu */
	KFM_CMD_ALL_CACHE_ENABLE,		/* enable cache on all cpus */
	KFM_CMD_GET_CACHE_INFO,			/* get cache info */
	KFM_CMD_GET_CACHE_INFO_REPLY,	/* get cache info reply */
	/* for cache, end */

	/* for memory, begin */
	KFM_CMD_MEMPAGE_ONLINE,			/* memory page online */
	KFM_CMD_GET_MEMPAGE_INFO,		/* get memory page info */
	KFM_CMD_GET_MEMPAGE_INFO_REPLY,	/* get memory page info reply */
	/* for memory, end */
	
	__KFM_CMD_MAX,
};

#define KFM_CMD_MAX (__KFM_CMD_MAX - 1)

/*
 * Attributes used to describe a cache
 *
 */
enum {
	KFM_CACHE_ATTR_UNSPEC = 0,
	KFM_CACHE_ATTR_CPU,			/* cahce belongs to cpu */
	KFM_CACHE_ATTR_STATUS, 		/* cache status */

	__KFM_CACHE_ATTR_MAX,
};

#define KFM_CACHE_ATTR_MAX (__KFM_CACHE_ATTR_MAX - 1)

/* Attributes used in response to KFM_CMD_GET_INFO command */
enum {
	KFM_INFO_ATTR_UNSPEC = 0,
	KFM_INFO_ATTR_VERSION,			/* KFM version number */
	__KFM_INFO_ATTR_MAX,
};

#define KFM_INFO_ATTR_MAX (__KFM_INFO_ATTR_MAX - 1)

enum {
	KFM_MEMPAGE_ATTR_UNSPEC = 0,
	KFM_MEMPAGE_ATTR_ADDR,			/* memory physical address */
	KFM_MEMPAGE_ATTR_PFN,			/* page offset number */
	KFM_MEMPAGE_ATTR_FLAGS,			/* memory page flags */
	__KFM_MEMPAGE_ATTR_MAX,
};

#define KFM_MEMPAGE_ATTR_MAX (__KFM_MEMPAGE_ATTR_MAX - 1)


#ifdef __KERNEL__
/*
*   For log.
*/
#ifndef CONFIG_KFM_DEBUG 
#define CONFIG_KFM_DEBUG
#endif

#ifdef CONFIG_KFM_DEBUG

extern int kfm_get_debug_level(void);

/*
  * how do you just show only one debug level. 
  * I define kfm log level 1 -255, use Hex.
  * eg. 0x005 show log  level <= 5 , 0x105 just show log level 5.
  */
#define DL_MASK_ONLY	0x0f00
#define DL_MASK_LEVEL	0x00ff

#define EnterFunction(level)							\
    do {												\
    	int dl = kfm_get_debug_level() & DL_MASK_LEVEL; \
    	int only = kfm_get_debug_level() & DL_MASK_ONLY; \
    	if ((only && (dl == level)) || (!only && (dl >= level)))	\
		    printk(KERN_DEBUG "Enter: %s, %s line %i\n",	\
			   __FUNCTION__, __FILE__, __LINE__);		\
    } while (0)

#define LeaveFunction(level)							\
    do {												\
	    int dl = kfm_get_debug_level() & DL_MASK_LEVEL; \
    	int only = kfm_get_debug_level() & DL_MASK_ONLY; \
    	if ((only && (dl == level)) || (!only && (dl >= level)))	\
			printk(KERN_DEBUG "Leave: %s, %s line %i\n",	\
			       __FUNCTION__, __FILE__, __LINE__);		\
    } while (0)


#define KFM_DBG(level, msg...)			       				\
    do {						                         	\
	    int dl = kfm_get_debug_level() & DL_MASK_LEVEL; 	\
    	int only = kfm_get_debug_level() & DL_MASK_ONLY; 	\
    	if ((only && (dl == level)) || (!only && (dl >= level))) \
		    printk(KERN_DEBUG "KFM: " msg);			\
    } while (0)

/* switch off assertions (if not already off) */
#define assert(expr)						\
	if(!(expr)) {						\
		printk(KERN_ERR "KFM: Assertion failed! %s,%s,%s,line=%d\n",	\
		#expr,__FILE__,__FUNCTION__,__LINE__);		\
	}
	
#else /* NO DEBUGGING at ALL */
#define EnterFunction(level)   do {} while (0)
#define LeaveFunction(level)   do {} while (0)
#define KFM_DBG(level, msg...)  do {} while (0)
#define assert(expr) do {} while (0)
#endif

#define KFM_ERR(msg...)   printk(KERN_ERR "KFM: " msg)
#define KFM_INFO(msg...) printk(KERN_INFO "KFM: " msg)
#define KFM_WARNING(msg...) printk(KERN_WARNING "KFM: " msg)

#define KFM_ERR_RL(msg...)  \
    do {						\
	    if (net_ratelimit())			\
		    printk(KERN_ERR "KFM: " msg);	\
    } while (0)

#define KFM_INFO_RL(msg...)  \
    do {						\
	    if (net_ratelimit())			\
		    printk(KERN_INFO "KFM: " msg);	\
    } while (0)

#define KFM_WARNING_RL(msg...)  \
    do {						\
	    if (net_ratelimit())			\
		    printk(KERN_WARNING "KFM: " msg);	\
    } while (0)


/* kfm_core.c */
extern int kfm_use_count_inc(void);
extern void kfm_use_count_dec(void);
extern void kfm_post_evt_thread_exit(void);


/* kfm_cache.c */
extern void kfm_cache_flush_on_cpu(int cpu);
extern void kfm_cache_flush_all(void);
extern void kfm_cache_disable_on_cpu(int cpu);
extern void kfm_cache_disable_all(void);
extern void kfm_cache_enable_on_cpu(int cpu);
extern void kfm_cache_enable_all(void);
extern int kfm_cache_status_on_cpu(int cpu);


/* kfm_mem.c */
extern int kfm_get_mempage_info(u64 addr, struct kfm_mempage_u *umempage);
extern int kfm_mempage_online(u64 addr);


/* kfm_ctl.c */
extern int sysctl_kfm_unload;
extern int kfm_control_start(void);
extern void kfm_control_stop(void);

#endif	/* __KERNEL__ */

#endif /* _KFM_H */
