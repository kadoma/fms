#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <linux/fm/fm.h>

#include "get_fm_event.h"

#ifndef SOL_NETLINK
#define SOL_NETLINK		270
#endif

#ifndef NETLINK_KFMD
#define NETLINK_KFMD 20 /* fault managment system */
#endif

/*
 * kernel_event_recv
 *  retrieve fm message from kernel via netlink
 *
 * Inputs
 *  none
 *
 * Return value
 *  Pointer to struct fm_kevt which contain the kernel evt
 *  NULL if failed 
 */
struct fm_kevt *
kernel_event_recv(void)
{
	int 	group = 1;
	int 	sock_fd;
	struct 	sockaddr_nl 	src_addr;
	struct 	nlmsghdr 	*nlh = NULL;
	struct 	iovec 		iov;
	struct 	msghdr 		msg;
	struct 	fm_kevt 	*kevt;		/* evt from kernel */
	
	kevt = (struct fm_kevt*)malloc(sizeof(struct fm_kevt));
	if ( kevt == NULL ) {
		fprintf(stderr, "SYSMON: Unable to alloc memory\n");
		return NULL;
	}
	memset(&msg, 0, sizeof(struct msghdr));
	memset(kevt, 0, sizeof(struct fm_kevt));
	memset(&src_addr, 0, sizeof(struct sockaddr_nl));

	sock_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KFMD);
	if (sock_fd < 0){
		fprintf(stderr, "SYSMON: Unable to create socket\n");
		return NULL;
	}

	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = (pthread_self() << 16)/* in case other module use netlink too */
		|| getpid();			
	src_addr.nl_groups = 1;			/* in mcast groups */
	bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

	if ( setsockopt(sock_fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP,
		       	&group, sizeof(group)) < 0 ){
		fprintf(stderr,"SYSMON: Setsockopt error\n");	
		close(sock_fd);
		return NULL;
	}

	/* Fill the netlink message header */
	nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);
	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	/* Read message from kernel */
	if ( recvmsg(sock_fd, &msg, MSG_DONTWAIT) == -1 ){
		if ( !errno == EAGAIN )		/* no message arrived? */
			fprintf(stderr, "SYSMON: recvmsg failed\n");
		close(sock_fd);
		return NULL;
	}
	kevt = (struct fm_kevt *)NLMSG_DATA(nlh);

	/* Close Netlink Socket */
	close(sock_fd);

	return kevt;
}

