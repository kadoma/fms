#ifndef __KFM_NL_H
#define __KFM_NL_H

#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "../kfm/kfm.h"

struct kfm_nl {
	int cmd;
	int flags;
	int (*fill_attr)(struct nl_msg*, void *);
	void *fill_attr_data;
	nl_recvmsg_msg_cb_t recvmsg_cb;
	void *recvmsg_cb_data;
};

extern int kfm_nl_connect(struct nl_handle **sock, int *family);
extern int kfm_nl_multicast_connect(struct nl_handle **sock, int *family, uint32_t group);
extern void kfm_nl_connect_close(struct nl_handle *sock);
extern struct nl_msg* kfm_nl_new_message(int cmd, int family, int flags);
extern void kfm_nl_free_message(struct nl_msg *msg);
extern int kfm_nl_noop_cb(struct nl_msg *msg, void *arg);
extern int kfm_nl_send_message(struct nl_handle *sock, struct nl_msg *msg, 
			nl_recvmsg_msg_cb_t func, void *arg);
extern int kfm_nl_process_message(struct kfm_nl *fm_nl);

#endif
