/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	kfm_cache.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-17
 * Description:	
 *				Kernel Fault Management cache.
 *
 */

#include <linux/smp.h>

#include "kfm.h"

static int g_cache_status;

/* flush a cpu cache */
void 
kfm_cache_flush_on_cpu(int cpu)
{
	EnterFunction(6);
	
	wbinvd_on_cpu(cpu);

	KFM_DBG(6, "flush cpu %d cache", cpu);
	
	LeaveFunction(6);
}

/* flush all cpus cache */
void 
kfm_cache_flush_all(void)
{
	EnterFunction(6);
	
	wbinvd_on_all_cpus();

	LeaveFunction(6);
}

static inline void
__kfm_disable_caches(void *dummy)
{
	write_cr0(read_cr0() | X86_CR0_CD);
	wbinvd();
}
 
static inline void
__kfm_enable_caches(void *dummy)
{
	write_cr0(read_cr0() & ~X86_CR0_CD);
}

static inline void
__kfm_caches_status(void *dummy)
{
	g_cache_status = read_cr0() & X86_CR0_CD;
}

/* disable a cpu cache */
void 
kfm_cache_disable_on_cpu(int cpu) 
{
	smp_call_function_single(cpu, __kfm_disable_caches, NULL, 1);

	KFM_DBG(6, "disable cpu %d cache", cpu);
}

/* disable all cpus cache */
void 
kfm_cache_disable_all(void)
{
	on_each_cpu(__kfm_disable_caches, NULL, 1);

	KFM_DBG(6, "disable all caches");
}

/* enable a cpu cache */
void 
kfm_cache_enable_on_cpu(int cpu) 
{
	smp_call_function_single(cpu, __kfm_enable_caches, NULL, 1);

	KFM_DBG(6, "enable cpu %d cache", cpu);
}

/* enable all cpus cache */
void 
kfm_cache_enable_all(void)
{
	on_each_cpu(__kfm_enable_caches, NULL, 1);

	KFM_DBG(6, "enable all caches");
}

/*
 * return:
 * 		0: enable
 *		1: disable
 *		-1: error
 */
int 
kfm_cache_status_on_cpu(int cpu)
{
	int ret = 0;
	
	g_cache_status = 0;
	if((ret = smp_call_function_single(cpu, __kfm_caches_status, NULL, 1)))
		return ret;

	KFM_DBG(6, "get cpu %d cache status", cpu);

	return g_cache_status;
}
