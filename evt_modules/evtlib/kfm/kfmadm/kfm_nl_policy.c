/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	kfm_nl_policy.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-17
 * Description:	
 *				kfm general netlink parse policy.
 *
 */

#include "kfm_nl.h"


struct nla_policy kfm_info_nla_policy[KFM_INFO_ATTR_MAX + 1] = {
	[KFM_INFO_ATTR_VERSION]	= { .type = NLA_U32 },
};

struct nla_policy kfm_cache_nla_policy[KFM_CACHE_ATTR_MAX + 1] = {
	[KFM_CACHE_ATTR_CPU]		= { .type = NLA_U32 },
	[KFM_CACHE_ATTR_STATUS]		= { .type = NLA_U32 },
};

struct nla_policy kfm_mempage_nla_policy[KFM_MEMPAGE_ATTR_MAX + 1] = {
	[KFM_MEMPAGE_ATTR_ADDR]		= { .type = NLA_U64 },
	[KFM_MEMPAGE_ATTR_PFN]		= { .type = NLA_U64 },
	[KFM_MEMPAGE_ATTR_FLAGS]	= { .type = NLA_U64 },
};
