/*
 * =========================================================
 *
 *       Filename:  mpio.c
 *
 *    Description:  listen the mpio errors and report to FMD
 *
 *        Version:  1.0
 *        Created:  10/21/2010 09:10:03 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: Inspur OS Team
 *
 * =========================================================
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/user.h>
#include <sys/un.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <sys/mman.h>
#include "mpio.h"

static struct uevent * alloc_uevent (void)
{
	return (struct uevent *)malloc(sizeof(struct uevent));
}

char *get_mpio(void)
{
	int 	sock;
	struct 	sockaddr_nl snl;
	int 	retval;
	int 	rcvbufsz = 128*1024;
	int 	rcvsz = 0;
	int 	rcvszsz = sizeof(rcvsz);
	unsigned int *prcvszsz = (unsigned int *)&rcvszsz;
	const 	int feature_on = 1;
	char 	*msg_ret = NULL;


	/* read kernel netlink events */
	memset(&snl, 0x00, sizeof(struct sockaddr_nl));
	snl.nl_family = AF_NETLINK;
	snl.nl_pid = pthread_self() << 16 || getpid();
	snl.nl_groups = 0x01;

	sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (sock == -1) {
		fprintf(stderr, "MPIO: Error getting socket, exit\n");
		return NULL;
	}
	/*
	 * try to avoid dropping uevents, even so, this is not a guarantee,
	 * but it does help to change the netlink uevent socket's
	 * receive buffer threshold from the default value of 106,496 to
	 * the maximum value of 262,142.
	 */
	retval = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbufsz,
			    sizeof(rcvbufsz));

	if (retval < 0) {
		fprintf(stderr, "MPIO: Error setting receive buffer size for socket, exit\n");
		close(sock);
		return NULL;
	}
	retval = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvsz, prcvszsz);
	if (retval < 0) {
		fprintf(stderr, "MPIO: Error setting receive buffer size for socket, exit\n");
		close(sock);
		return NULL;
	}
	/* enable receiving of the sender credentials */
	setsockopt(sock, SOL_SOCKET, SO_PASSCRED,
		   &feature_on, sizeof(feature_on));

	retval = bind(sock, (struct sockaddr *) &snl,
		      sizeof(struct sockaddr_nl));
	if (retval < 0) {
		fprintf(stderr, "MPIO: Bind failed, exit\n");
		close(sock);
		return NULL;
	}

		do{
		int 	i;
		char	*pos;
		size_t 	bufpos;
		ssize_t buflen;
		char 	*buffer;
		struct 	uevent 	*uev;
		struct 	msghdr 	smsg;
		struct 	iovec 	iov;
		static 	char 	buf[HOTPLUG_BUFFER_SIZE + OBJECT_SIZE];

		memset(buf, 0x00, sizeof(buf));
		iov.iov_base = &buf;
		iov.iov_len = sizeof(buf);
		memset (&smsg, 0x00, sizeof(struct msghdr));
		smsg.msg_iov = &iov;
		smsg.msg_iovlen = 1;

		buflen = recvmsg(sock, &smsg, MSG_DONTWAIT);
		if (buflen < 0) {
			if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
				fprintf(stderr, "MPIO: Error receiving message\n");
			continue;
		}

		/* skip header */
		bufpos = strlen(buf) + 1;
		if (bufpos < sizeof("a@/d") || bufpos >= sizeof(buf)) {
			fprintf(stderr, "MPIO: Invalid message length\n");
			continue;
		}

		/* check message header */
		if (strstr(buf, "@/") == NULL) {
			fprintf(stderr, "MPIO: Unrecognized message header\n");
			continue;
		}

		uev = alloc_uevent();

		if (!uev) {
			fprintf(stderr, "MPIO: Lost uevent, oom\n");
			continue;
		}

		if ((size_t)buflen > sizeof(buf)-1)
			buflen = sizeof(buf)-1;

		/*
		 * Copy the shared receive buffer contents to buffer private
		 * to this uevent so we can immediately reuse the shared buffer.
		 */
		memcpy(uev->buffer, buf, HOTPLUG_BUFFER_SIZE + OBJECT_SIZE);
		buffer = uev->buffer;
		buffer[buflen] = '\0';

		/* save start of payload */
		bufpos = strlen(buffer) + 1;

		/* action string */
		uev->action = buffer;
		pos = strchr(buffer, '@');
		if (!pos) {
			fprintf(stderr, "MPIO: bad action string '%s'\n", buffer);
			continue;
		}
		pos[0] = '\0';

		/* sysfs path */
		uev->devpath = &pos[1];

		/* hotplug events have the environment attached - reconstruct envp[] */
		for (i = 0; (bufpos < (size_t)buflen) && (i < HOTPLUG_NUM_ENVP-1); i++) {
			int keylen;
			char *key;

			key = &buffer[bufpos];
			keylen = strlen(key);
			uev->envp[i] = key;
			bufpos += keylen + 1;
		}
		uev->envp[i] = NULL;
		uev->next = NULL;

		fprintf(stderr, "MPIO: uevent '%s' from '%s'\n", uev->action, uev->devpath);
 	
		msg_ret = malloc(256);	/*  256 is enough */
		int len = 0;
		len = sprintf(msg_ret, "DM.");
		/* print payload environment */
		for (i = 0; uev->envp[i] != NULL; i++)
		{
		//	fprintf(stderr, "%s\n", uev->envp[i]);
			if (strncmp("DM_NAME=", uev->envp[i], 8) )
					len += sprintf(msg_ret, "%s.", uev->envp[i]+8);
			if (strncmp("DM_ACTION=", uev->envp[i], 10) )
					len += sprintf(msg_ret, "%s.", uev->envp[i]+10);
			if (strncmp("DEVPATH=", uev->envp[i], 8) )
					len += sprintf(msg_ret, "%s.", uev->envp[i]+8);
			if (strncmp("DM_PATH=", uev->envp[i], 8) )
					len += sprintf(msg_ret, "%s.", uev->envp[i]+8);
			if (strncmp("DM_NR_VALID_PATHS=", uev->envp[i], 18) )
					len += sprintf(msg_ret, "%s", uev->envp[i]+18);
		}
	}while(0);
	close(sock);
	return msg_ret;
}


int main(int argc, char **argv)
{
	char *p;
	p = get_mpio();
	return 1;
}
