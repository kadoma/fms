/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	kfm_core.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-17
 * Description:	
 *				Fault Management Kernel Module.
 *
 */

#include <linux/kthread.h>
#include <linux/wait.h>

#include "kfm.h"

#define POST_THREAD_NAME	"KFM post"

struct task_struct *post_task;
static int g_post_event = 0;

DECLARE_WAIT_QUEUE_HEAD(post_thread_wq);

int
kfm_use_count_inc(void)
{
	return try_module_get(THIS_MODULE);
}

void
kfm_use_count_dec(void)
{
	module_put(THIS_MODULE);
}

void
kfm_post_evt_thread_exit(void)
{
	if (post_task && !IS_ERR(post_task)) {
		/* lock */
		g_post_event = 1;
		/* unlock */
		kthread_stop(post_task);
		wake_up(&post_thread_wq);
	}
}

static int
kfm_post_evt_thread(void *unused)
{		
	kfm_use_count_inc();
	
	/* Block all signals except SIGKILL and SIGSTOP */
	spin_lock_irq(&current->sighand->siglock);
	siginitsetinv(&current->blocked, sigmask(SIGKILL) | sigmask(SIGSTOP));
	recalc_sigpending();
	spin_unlock_irq(&current->sighand->siglock);

	/* register post event callback */
	// TODO
	
	/* main loop */
	while (!kthread_should_stop() && !sysctl_kfm_unload) {
		
		/* 
		 *	send fault event (disk, cpu, mem, net etc)
		 *  TODO
		 */
		 
		if (signal_pending(current)) {
			KFM_DBG(1, "signal_pending break\n");
			break;
		}

		wait_event_interruptible(post_thread_wq, g_post_event);

		KFM_DBG(1, "send event one times\n");
	}

	/* unregister post event callback */
	//TODO
	
	KFM_DBG(1, "The post event thread stopped.\n");
	
	post_task = NULL;
	kfm_use_count_dec();
	
	return 0;
}

static int __init 
kfm_init(void)
{
	int ret = 0;
	
	ret = kfm_control_start();
	if (ret) {
		KFM_ERR("Failed to start control. err=%d\n", ret);
		return ret;
	}

	post_task = kthread_run(kfm_post_evt_thread, NULL, POST_THREAD_NAME);
	if (IS_ERR(post_task)) {
		KFM_ERR("Failed to start post event thread.\n");
		ret = -ENOMEM;
		goto control_stop;
	}
	
	pr_info("kfm loaded.\n");

	return 0;

control_stop:
	kfm_control_stop();
	return ret;
}

static void __exit 
kfm_cleanup(void)
{
	kfm_control_stop();
	
	pr_info("kfm unloaded.\n");
}

module_init(kfm_init);
module_exit(kfm_cleanup);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(KFM_MODULE_DESC);
