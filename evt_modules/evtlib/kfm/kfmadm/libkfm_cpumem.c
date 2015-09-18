/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	libkfm_cpumem.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-17
 * Description:	
 *				Library for manipulating KFM(cpumem) through general netlink
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "libkfm_cpumem.h"
#include "kfm_nl.h"


static int
kfm_nl_fill_cache_attr(struct nl_msg *msg, void *ucache)
{
	struct kfm_cache_u *uc = (struct kfm_cache_u *)ucache;
	
	NLA_PUT_U32(msg, KFM_CACHE_ATTR_CPU, uc->cpu);
	NLA_PUT_U32(msg, KFM_CACHE_ATTR_STATUS, uc->status);

	return 0;
	
nla_put_failure:
	return -1;
}

int
kfm_cache_disable(struct kfm_cache_u *cache)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_CACHE_DISABLE;
	fm_nl.flags = 0;
	fm_nl.fill_attr = kfm_nl_fill_cache_attr;
	fm_nl.fill_attr_data = (void *)cache;
	fm_nl.recvmsg_cb = kfm_nl_noop_cb;
	fm_nl.recvmsg_cb_data = NULL;

	return kfm_nl_process_message(&fm_nl);
}

int
kfm_cache_disable_all(void)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_ALL_CACHE_DISABLE;
	fm_nl.flags = 0;
	fm_nl.fill_attr = NULL;
	fm_nl.fill_attr_data = NULL;
	fm_nl.recvmsg_cb = kfm_nl_noop_cb;
	fm_nl.recvmsg_cb_data = NULL;

	return kfm_nl_process_message(&fm_nl);
}

int
kfm_cache_enable(struct kfm_cache_u *cache)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_CACHE_ENABLE;
	fm_nl.flags = 0;
	fm_nl.fill_attr = kfm_nl_fill_cache_attr;
	fm_nl.fill_attr_data = (void *)cache;
	fm_nl.recvmsg_cb = kfm_nl_noop_cb;
	fm_nl.recvmsg_cb_data = NULL;

	return kfm_nl_process_message(&fm_nl);
}

int
kfm_cache_enable_all(void)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_ALL_CACHE_ENABLE;
	fm_nl.flags = 0;
	fm_nl.fill_attr = NULL;
	fm_nl.fill_attr_data = NULL;
	fm_nl.recvmsg_cb = kfm_nl_noop_cb;
	fm_nl.recvmsg_cb_data = NULL;

	return kfm_nl_process_message(&fm_nl);
}

int
kfm_cache_flush(struct kfm_cache_u *cache)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_CACHE_FLUSH;
	fm_nl.flags = 0;
	fm_nl.fill_attr = kfm_nl_fill_cache_attr;
	fm_nl.fill_attr_data = (void *)cache;
	fm_nl.recvmsg_cb = kfm_nl_noop_cb;
	fm_nl.recvmsg_cb_data = NULL;

	return kfm_nl_process_message(&fm_nl);
}

int
kfm_cache_flush_all(void)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_ALL_CACHE_FLUSH;
	fm_nl.flags = 0;
	fm_nl.fill_attr = NULL;
	fm_nl.fill_attr_data = NULL;
	fm_nl.recvmsg_cb = kfm_nl_noop_cb;
	fm_nl.recvmsg_cb_data = NULL;

	return kfm_nl_process_message(&fm_nl);
}

static int
kfm_nl_cache_parse(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[KFM_CACHE_ATTR_MAX + 1];
	struct kfm_cache_u *ucache = (struct kfm_cache_u *)arg;
	
	if (genlmsg_parse(nlh, 0, attrs, KFM_CACHE_ATTR_MAX, kfm_cache_nla_policy) != 0)
		return -1;

	if (!(attrs[KFM_CACHE_ATTR_CPU] &&
		  attrs[KFM_CACHE_ATTR_STATUS]))
		return -1;

	if (ucache) {
		ucache->cpu = nla_get_u32(attrs[KFM_CACHE_ATTR_CPU]);
		ucache->status = nla_get_u32(attrs[KFM_CACHE_ATTR_STATUS]);
	}
		
	return NL_OK;
}

int
kfm_cache_get_info(struct kfm_cache_u *cache)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_GET_CACHE_INFO;
	fm_nl.flags = 0;
	fm_nl.fill_attr = kfm_nl_fill_cache_attr;
	fm_nl.fill_attr_data = (void *)cache;
	fm_nl.recvmsg_cb = kfm_nl_cache_parse;
	fm_nl.recvmsg_cb_data = (void *)cache;

	return kfm_nl_process_message(&fm_nl);
}

static int
kfm_nl_fill_mempage_attr(struct nl_msg *msg, void *umempage)
{
	struct kfm_mempage_u *ump = (struct kfm_mempage_u *)umempage;
	
	NLA_PUT_U64(msg, KFM_MEMPAGE_ATTR_ADDR, ump->addr);
	
	return 0;
	
nla_put_failure:
	return -1;
}

static int
kfm_nl_mempage_parse(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[KFM_MEMPAGE_ATTR_MAX + 1];
	struct kfm_mempage_u *umempage = (struct kfm_mempage_u *)arg;
	
	if (genlmsg_parse(nlh, 0, attrs, KFM_MEMPAGE_ATTR_MAX, kfm_mempage_nla_policy) != 0)
		return -1;

	if (!(attrs[KFM_MEMPAGE_ATTR_ADDR] &&
		  attrs[KFM_MEMPAGE_ATTR_PFN] &&
		  attrs[KFM_MEMPAGE_ATTR_FLAGS]))
		return -1;

	if (umempage) {
		umempage->addr= nla_get_u64(attrs[KFM_MEMPAGE_ATTR_ADDR]);
		umempage->pfn= nla_get_u64(attrs[KFM_MEMPAGE_ATTR_PFN]);
		umempage->flags= nla_get_u64(attrs[KFM_MEMPAGE_ATTR_FLAGS]);
	}
		
	return NL_OK;
}

int
kfm_mempage_get_info(struct kfm_mempage_u *mempage)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_GET_MEMPAGE_INFO;
	fm_nl.flags = 0;
	fm_nl.fill_attr = kfm_nl_fill_mempage_attr;
	fm_nl.fill_attr_data = (void *)mempage;
	fm_nl.recvmsg_cb = kfm_nl_mempage_parse;
	fm_nl.recvmsg_cb_data = (void *)mempage;

	return kfm_nl_process_message(&fm_nl);
}

int
kfm_mempage_online(struct kfm_mempage_u *mempage)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_MEMPAGE_ONLINE;
	fm_nl.flags = 0;
	fm_nl.fill_attr = kfm_nl_fill_mempage_attr;
	fm_nl.fill_attr_data = (void *)mempage;
	fm_nl.recvmsg_cb = kfm_nl_noop_cb;
	fm_nl.recvmsg_cb_data = NULL;

	return kfm_nl_process_message(&fm_nl);
}
