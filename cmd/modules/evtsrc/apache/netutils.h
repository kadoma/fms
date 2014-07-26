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
 * Apache Service Event Source Module net utilities include file
 *
 * This file contains common include files and function definitions
 * used in many of the apache service event source modules.
 */

#ifndef _NETUTILS_H
#define _NETUTILS_H

//#include "common.h"
#include "utils.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/un.h>

/* linux uses this, on sun it's hard-coded at 108 without a define */
#define UNIX_PATH_MAX 108

/* my_connect and wrapper macros */
#define my_tcp_connect(addr, port, s) np_net_connect(addr, port, s, IPPROTO_TCP)
#define my_udp_connect(addr, port, s) np_net_connect(addr, port, s, IPPROTO_UDP)
int np_net_connect(const char *address, int port, int *sd, int proto);

extern int econn_refuse_state;
extern int was_refused;
extern int address_family;

#endif  /* _NETUTILS_H */
