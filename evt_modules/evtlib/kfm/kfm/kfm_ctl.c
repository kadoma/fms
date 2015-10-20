/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	kfm_ctl.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-17
 * Description:	
 *				Fault Management Control.
 *
 */

#include <linux/mutex.h>
#include <net/genetlink.h>
#include <linux/version.h>

#include "kfm.h"

/* semaphore for KFM. */
static DEFINE_MUTEX(__kfm_mutex);

int sysctl_kfm_unload = 0;


#ifdef CONFIG_KFM_DEBUG
static int sysctl_kfm_debug_level = 0;

int
kfm_get_debug_level(void)
{
	return sysctl_kfm_debug_level;
}
#endif


/*
 * Generic Netlink interface
 */

/* KFM genetlink family */
static struct genl_family kfm_genl_family = {
	.id			= GENL_ID_GENERATE,
	.hdrsize	= 0,
	.name		= KFM_GENL_NAME,
	.version	= KFM_GENL_VERSION,
	.maxattr	= KFM_CMD_MAX,
	.netnsok    = true,
};


static struct genl_multicast_group kfm_mcgrps[] = {
	[KFM_MCGRP_CPUMEM]	= { .name = "cpumem", },
	[KFM_MCGRP_DISK]	= { .name = "disk", },
	[KFM_MCGRP_NET]		= { .name = "net", },
};

/* Policy used for cache */
static const struct nla_policy kfm_cache_nla_policy[KFM_CACHE_ATTR_MAX + 1] = {
	[KFM_CACHE_ATTR_CPU]		= { .type = NLA_U32 },
	[KFM_CACHE_ATTR_STATUS]		= { .type = NLA_U32 },
};

/* Policy used for mempage */
static const struct nla_policy kfm_mempage_nla_policy[KFM_MEMPAGE_ATTR_MAX + 1] = {
	[KFM_MEMPAGE_ATTR_ADDR]		= { .type = NLA_U64 },
	[KFM_MEMPAGE_ATTR_PFN]		= { .type = NLA_U64 },
	[KFM_MEMPAGE_ATTR_FLAGS]	= { .type = NLA_U64 },
};


static int 
kfm_genl_parse_cache(struct kfm_cache_u *ucache, struct nlattr **attrs)
{
	struct nlattr *nla_cpu, *nla_status;
		
	if(attrs == NULL || ucache == NULL)
		return -EINVAL;

	nla_cpu = attrs[KFM_CACHE_ATTR_CPU];
	nla_status = attrs[KFM_CACHE_ATTR_STATUS];

	if (!nla_cpu || !nla_status)
		return -EINVAL;

	memset(ucache, 0, sizeof(*ucache));

	ucache->cpu = nla_get_u32(nla_cpu);
	ucache->status = nla_get_u32(nla_status);

	return 0;
}

static int 
kfm_genl_fill_cache(struct sk_buff *skb, struct kfm_cache_u *ucache)
{	
	if (nla_put_u32(skb, KFM_CACHE_ATTR_CPU, ucache->cpu))
		goto nla_put_failure;
	
	if (nla_put_u32(skb, KFM_CACHE_ATTR_STATUS, ucache->status))
		goto nla_put_failure;
	
	return 0;

nla_put_failure:
	return -EMSGSIZE;
}

static int 
kfm_genl_parse_mempage(struct kfm_mempage_u *umempage, struct nlattr **attrs)
{
	struct nlattr *nla_addr;
		
	if(attrs == NULL || umempage == NULL)
		return -EINVAL;

	nla_addr = attrs[KFM_MEMPAGE_ATTR_ADDR];
	
	if (!nla_addr)
		return -EINVAL;

	memset(umempage, 0, sizeof(*umempage));

	umempage->addr = nla_get_u64(nla_addr);
	
	KFM_DBG(7, "ADDR:0x%016llx", umempage->addr);
	
	return 0;
}

static int 
kfm_genl_fill_mempage(struct sk_buff *skb, struct kfm_mempage_u *umempage)
{	
	KFM_DBG(7, "ADDR:0x%016llx, PFN:0x%016llx, FLAGS:0x%016llx", 
		umempage->addr, umempage->pfn, umempage->flags);
	
	if (nla_put_u64(skb, KFM_MEMPAGE_ATTR_ADDR, umempage->addr))
		goto nla_put_failure;

	if (nla_put_u64(skb, KFM_MEMPAGE_ATTR_PFN, umempage->pfn))
		goto nla_put_failure;
	
	if (nla_put_u64(skb, KFM_MEMPAGE_ATTR_FLAGS, umempage->flags))
		goto nla_put_failure;
	
	return 0;

nla_put_failure:
	return -EMSGSIZE;
}

static int 
kfm_genl_set_cmd(struct sk_buff *skb, struct genl_info *info)
{
	struct kfm_cache_u ucache;
	struct kfm_mempage_u umempage;
	int ret = 0, cmd;

	cmd = info->genlhdr->cmd;

	/* increase the module use count */
	kfm_use_count_inc();
	
	mutex_lock(&__kfm_mutex);	// need lock??	

	/* parse user param */
	switch (cmd) {
	case KFM_CMD_CACHE_DISABLE:
	case KFM_CMD_CACHE_ENABLE:
	case KFM_CMD_CACHE_FLUSH:
		ret = kfm_genl_parse_cache(&ucache, info->attrs);
		if (ret)
			goto out;
		break;
	case KFM_CMD_MEMPAGE_ONLINE:
		ret = kfm_genl_parse_mempage(&umempage, info->attrs);
		if (ret)
			goto out;
		break;
	}

	/* operation cmd */
	switch (cmd) {
	case KFM_CMD_CACHE_DISABLE:
		kfm_cache_disable_on_cpu(ucache.cpu);
		break;
		
	case KFM_CMD_ALL_CACHE_DISABLE:
		kfm_cache_disable_all();
		break;
		
	case KFM_CMD_CACHE_ENABLE:
		kfm_cache_enable_on_cpu(ucache.cpu);
		break;
		
	case KFM_CMD_ALL_CACHE_ENABLE:
		kfm_cache_enable_all();
		break;
		
	case KFM_CMD_CACHE_FLUSH:
		kfm_cache_flush_on_cpu(ucache.cpu);
		break;
		
	case KFM_CMD_ALL_CACHE_FLUSH:
		kfm_cache_flush_all();
		break;

	case KFM_CMD_MEMPAGE_ONLINE:
		ret = kfm_mempage_online(umempage.addr);
		break;
		
	default:
		ret = -EINVAL;
	}

out:
	mutex_unlock(&__kfm_mutex);
	kfm_use_count_dec();

	return ret;
}

static int 
kfm_genl_get_cmd(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *msg;
	void *reply;
	int ret, cmd, reply_cmd;
	
	cmd = info->genlhdr->cmd;

	switch (cmd) {
	case KFM_CMD_GET_INFO:
		reply_cmd = KFM_CMD_GET_INFO_REPLY;
		break;
	case KFM_CMD_GET_CACHE_INFO:
		reply_cmd = KFM_CMD_GET_CACHE_INFO_REPLY;
		break;
	case KFM_CMD_GET_MEMPAGE_INFO:
		reply_cmd = KFM_CMD_GET_MEMPAGE_INFO_REPLY;
		break;
	default:
		KFM_ERR("unknown Generic Netlink command\n");
		return -EINVAL;
	}
	
	msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	mutex_lock(&__kfm_mutex);  // need lock??

	reply = genlmsg_put_reply(msg, info, &kfm_genl_family, 0, reply_cmd);
	if (reply == NULL)
		goto nla_put_failure;

	switch (cmd) {
	case KFM_CMD_GET_INFO:
		if (nla_put_u32(msg, KFM_INFO_ATTR_VERSION, KFM_VERSION_CODE))
			goto nla_put_failure;
		break;
	
	case KFM_CMD_GET_CACHE_INFO:
	{
		struct kfm_cache_u ucache;
		
		ret = kfm_genl_parse_cache(&ucache, info->attrs);
		if (ret)
			goto out_err;

		if((ret = kfm_cache_status_on_cpu(ucache.cpu)) < 0)
			goto out_err;
		ucache.status = ret;

		ret = kfm_genl_fill_cache(msg, &ucache);
		if (ret)
				goto nla_put_failure;

		break;
	}

	case KFM_CMD_GET_MEMPAGE_INFO:
	{
		struct kfm_mempage_u umempage;

		ret = kfm_genl_parse_mempage(&umempage, info->attrs);
		if (ret)
			goto out_err;

		if(kfm_get_mempage_info(umempage.addr, &umempage))
			goto out_err;

		ret = kfm_genl_fill_mempage(msg, &umempage);
		if (ret)
				goto nla_put_failure;
		
		break;
	}
	
	/* for other get cmd */
	}

	genlmsg_end(msg, reply);
	ret = genlmsg_reply(msg, info);
	goto out;

nla_put_failure:
	pr_err("not enough space in Netlink message\n");
	ret = -EMSGSIZE;

out_err:
	nlmsg_free(msg);
out:
	mutex_unlock(&__kfm_mutex);

	return ret;
}

static struct genl_ops kfm_genl_ops[] = {
	{
		.cmd	= KFM_CMD_GET_INFO,
		.flags	= GENL_ADMIN_PERM,
		.doit	= kfm_genl_get_cmd,
	},
	
	/* for cache, begin */
	{
		.cmd	= KFM_CMD_CACHE_FLUSH,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_cache_nla_policy,
		.doit	= kfm_genl_set_cmd,
	},
	{
		.cmd	= KFM_CMD_ALL_CACHE_FLUSH,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_cache_nla_policy,
		.doit	= kfm_genl_set_cmd,
	},
	{
		.cmd	= KFM_CMD_CACHE_DISABLE,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_cache_nla_policy,
		.doit	= kfm_genl_set_cmd,
	},
	{
		.cmd	= KFM_CMD_ALL_CACHE_DISABLE,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_cache_nla_policy,
		.doit	= kfm_genl_set_cmd,
	},
	{
		.cmd	= KFM_CMD_CACHE_ENABLE,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_cache_nla_policy,
		.doit	= kfm_genl_set_cmd,
	},
	{
		.cmd	= KFM_CMD_ALL_CACHE_ENABLE,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_cache_nla_policy,
		.doit	= kfm_genl_set_cmd,
	},
	{
		.cmd	= KFM_CMD_GET_CACHE_INFO,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_cache_nla_policy,
		.doit	= kfm_genl_get_cmd,
	},
	/* for cache, end */
	
	/* for mem, begin */
	{
		.cmd	= KFM_CMD_MEMPAGE_ONLINE,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_mempage_nla_policy,
		.doit	= kfm_genl_set_cmd,
	},
	{
		.cmd	= KFM_CMD_GET_MEMPAGE_INFO,
		.flags	= GENL_ADMIN_PERM,
		.policy	= kfm_mempage_nla_policy,
		.doit	= kfm_genl_get_cmd,
	},
	/* for mem, end */
};


static int __init 
kfm_genl_register(void)
{
// kernel 3.10.0-229 centos 7.1
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,1)			/* for kanas (kernel 4.1.1) */
	return genl_register_family_with_ops_groups(&kfm_genl_family, 
											kfm_genl_ops,
											kfm_mcgrps);
#else    /* for centos 7.0 (kernel 3.10.0) */
	{
		int ret = 0, i;
		
		ret = genl_register_family_with_ops(&kfm_genl_family,
										kfm_genl_ops,
										ARRAY_SIZE(kfm_genl_ops));
		if (ret) 
			return -1;

		for (i = 0; i < ARRAY_SIZE(kfm_mcgrps); i++) {
			ret = genl_register_mc_group(&kfm_genl_family, &kfm_mcgrps[i]);
			if (ret) {
				genl_unregister_family(&kfm_genl_family);
				return -1;
			}
		}
		
		return 0;	
	}
#endif
}

static void 
kfm_genl_unregister(void)
{
	genl_unregister_family(&kfm_genl_family);
}

/* End of Generic Netlink interface definitions */



#ifdef CONFIG_SYSCTL
static
int kfm_sysctl_unload_handler(struct ctl_table *ctl, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);

	if (sysctl_kfm_unload) {
		kfm_post_evt_thread_exit();
	}
	
	return ret;
}


static struct ctl_table_header *kfm_table_header;

static struct ctl_table kfm_table[] = {
#ifdef CONFIG_KFM_DEBUG
	{
		.procname 	= "debug_level", 
		.data		= &sysctl_kfm_debug_level,
		.maxlen		= sizeof(int), 
		.mode		= 0644, 
		.proc_handler	= &proc_dointvec,
	},
#endif
	{
		.procname	= "unload", 
		.data		= &sysctl_kfm_unload,
		.maxlen 	= sizeof(int), 
		.mode		= 0644, 
		.proc_handler	= &kfm_sysctl_unload_handler,
	},
	{0}
};

static const struct ctl_path kfm_ctl_path[] = {
	{ .procname = "fm"},
	{ }
};
#endif

int 
kfm_control_start(void)
{	
	int ret;

	ret = kfm_genl_register();
	if (ret) {
		KFM_ERR("cannot register Generic Netlink interface.\n");
		return ret;
	}
	
#ifdef CONFIG_SYSCTL	
	kfm_table_header = register_sysctl_paths(kfm_ctl_path, kfm_table);
#endif
	
	return ret;
}

void 
kfm_control_stop(void)
{
	EnterFunction(1);
	
#ifdef CONFIG_SYSCTL	
	unregister_sysctl_table(kfm_table_header);
#endif
	
	kfm_genl_unregister();
	
	LeaveFunction(1);
}
