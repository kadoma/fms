/*
 * netutils.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * 
 * This file contains commons functions used in many of the plugins
 * for network utilities.
 */

#include "common.h"
#include "netutils.h"

/*Ben add*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int econn_refuse_state = STATE_CRITICAL;
int was_refused = FALSE;

int address_family = AF_INET;

/* opens a tcp or udp connection to a remote host or local socket */
int
np_net_connect (const char *host_name, int port, int *sd, int proto)
{
	struct addrinfo hints;
	struct addrinfo *r, *res;
	struct sockaddr_un su;
	char port_str[6], host[MAX_HOST_ADDRESS_LENGTH];
	size_t len;
	int socktype, result;

	socktype = (proto == IPPROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;

	/* as long as it doesn't start with a '/', it's assumed a host or ip */
	if(host_name[0] != '/'){
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = address_family;
		hints.ai_protocol = proto;
		hints.ai_socktype = socktype;

		len = strlen(host_name);
		/* check for an [IPv6] address (and strip the brackets) */
		if (len >= 2 && host_name[0] == '[' && host_name[len - 1] == ']') {
			host_name++;
			len -= 2;
		}
		if (len >= sizeof(host))
			return STATE_UNKNOWN;
		memcpy(host, host_name, len);
		host[len] = '\0';
		snprintf(port_str, sizeof (port_str), "%d", port);
		result = getaddrinfo(host, port_str, &hints, &res);

		if (result != 0) {
			printf("%s\n", gai_strerror (result));
			return STATE_UNKNOWN;
		}

		r = res;
		while (r) {
			/* attempt to create a socket */
			*sd = socket(r->ai_family, socktype, r->ai_protocol);

			if (*sd < 0) {
				printf("%s\n", "Socket creation failed");
				freeaddrinfo (r);
				return STATE_UNKNOWN;
			}

			/* attempt to open a connection */
			result = connect(*sd, r->ai_addr, r->ai_addrlen);

			if (result == 0) {
				was_refused = FALSE;
				break;
			}

			if (result < 0) {
				switch (errno) {
				case ECONNREFUSED:
					was_refused = TRUE;
					break;
				}
			}

			close (*sd);
			r = r->ai_next;
		}
		freeaddrinfo(res);
	}
	/* else the hostname is interpreted as a path to a unix socket */
	else {
		if(strlen(host_name) >= UNIX_PATH_MAX){
			printf("Supplied path too long unix domain socket");
		}
		memset(&su, 0, sizeof(su));
		su.sun_family = AF_UNIX;
		strncpy(su.sun_path, host_name, UNIX_PATH_MAX);
		*sd = socket(PF_UNIX, SOCK_STREAM, 0);
		if(*sd < 0){
			printf("Socket creation failed");
		}
		result = connect(*sd, (struct sockaddr *)&su, sizeof(su));
		if (result < 0 && errno == ECONNREFUSED)
			was_refused = TRUE;
	}

	if (result == 0) {
		close (*sd);
		return STATE_OK;
	} else if (was_refused) {
		switch (econn_refuse_state) { /* a user-defined expected outcome */
		case STATE_OK:
		case STATE_WARNING:  /* user wants WARN or OK on refusal */
			close (*sd);
			return econn_refuse_state;
			break;
		case STATE_CRITICAL: /* user did not set econn_refuse_state */
			printf ("%s\n", strerror(errno));
			close (*sd);
			return econn_refuse_state;
			break;
		default: /* it's a logic error if we do not end up in STATE_(OK|WARNING|CRITICAL) */
			close (*sd);
			return STATE_UNKNOWN;
			break;
		}
	} else {
		printf ("%s\n", strerror(errno));
		close (*sd);
		return STATE_CRITICAL;
	}
}

