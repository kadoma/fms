/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	kfm_nl.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-17
 * Description:	
 *				kfm general netlink.
 *
 */

#include <stdlib.h>
#include <errno.h>

#include "kfm_nl.h"

int
kfm_nl_connect(struct nl_handle **sock, int *family)
{
	struct nl_handle *sk = NULL;
	int fml;

	if (!sock || !family) {
		errno = EINVAL;
		return -1;
	}
	
	sk = nl_handle_alloc();
	if (!sk) {
		return -1;
	}

	if (genl_connect(sk) < 0)
		goto fail_genl;

	fml = genl_ctrl_resolve(sk, KFM_GENL_NAME);
	if (fml < 0)
		goto fail_genl;

	*sock = sk;
	*family = fml;
	
	return 0;

fail_genl:
	nl_handle_destroy(sk);
	return -1;
}

int
kfm_nl_multicast_connect(struct nl_handle **sock, int *family, uint32_t group)
{	
	struct nl_handle *sk = NULL;
	
	if (kfm_nl_connect(sock, family))
		return -1;

	sk = *sock;
//	if (nl_socket_add_memberships(sk, group, 0))
//		goto fail_genl;

	if (nl_socket_add_membership(sk, group))
		goto fail_genl;

	return 0;

fail_genl:
	nl_handle_destroy(sk);
	return -1;
}

void
kfm_nl_connect_close(struct nl_handle *sock)
{
	if (sock)
		nl_handle_destroy(sock);
}

struct nl_msg *
kfm_nl_new_message(int cmd, int family, int flags)
{
	struct nl_msg *msg;

	msg = nlmsg_alloc();
	if (!msg)
		return NULL;

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, flags,
		    cmd, KFM_GENL_VERSION);

	return msg;
}

void
kfm_nl_free_message(struct nl_msg *msg)
{
	if (msg)
		nlmsg_free(msg);
}

int 
kfm_nl_noop_cb(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

int 
kfm_nl_send_message(struct nl_handle *sock, struct nl_msg *msg, 
			nl_recvmsg_msg_cb_t func, void *arg)
{	
	int err = 0;
	
	if (!sock || !msg) {
		errno = EINVAL;
		return -1;
	}

	if (nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, func, arg) != 0)
		goto fail_genl;

	if (nl_send_auto_complete(sock, msg) < 0)
		goto fail_genl;

	if ((err = nl_recvmsgs_default(sock)) < 0) {
		errno = -err;
		goto fail_genl;
	}
	
	return 0;

fail_genl:
	return -1;	
}

int 
kfm_nl_process_message(struct kfm_nl *fm_nl)
{
	struct nl_handle *sock;
	struct nl_msg *msg;
	int family;

	if (!fm_nl || !fm_nl->recvmsg_cb) {
		errno = EINVAL;
		return -1;
	}
	
	if (kfm_nl_connect(&sock, &family))
		return -1;
	
	msg = kfm_nl_new_message(fm_nl->cmd, family, fm_nl->flags);
	if (!msg)
		goto cleanup_sock;

	if (fm_nl->fill_attr && fm_nl->fill_attr(msg, fm_nl->fill_attr_data))
		goto cleanup_msg;
	
	if (kfm_nl_send_message(sock, msg, fm_nl->recvmsg_cb, fm_nl->recvmsg_cb_data))
		goto cleanup_msg;
	
	kfm_nl_free_message(msg);
	kfm_nl_connect_close(sock);
	
	return 0;

cleanup_msg:
	kfm_nl_free_message(msg);
cleanup_sock:
	kfm_nl_connect_close(sock);
	return -1;
}
